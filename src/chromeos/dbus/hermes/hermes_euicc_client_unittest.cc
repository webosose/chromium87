// Copyright (c) 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <deque>

#include "base/run_loop.h"
#include "base/test/task_environment.h"
#include "chromeos/dbus/hermes/hermes_client_test_base.h"
#include "chromeos/dbus/hermes/hermes_euicc_client.h"
#include "chromeos/dbus/hermes/hermes_test_utils.h"
#include "dbus/bus.h"
#include "dbus/mock_bus.h"
#include "dbus/mock_object_proxy.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/cros_system_api/dbus/hermes/dbus-constants.h"

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;

namespace chromeos {

namespace {

const char* kInvalidPath = "/test/invalid/path";
const char* kTestActivationCode = "abc123";
const char* kTestConfirmationCode = "def456";
const char* kTestEuiccPath = "/org/chromium/hermes/Euicc/1";
const char* kTestCarrierProfilePath = "/org/chromium/hermes/Profile/1";

// Matches dbus::MethodCall for UninstallProfile call with given path.
MATCHER_P(MatchUninstallProfileCall, expected_profile_path, "") {
  dbus::MessageReader reader(arg);
  dbus::ObjectPath carrier_profile_path;
  if (arg->GetMember() != hermes::euicc::kUninstallProfile ||
      !reader.PopObjectPath(&carrier_profile_path) ||
      carrier_profile_path != expected_profile_path) {
    *result_listener << "has method_name=" << arg->GetMember()
                     << " carrier_profile_path="
                     << carrier_profile_path.value();
    return false;
  }
  return true;
}

// Matches dbus::MethodCall for InstrallProfileFromActivationCode call with
// given activation code and confirmation code.
MATCHER_P2(MatchInstallFromActivationCodeCall,
           expected_activation_code,
           expected_confirmation_code,
           "") {
  dbus::MessageReader reader(arg);
  std::string activation_code;
  std::string confirmation_code;
  if (arg->GetMember() != hermes::euicc::kInstallProfileFromActivationCode ||
      !reader.PopString(&activation_code) ||
      activation_code != expected_activation_code ||
      !reader.PopString(&confirmation_code) ||
      confirmation_code != expected_confirmation_code) {
    *result_listener << "has method_name=" << arg->GetMember()
                     << " activation_code=" << activation_code
                     << " confirmation_code=" << confirmation_code;
    return false;
  }
  return true;
}

// Matches dbus::MethodCall for InstallPendingProfile call with given profile
// path and confirmation code.
MATCHER_P2(MatchInstallPendingProfileCall,
           expected_profile_path,
           expected_confirmation_code,
           "") {
  dbus::MessageReader reader(arg);
  dbus::ObjectPath carrier_profile_path;
  std::string confirmation_code;
  if (arg->GetMember() != hermes::euicc::kInstallPendingProfile ||
      !reader.PopObjectPath(&carrier_profile_path) ||
      carrier_profile_path != expected_profile_path ||
      !reader.PopString(&confirmation_code) ||
      confirmation_code != expected_confirmation_code) {
    *result_listener << "has method_name=" << arg->GetMember()
                     << " carrier_profile_path=" << carrier_profile_path.value()
                     << " confirmation_code=" << confirmation_code;
    return false;
  }
  return true;
}

void CopyInstallResult(HermesResponseStatus* dest_status,
                       dbus::ObjectPath* dest_path,
                       HermesResponseStatus status,
                       const dbus::ObjectPath* carrier_profile_path) {
  *dest_status = status;
  if (carrier_profile_path) {
    *dest_path = *carrier_profile_path;
  }
}

}  // namespace

class HermesEuiccClientTest : public HermesClientTestBase {
 public:
  HermesEuiccClientTest() = default;
  HermesEuiccClientTest(const HermesEuiccClientTest&) = delete;
  ~HermesEuiccClientTest() override = default;

