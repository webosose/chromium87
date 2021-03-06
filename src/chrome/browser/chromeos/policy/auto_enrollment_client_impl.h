// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_POLICY_AUTO_ENROLLMENT_CLIENT_IMPL_H_
#define CHROME_BROWSER_CHROMEOS_POLICY_AUTO_ENROLLMENT_CLIENT_IMPL_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/scoped_refptr.h"
#include "base/optional.h"
#include "base/time/time.h"
#include "chrome/browser/chromeos/policy/auto_enrollment_client.h"
#include "components/policy/core/common/cloud/cloud_policy_constants.h"
#include "components/policy/core/common/cloud/device_management_service.h"
#include "services/network/public/cpp/network_connection_tracker.h"
#include "third_party/protobuf/src/google/protobuf/repeated_field.h"

class PrefRegistrySimple;
class PrefService;

namespace private_membership {
namespace rlwe {
class PrivateMembershipRlweClient;
class RlwePlaintextId;
}  // namespace rlwe
}  // namespace private_membership

namespace enterprise_management {
class DeviceManagementResponse;
}

namespace policy {

// Construct the private set membership identifier. See
// go/cros-enterprise-psm and go/cros-client-psm for more details.
private_membership::rlwe::RlwePlaintextId ConstructDeviceRlweId(
    const std::string& device_serial_number,
    const std::string& device_rlz_brand_code);

// A class that handles all communications related to private set membership
// protocol with DMServer. Also, upon successful determination, it caches the
// membership state of a given identifier in the local_state PrefService.
// Upon a failed determination it won't allow another membership check.
class PrivateSetMembershipHelper;

// These values are persisted to logs. Entries should not be renumbered and
// numeric values should never be reused.
enum class PrivateSetMembershipStatus {
  kAttempt = 0,
  kSuccessfulDetermination = 1,
  kError = 2,
  kTimeout = 3,
  kMaxValue = kTimeout,
};

// These values are persisted to logs. Entries should not be renumbered and
// numeric values should never be reused.
enum class PrivateSetMembershipHashDanceComparison {
  kEqualResults = 0,
  kDifferentResults = 1,
  kPSMErrorHashDanceSuccess = 2,
  kPSMSuccessHashDanceError = 3,
  kBothError = 4,
  kMaxValue = kBothError,
};

// Interacts with the device management service and determines whether this
// machine should automatically enter the Enterprise Enrollment screen during
// OOBE.
class AutoEnrollmentClientImpl
    : public AutoEnrollmentClient,
      public network::NetworkConnectionTracker::NetworkConnectionObserver {
 public:
  // Subclasses of this class provide an identifier and specify the identifier
  // set for the DeviceAutoEnrollmentRequest,
  class DeviceIdentifierProvider;

  // Subclasses of this class generate the request to download the device state
  // (after determining that there is server-side device state) and parse the
  // response.
  class StateDownloadMessageProcessor;

  class FactoryImpl : public Factory {
   public:
    FactoryImpl();
    ~FactoryImpl() override;

    std::unique_ptr<AutoEnrollmentClient> CreateForFRE(
        const ProgressCallback& progress_callback,
        DeviceManagementService* device_management_service,
        PrefService* local_state,
        scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
        const std::string& server_backed_state_key,
        int power_initial,
        int power_limit) override;

    std::unique_ptr<AutoEnrollmentClient> CreateForInitialEnrollment(
        const ProgressCallback& progress_callback,
        DeviceManagementService* device_management_service,
        PrefService* local_state,
        scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
        const std::string& device_serial_number,
        const std::string& device_brand_code,
        int power_initial,
        int power_limit,
        int power_outdated_server_detect) override;

   private:
    DISALLOW_COPY_AND_ASSIGN(FactoryImpl);
  };

  ~AutoEnrollmentClientImpl() override;

  // Registers preferences in local state.
  static void RegisterPrefs(PrefRegistrySimple* registry);

  void Start() override;
  void Retry() override;
  void CancelAndDeleteSoon() override;
  std::string device_id() const override;
  AutoEnrollmentState state() const override;

  // network::NetworkConnectionTracker::NetworkConnectionObserver:
  void OnConnectionChanged(network::mojom::ConnectionType type) override;

 private:
  typedef bool (AutoEnrollmentClientImpl::*RequestCompletionHandler)(
      policy::DeviceManagementService::Job*,
      DeviceManagementStatus,
      int,
      const enterprise_management::DeviceManagementResponse&);

  AutoEnrollmentClientImpl(
      const ProgressCallback& progress_callback,
      DeviceManagementService* device_management_service,
      PrefService* local_state,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      std::unique_ptr<DeviceIdentifierProvider> device_identifier_provider,
      std::unique_ptr<StateDownloadMessageProcessor>
          state_download_message_processor,
      int power_initial,
      int power_limit,
      base::Optional<int> power_outdated_server_detect,
      std::string uma_suffix,
      std::unique_ptr<PrivateSetMembershipHelper>
          private_set_membership_helper);

  // Tries to load the result of a previous execution of the protocol from
  // local state. Returns true if that decision has been made and is valid.
  bool GetCachedDecision();

  // Kicks protocol processing, restarting the current step if applicable.
  // Returns true if progress has been made, false if the protocol is done.
  bool RetryStep();

  // Retries running private set membership protocol, if the protocol
  // is enabled and it is possible to start. Returns true if the protocol is
  // enabled and progress has been made, false if the protocol is done. Also,
  // that protocol is being started only one time.
  bool PrivateSetMembershipRetryStep();

  // Sets the private set membership RLWE client for testing through
  // |private_set_membership_helper_|, if the protocol is enabled. Also, the
  // |private_set_membership_rlwe_client| has to be non-null.
  void SetPrivateSetMembershipRlweClientForTesting(
      std::unique_ptr<private_membership::rlwe::PrivateMembershipRlweClient>
          private_set_membership_rlwe_client,
      private_membership::rlwe::RlwePlaintextId& psm_rlwe_id);

  // Cleans up and invokes |progress_callback_|.
  void ReportProgress(AutoEnrollmentState state);

  // Calls RetryStep() to make progress or determine that all is done. In the
  // latter case, calls ReportProgress().
  void NextStep();

  // Sends an auto-enrollment check request to the device management service.
  void SendBucketDownloadRequest();

  // Sends a device state download request to the device management service.
  void SendDeviceStateRequest();

  // Runs the response handler for device management requests and calls
  // NextStep().
  void HandleRequestCompletion(
      RequestCompletionHandler handler,
      policy::DeviceManagementService::Job* job,
      DeviceManagementStatus status,
      int net_error,
      const enterprise_management::DeviceManagementResponse& response);

  // Parses the server response to a bucket download request.
  bool OnBucketDownloadRequestCompletion(
      policy::DeviceManagementService::Job* job,
      DeviceManagementStatus status,
      int net_error,
      const enterprise_management::DeviceManagementResponse& response);

  // Parses the server response to a device state request.
  bool OnDeviceStateRequestCompletion(
      policy::DeviceManagementService::Job* job,
      DeviceManagementStatus status,
      int net_error,
      const enterprise_management::DeviceManagementResponse& response);

  // Returns true if the identifier hash provided by
  // |device_identifier_provider_| is contained in |hashes|.
  bool IsIdHashInProtobuf(
      const google::protobuf::RepeatedPtrField<std::string>& hashes);

  // Updates UMA histograms for bucket download timings.
  void UpdateBucketDownloadTimingHistograms();

  // Updates the UMA histogram for successful hash dance.
  void RecordHashDanceSuccessTimeHistogram();

  // Records the UMA histogram comparing results of hash dance and private set
  // membership. This function should be called after PSM and hash dance
  // requests finished.
  void RecordPrivateSetMembershipHashDanceComparison();

  // Callback to invoke when the protocol generates a relevant event. This can
  // be either successful completion or an error that requires external action.
  ProgressCallback progress_callback_;

  // Current state.
  AutoEnrollmentState state_;

  // Whether the hash bucket check succeeded, indicating that the server knows
  // this device and might have keep state for it.
  bool has_server_state_;

  // Whether the download of server-kept device state completed successfully.
  bool device_state_available_;

  // Randomly generated device id for the auto-enrollment requests.
  std::string device_id_;

  // Power-of-2 modulus to try next.
  int current_power_;

  // Power of the maximum power-of-2 modulus that this client will accept from
  // a retry response from the server.
  int power_limit_;

  // If set and the modulus requested by the server is higher than
  // |1<<power_outdated_server_detect|, this client will assume that the server
  // is outdated.
  base::Optional<int> power_outdated_server_detect_;

  // Number of requests for a different modulus received from the server.
  // Used to determine if the server keeps asking for different moduli.
  int modulus_updates_received_;

  // Used to communicate with the device management service.
  DeviceManagementService* device_management_service_;
  std::unique_ptr<DeviceManagementService::Job> request_job_;

  // PrefService where the protocol's results are cached.
  PrefService* local_state_;

  // The loader factory to use to perform the auto enrollment request.
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;

  // Specifies the identifier set and the hash of the device's current
  // identifier.
  std::unique_ptr<DeviceIdentifierProvider> device_identifier_provider_;

  // Fills and parses state retrieval request / response.
  std::unique_ptr<StateDownloadMessageProcessor>
      state_download_message_processor_;

  // Obtains the device state using private set membership protocol.
  std::unique_ptr<PrivateSetMembershipHelper> private_set_membership_helper_;

  // Times used to determine the duration of the protocol, and the extra time
  // needed to complete after the signin was complete.
  // If |time_start_| is not null, the protocol is still running.
  // If |time_extra_start_| is not null, the protocol is still running but our
  // owner has relinquished ownership.
  base::TimeTicks time_start_;
  base::TimeTicks time_extra_start_;

  // The time when the bucket download part of the protocol started.
  base::TimeTicks time_start_bucket_download_;

  // The UMA histogram suffix. Will be ".ForcedReenrollment" for an
  // |AutoEnrollmentClient| used for FRE and ".InitialEnrollment" for an
  // |AutoEnrollmentclient| used for initial enrollment.
  const std::string uma_suffix_;

  // Whether this instance already recorded the comparison of private set
  // membership and hash dance. This is required because we do not want to
  // record the result again on a hash dance retry.
  bool recorded_psm_hash_dance_comparison_ = false;

  DISALLOW_COPY_AND_ASSIGN(AutoEnrollmentClientImpl);
};

}  // namespace policy

#endif  // CHROME_BROWSER_CHROMEOS_POLICY_AUTO_ENROLLMENT_CLIENT_IMPL_H_
