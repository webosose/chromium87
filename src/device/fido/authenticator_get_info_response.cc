// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/fido/authenticator_get_info_response.h"

#include <utility>

#include "components/cbor/values.h"
#include "components/cbor/writer.h"
#include "device/fido/fido_parsing_utils.h"

namespace device {

namespace {

template <typename Container>
cbor::Value::ArrayValue ToArrayValue(const Container& container) {
  cbor::Value::ArrayValue value;
  value.reserve(container.size());
  for (const auto& item : container)
    value.emplace_back(item);
  return value;
}

}  // namespace

AuthenticatorGetInfoResponse::AuthenticatorGetInfoResponse(
    base::flat_set<ProtocolVersion> in_versions,
    base::flat_set<Ctap2Version> in_ctap2_versions,
    base::span<const uint8_t, kAaguidLength> in_aaguid)
    : versions(std::move(in_versions)),
      ctap2_versions(std::move(in_ctap2_versions)),
      aaguid(fido_parsing_utils::Materialize(in_aaguid)) {
  DCHECK_NE(base::Contains(versions, ProtocolVersion::kCtap2),
            ctap2_versions.empty());
}

AuthenticatorGetInfoResponse::AuthenticatorGetInfoResponse(
    AuthenticatorGetInfoResponse&& that) = default;

AuthenticatorGetInfoResponse& AuthenticatorGetInfoResponse::operator=(
    AuthenticatorGetInfoResponse&& other) = default;

AuthenticatorGetInfoResponse::~AuthenticatorGetInfoResponse() = default;

// static
std::vector<uint8_t> AuthenticatorGetInfoResponse::EncodeToCBOR(
    const AuthenticatorGetInfoResponse& response) {
  cbor::Value::ArrayValue version_array;
  for (const auto& version : response.versions) {
    switch (version) {
      case ProtocolVersion::kCtap2:
        for (const auto& ctap2_version : response.ctap2_versions) {
          switch (ctap2_version) {
            case Ctap2Version::kCtap2_0:
              version_array.emplace_back(kCtap2Version);
              break;
            case Ctap2Version::kCtap2_1:
              version_array.emplace_back(kCtap2_1Version);
              break;
          }
        }
        break;
      case ProtocolVersion::kU2f:
        version_array.emplace_back(kU2fVersion);
        break;
      case ProtocolVersion::kUnknown:
        NOTREACHED();
    }
  }
  cbor::Value::MapValue device_info_map;
  device_info_map.emplace(1, std::move(version_array));

  if (response.extensions)
    device_info_map.emplace(2, ToArrayValue(*response.extensions));

  device_info_map.emplace(3, response.aaguid);
  device_info_map.emplace(4, AsCBOR(response.options));

  if (response.max_msg_size) {
    device_info_map.emplace(5,
                            base::strict_cast<int64_t>(*response.max_msg_size));
  }

  if (response.pin_protocols) {
    device_info_map.emplace(6, ToArrayValue(*response.pin_protocols));
  }

  if (response.max_credential_count_in_list) {
    device_info_map.emplace(
        7, base::strict_cast<int64_t>(*response.max_credential_count_in_list));
  }

  if (response.max_credential_id_length) {
    device_info_map.emplace(
        8, base::strict_cast<int64_t>(*response.max_credential_id_length));
  }

  if (!response.algorithms.empty()) {
    std::vector<cbor::Value> algorithms_cbor;
    algorithms_cbor.reserve(response.algorithms.size());
    for (const auto& algorithm : response.algorithms) {
      // Entries are PublicKeyCredentialParameters
      // https://w3c.github.io/webauthn/#dictdef-publickeycredentialparameters
      cbor::Value::MapValue entry;
      entry.emplace("type", "public-key");
      entry.emplace("alg", algorithm);
      algorithms_cbor.emplace_back(cbor::Value(entry));
    }
    device_info_map.emplace(10, std::move(algorithms_cbor));
  }

  auto encoded_bytes =
      cbor::Writer::Write(cbor::Value(std::move(device_info_map)));
  DCHECK(encoded_bytes);
  return *encoded_bytes;
}

}  // namespace device
