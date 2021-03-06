// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/trusted_vault/standalone_trusted_vault_backend.h"

#include <memory>
#include <utility>

#include "base/containers/span.h"
#include "base/files/file_util.h"
#include "base/files/important_file_writer.h"
#include "base/logging.h"
#include "base/sequence_checker.h"
#include "components/os_crypt/os_crypt.h"
#include "components/sync/driver/sync_driver_switches.h"
#include "components/sync/trusted_vault/securebox.h"

namespace syncer {

namespace {

sync_pb::LocalTrustedVault ReadEncryptedFile(const base::FilePath& file_path) {
  sync_pb::LocalTrustedVault proto;
  std::string ciphertext;
  std::string decrypted_content;
  if (base::ReadFileToString(file_path, &ciphertext) &&
      OSCrypt::DecryptString(ciphertext, &decrypted_content)) {
    proto.ParseFromString(decrypted_content);
  }

  return proto;
}

void WriteToDisk(const sync_pb::LocalTrustedVault& data,
                 const base::FilePath& file_path) {
  std::string encrypted_data;
  if (!OSCrypt::EncryptString(data.SerializeAsString(), &encrypted_data)) {
    DLOG(ERROR) << "Failed to encrypt trusted vault file.";
    return;
  }

  if (!base::ImportantFileWriter::WriteFileAtomically(file_path,
                                                      encrypted_data)) {
    DLOG(ERROR) << "Failed to write trusted vault file.";
  }
}

}  // namespace

StandaloneTrustedVaultBackend::StandaloneTrustedVaultBackend(
    const base::FilePath& file_path,
    std::unique_ptr<TrustedVaultConnection> connection)
    : file_path_(file_path), connection_(std::move(connection)) {}

StandaloneTrustedVaultBackend::~StandaloneTrustedVaultBackend() = default;

void StandaloneTrustedVaultBackend::ReadDataFromDisk() {
  data_ = ReadEncryptedFile(file_path_);
}

void StandaloneTrustedVaultBackend::FetchKeys(
    const CoreAccountInfo& account_info,
    FetchKeysCallback callback) {
  // Concurrent keys fetches aren't supported.
  DCHECK(ongoing_fetch_keys_callback_.is_null());
  DCHECK(!callback.is_null());

  ongoing_fetch_keys_callback_ = std::move(callback);
  ongoing_fetch_keys_gaia_id_ = account_info.gaia;

  const sync_pb::LocalTrustedVaultPerUser* per_user_vault =
      FindUserVault(account_info.gaia);

  // TODO(crbug.com/1094326): currently there is no guarantee that
  // |primary_account_| is set before FetchKeys() call and this may cause
  // redundant sync error in the UI (for key retrieval), especially during the
  // browser startup. Try to find a way to avoid this issue.
  if (!base::FeatureList::IsEnabled(switches::kFollowTrustedVaultKeyRotation) ||
      !primary_account_.has_value() ||
      primary_account_->gaia != account_info.gaia || !per_user_vault ||
      !per_user_vault->keys_are_stale() ||
      !per_user_vault->local_device_registration_info().device_registered()) {
    // Keys download attempt is not needed or not possible.
    FulfillOngoingFetchKeys();
    return;
  }

  // Current state guarantees there is no ongoing requests to the server:
  // 1. Current |primary_account_| is |account_info|, so there is no ongoing
  // request for other accounts.
  // 2. Device is already registered, so there is no device registration for
  // |account_info|.
  // 3. Concurrent FetchKeys() calls aren't supported, so there is no keys
  // download for |account_info|.
  DCHECK(!weak_factory_for_connection_.HasWeakPtrs());

  std::unique_ptr<SecureBoxKeyPair> key_pair =
      SecureBoxKeyPair::CreateByPrivateKeyImport(base::as_bytes(
          base::make_span(per_user_vault->local_device_registration_info()
                              .private_key_material())));
  if (!key_pair) {
    // Corrupted state: device is registered, but |key_pair| can't be imported.
    // TODO(crbug.com/1094326): restore from this state (throw away the key and
    // trigger device registration again).
    FulfillOngoingFetchKeys();
    return;
  }
  if (per_user_vault->vault_key_size() == 0) {
    // Corrupted state: device is registered, but there is no vault keys.
    // TODO(crbug.com/1094326): restore from this state (just mark device as not
    // registered?).
    FulfillOngoingFetchKeys();
    return;
  }

  // TODO(crbug.com/1094326): add throttling mechanism.
  std::string last_key = per_user_vault->vault_key()
                             .at(per_user_vault->vault_key_size() - 1)
                             .key_material();
  std::vector<uint8_t> last_key_bytes(last_key.begin(), last_key.end());
  connection_->DownloadKeys(
      *primary_account_, last_key_bytes,
      per_user_vault->last_vault_key_version(), std::move(key_pair),
      base::BindOnce(&StandaloneTrustedVaultBackend::OnKeysDownloaded,
                     weak_factory_for_connection_.GetWeakPtr(),
                     account_info.gaia));
}

void StandaloneTrustedVaultBackend::StoreKeys(
    const std::string& gaia_id,
    const std::vector<std::vector<uint8_t>>& keys,
    int last_key_version) {
  // Find or create user for |gaid_id|.
  sync_pb::LocalTrustedVaultPerUser* per_user_vault = FindUserVault(gaia_id);
  if (!per_user_vault) {
    per_user_vault = data_.add_user();
    per_user_vault->set_gaia_id(gaia_id);
  }

  // Replace all keys.
  per_user_vault->set_last_vault_key_version(last_key_version);
  per_user_vault->set_keys_are_stale(false);
  per_user_vault->clear_vault_key();
  for (const std::vector<uint8_t>& key : keys) {
    per_user_vault->add_vault_key()->set_key_material(key.data(), key.size());
  }

  WriteToDisk(data_, file_path_);
  MaybeRegisterDevice(gaia_id);
}

void StandaloneTrustedVaultBackend::RemoveAllStoredKeys() {
  base::DeleteFile(file_path_);
  data_.Clear();
  AbandonConnectionRequest();
}

void StandaloneTrustedVaultBackend::SetPrimaryAccount(
    const base::Optional<CoreAccountInfo>& primary_account) {
  if (primary_account == primary_account_) {
    return;
  }
  primary_account_ = primary_account;
  AbandonConnectionRequest();
  if (primary_account_.has_value()) {
    MaybeRegisterDevice(primary_account_->gaia);
  }
}

bool StandaloneTrustedVaultBackend::MarkKeysAsStale(
    const CoreAccountInfo& account_info) {
  sync_pb::LocalTrustedVaultPerUser* per_user_vault =
      FindUserVault(account_info.gaia);
  if (!per_user_vault || per_user_vault->keys_are_stale()) {
    // No keys available for |account_info| or they are already marked as stale.
    return false;
  }

  per_user_vault->set_keys_are_stale(true);
  WriteToDisk(data_, file_path_);
  return true;
}

base::Optional<CoreAccountInfo>
StandaloneTrustedVaultBackend::GetPrimaryAccountForTesting() const {
  return primary_account_;
}

sync_pb::LocalDeviceRegistrationInfo
StandaloneTrustedVaultBackend::GetDeviceRegistrationInfoForTesting(
    const std::string& gaia_id) {
  sync_pb::LocalTrustedVaultPerUser* per_user_vault = FindUserVault(gaia_id);
  if (!per_user_vault) {
    return sync_pb::LocalDeviceRegistrationInfo();
  }
  return per_user_vault->local_device_registration_info();
}

void StandaloneTrustedVaultBackend::MaybeRegisterDevice(
    const std::string& gaia_id) {
  if (!base::FeatureList::IsEnabled(switches::kFollowTrustedVaultKeyRotation)) {
    return;
  }
  if (!primary_account_.has_value() || primary_account_->gaia != gaia_id) {
    // Device registration is supported only for |primary_account_|.
    return;
  }
  sync_pb::LocalTrustedVaultPerUser* per_user_vault = FindUserVault(gaia_id);
  if (!per_user_vault || per_user_vault->vault_key_size() == 0 ||
      per_user_vault->keys_are_stale()) {
    // Fresh vault key is required to register the device.
    return;
  }
  if (per_user_vault->local_device_registration_info().device_registered()) {
    // Device is already registered.
    return;
  }
  // TODO(crbug.com/1102340): add throttling mechanism.

  std::unique_ptr<SecureBoxKeyPair> key_pair;
  if (per_user_vault->has_local_device_registration_info()) {
    key_pair = SecureBoxKeyPair::CreateByPrivateKeyImport(base::as_bytes(
        base::make_span(per_user_vault->local_device_registration_info()
                            .private_key_material())));
    if (!key_pair) {
      // Device key is corrupted.
      // TODO(crbug.com/1102340): consider generation of new key in this case.
      return;
    }
  } else {
    key_pair = SecureBoxKeyPair::GenerateRandom();
    // It's possible that device will be successfully registered, but the client
    // won't persist this state (for example response doesn't reach the client
    // or registration callback is cancelled). To avoid duplicated registrations
    // device key is stored before sending the registration request, so the same
    // key will be used for future registration attempts.
    std::vector<uint8_t> serialized_private_key =
        key_pair->private_key().ExportToBytes();
    per_user_vault->mutable_local_device_registration_info()
        ->set_private_key_material(serialized_private_key.data(),
                                   serialized_private_key.size());
    WriteToDisk(data_, file_path_);
  }

  std::string last_key = per_user_vault->vault_key()
                             .at(per_user_vault->vault_key_size() - 1)
                             .key_material();
  std::vector<uint8_t> last_key_bytes(last_key.begin(), last_key.end());

  // Cancel existing callbacks passed to |connection_| to ensure there is only
  // one ongoing request.
  AbandonConnectionRequest();
  connection_->RegisterDevice(
      *primary_account_, last_key_bytes,
      per_user_vault->last_vault_key_version(), key_pair->public_key(),
      base::BindOnce(&StandaloneTrustedVaultBackend::OnDeviceRegistered,
                     weak_factory_for_connection_.GetWeakPtr(), gaia_id));
}

void StandaloneTrustedVaultBackend::OnDeviceRegistered(
    const std::string& gaia_id,
    TrustedVaultRequestStatus status) {
  // If |primary_account_| was changed meanwhile, this callback must be
  // cancelled.
  DCHECK(primary_account_ && primary_account_->gaia == gaia_id);

  sync_pb::LocalTrustedVaultPerUser* per_user_vault = FindUserVault(gaia_id);
  DCHECK(per_user_vault);

  switch (status) {
    case TrustedVaultRequestStatus::kSuccess:
      per_user_vault->mutable_local_device_registration_info()
          ->set_device_registered(true);
      WriteToDisk(data_, file_path_);
      return;
    case TrustedVaultRequestStatus::kLocalDataObsolete:
      per_user_vault->set_keys_are_stale(true);
      return;
    case TrustedVaultRequestStatus::kOtherError:
      // TODO(crbug.com/1102340): prevent future queries until browser restart?
      // TODO(crbug.com/1102340): introduce throttling mechanism across browser
      // restarts?
      return;
  }
}

void StandaloneTrustedVaultBackend::OnKeysDownloaded(
    const std::string& gaia_id,
    TrustedVaultRequestStatus status,
    const std::vector<std::vector<uint8_t>>& vault_keys,
    int last_vault_key_version) {
  DCHECK(primary_account_ && primary_account_->gaia == gaia_id);
  DCHECK(!ongoing_fetch_keys_callback_.is_null());
  DCHECK(ongoing_fetch_keys_gaia_id_ == gaia_id);

  sync_pb::LocalTrustedVaultPerUser* per_user_vault = FindUserVault(gaia_id);
  DCHECK(per_user_vault);

  switch (status) {
    case TrustedVaultRequestStatus::kSuccess:
      StoreKeys(gaia_id, vault_keys, last_vault_key_version);
      break;
    case TrustedVaultRequestStatus::kLocalDataObsolete: {
      sync_pb::LocalTrustedVaultPerUser* per_user_vault =
          FindUserVault(gaia_id);
      // Either device isn't registered or vault keys are too outdated or
      // corrupted. The only way to go out of this states is to receive new
      // vault keys through external StoreKeys() call. It's safe to mark device
      // as not registered regardless of the cause (device registration will be
      // triggered once new vault keys are available).
      per_user_vault->mutable_local_device_registration_info()
          ->set_device_registered(false);
      WriteToDisk(data_, file_path_);
      break;
    }
    case TrustedVaultRequestStatus::kOtherError:
      // TODO(crbug.com/1102340): prevent future queries until browser restart?
      // TODO(crbug.com/1102340): introduce throttling mechanism across browser
      // restarts?
      break;
  }
  // Regardless of the |status| ongoing fetch keys request should be fulfilled.
  FulfillOngoingFetchKeys();
}

void StandaloneTrustedVaultBackend::AbandonConnectionRequest() {
  weak_factory_for_connection_.InvalidateWeakPtrs();
  FulfillOngoingFetchKeys();
}

void StandaloneTrustedVaultBackend::FulfillOngoingFetchKeys() {
  if (!ongoing_fetch_keys_gaia_id_.has_value()) {
    return;
  }
  DCHECK(!ongoing_fetch_keys_callback_.is_null());

  const sync_pb::LocalTrustedVaultPerUser* per_user_vault =
      FindUserVault(*ongoing_fetch_keys_gaia_id_);

  std::vector<std::vector<uint8_t>> vault_keys;
  if (per_user_vault) {
    for (const sync_pb::LocalTrustedVaultKey& key :
         per_user_vault->vault_key()) {
      const std::string& key_material = key.key_material();
      vault_keys.emplace_back(key_material.begin(), key_material.end());
    }
  }

  std::move(ongoing_fetch_keys_callback_).Run(vault_keys);
  ongoing_fetch_keys_callback_.Reset();
  ongoing_fetch_keys_gaia_id_.reset();
}

sync_pb::LocalTrustedVaultPerUser* StandaloneTrustedVaultBackend::FindUserVault(
    const std::string& gaia_id) {
  for (int i = 0; i < data_.user_size(); ++i) {
    if (data_.user(i).gaia_id() == gaia_id) {
      return data_.mutable_user(i);
    }
  }
  return nullptr;
}

}  // namespace syncer