  void SetUp() override {
    InitMockBus();

    dbus::ObjectPath euicc_path(kTestEuiccPath);
    proxy_ = new dbus::MockObjectProxy(GetMockBus(), hermes::kHermesServiceName,
                                       euicc_path);
    EXPECT_CALL(*GetMockBus(),
                GetObjectProxy(hermes::kHermesServiceName, euicc_path))
        .WillRepeatedly(Return(proxy_.get()));

    HermesEuiccClient::Initialize(GetMockBus());
    client_ = HermesEuiccClient::Get();

    base::RunLoop().RunUntilIdle();
  }

  void TearDown() override { HermesEuiccClient::Shutdown(); }

  HermesEuiccClientTest& operator=(const HermesEuiccClientTest&) = delete;

 protected:
  scoped_refptr<dbus::MockObjectProxy> proxy_;

  HermesEuiccClient* client_;
};

TEST_F(HermesEuiccClientTest, TestInstallProfileFromActivationCode) {
  dbus::ObjectPath test_euicc_path(kTestEuiccPath);
  dbus::ObjectPath test_carrier_path(kTestCarrierProfilePath);
  dbus::MethodCall method_call(
      hermes::kHermesEuiccInterface,
      hermes::euicc::kInstallProfileFromActivationCode);
  method_call.SetSerial(123);
  EXPECT_CALL(*proxy_.get(),
              DoCallMethodWithErrorResponse(
                  MatchInstallFromActivationCodeCall(kTestActivationCode,
                                                     kTestConfirmationCode),
                  _, _))
      .Times(2)
      .WillRepeatedly(Invoke(this, &HermesEuiccClientTest::OnMethodCalled));

  HermesResponseStatus install_status;
  dbus::ObjectPath installed_profile_path(kInvalidPath);

  // Verify that client makes corresponding dbus method call with
  // correct arguments.
  std::unique_ptr<dbus::Response> response(dbus::Response::CreateEmpty());
  dbus::MessageWriter response_writer(response.get());
  response_writer.AppendObjectPath(test_carrier_path);
  AddPendingMethodCallResult(std::move(response), nullptr);
  client_->InstallProfileFromActivationCode(
      test_euicc_path, kTestActivationCode, kTestConfirmationCode,
      base::BindOnce(&CopyInstallResult, &install_status,
                     &installed_profile_path));
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(install_status, HermesResponseStatus::kSuccess);
  EXPECT_EQ(installed_profile_path, test_carrier_path);

  // Verify that error responses are returned properly.
  installed_profile_path = dbus::ObjectPath(kInvalidPath);
  std::unique_ptr<dbus::ErrorResponse> error_response =
      dbus::ErrorResponse::FromMethodCall(
          &method_call, hermes::kErrorInvalidActivationCode, "");
  AddPendingMethodCallResult(nullptr, std::move(error_response));
  client_->InstallProfileFromActivationCode(
      test_euicc_path, kTestActivationCode, kTestConfirmationCode,
      base::BindOnce(&CopyInstallResult, &install_status,
                     &installed_profile_path));
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(install_status, HermesResponseStatus::kErrorInvalidActivationCode);
  EXPECT_EQ(installed_profile_path.value(), kInvalidPath);
}

TEST_F(HermesEuiccClientTest, TestInstallPendingProfile) {
  dbus::ObjectPath test_euicc_path(kTestEuiccPath);
  dbus::ObjectPath test_carrier_path(kTestCarrierProfilePath);
  dbus::MethodCall method_call(hermes::kHermesEuiccInterface,
                               hermes::euicc::kInstallPendingProfile);
  method_call.SetSerial(123);
  EXPECT_CALL(*proxy_.get(), DoCallMethodWithErrorResponse(
                                 MatchInstallPendingProfileCall(
                                     test_carrier_path, kTestConfirmationCode),
                                 _, _))
      .Times(2)
      .WillRepeatedly(Invoke(this, &HermesEuiccClientTest::OnMethodCalled));

  HermesResponseStatus install_status;

  // Verify that client makes corresponding dbus method call with
  // correct arguments.
  std::unique_ptr<dbus::Response> response(dbus::Response::CreateEmpty());
  dbus::MessageWriter response_writer(response.get());
  response_writer.AppendObjectPath(test_carrier_path);
  AddPendingMethodCallResult(std::move(response), nullptr);
  client_->InstallPendingProfile(
      test_euicc_path, test_carrier_path, kTestConfirmationCode,
      base::BindOnce(&hermes_test_utils::CopyHermesStatus, &install_status));
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(install_status, HermesResponseStatus::kSuccess);

  // Verify that error responses are returned properly.
  std::unique_ptr<dbus::ErrorResponse> error_response =
      dbus::ErrorResponse::FromMethodCall(&method_call,
                                          hermes::kErrorInvalidParameter, "");
  AddPendingMethodCallResult(nullptr, std::move(error_response));
  client_->InstallPendingProfile(
      test_euicc_path, test_carrier_path, kTestConfirmationCode,
      base::BindOnce(&hermes_test_utils::CopyHermesStatus, &install_status));
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(install_status, HermesResponseStatus::kErrorInvalidParameter);
}

TEST_F(HermesEuiccClientTest, TestRequestPendingEvents) {
  dbus::ObjectPath test_euicc_path(kTestEuiccPath);
  dbus::MethodCall method_call(hermes::kHermesEuiccInterface,
                               hermes::euicc::kRequestPendingEvents);
  method_call.SetSerial(123);
  EXPECT_CALL(*proxy_.get(), DoCallMethodWithErrorResponse(
                                 hermes_test_utils::MatchMethodName(
                                     hermes::euicc::kRequestPendingEvents),
                                 _, _))
      .Times(2)
      .WillRepeatedly(Invoke(this, &HermesEuiccClientTest::OnMethodCalled));

  HermesResponseStatus status;

  // Verify that client makes corresponding dbus method call with
  // correct arguments.
  AddPendingMethodCallResult(dbus::Response::CreateEmpty(), nullptr);
  client_->RequestPendingEvents(
      test_euicc_path,
      base::BindOnce(&hermes_test_utils::CopyHermesStatus, &status));
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(status, HermesResponseStatus::kSuccess);

  // Verify that error responses are returned properly.
  std::unique_ptr<dbus::ErrorResponse> error_response =
      dbus::ErrorResponse::FromMethodCall(&method_call, hermes::kErrorUnknown,
                                          "");
  AddPendingMethodCallResult(nullptr, std::move(error_response));
  client_->RequestPendingEvents(
      test_euicc_path,
      base::BindOnce(&hermes_test_utils::CopyHermesStatus, &status));
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(status, HermesResponseStatus::kErrorUnknown);
}

TEST_F(HermesEuiccClientTest, TestUninstallProfile) {
  dbus::ObjectPath test_euicc_path(kTestEuiccPath);
  dbus::ObjectPath test_carrier_path(kTestCarrierProfilePath);
  dbus::MethodCall method_call(hermes::kHermesEuiccInterface,
                               hermes::euicc::kRequestPendingEvents);
  method_call.SetSerial(123);
  EXPECT_CALL(*proxy_.get(),
              DoCallMethodWithErrorResponse(
                  MatchUninstallProfileCall(test_carrier_path), _, _))
      .Times(1)
      .WillRepeatedly(Invoke(this, &HermesEuiccClientTest::OnMethodCalled));

  HermesResponseStatus status;

  // Verify that client makes corresponding dbus method call with
  // correct arguments.
  AddPendingMethodCallResult(dbus::Response::CreateEmpty(), nullptr);
  client_->UninstallProfile(
      test_euicc_path, test_carrier_path,
      base::BindOnce(&hermes_test_utils::CopyHermesStatus, &status));
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(status, HermesResponseStatus::kSuccess);
}

}  // namespace chromeos
