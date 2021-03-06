// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_FINGERPRINT_SETUP_SCREEN_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_FINGERPRINT_SETUP_SCREEN_H_

#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "chrome/browser/chromeos/login/screen_manager.h"
#include "chrome/browser/chromeos/login/screens/base_screen.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"

#include "services/device/public/mojom/fingerprint.mojom.h"

namespace chromeos {

class FingerprintSetupScreenView;

// Controls fingerprint setup. The screen can be shown during OOBE. It allows
// user to enroll fingerprint on the device.
class FingerprintSetupScreen : public BaseScreen,
                               public device::mojom::FingerprintObserver {
 public:
  enum class Result { DONE, SKIPPED, DO_IT_LATER, NOT_APPLICABLE };

  // This enum is tied directly to a UMA enum defined in
  // //tools/metrics/histograms/enums.xml, and should always reflect it (do not
  // change one without changing the other). Entries should be never modified
  // or deleted. Only additions possible.
  enum class UserAction {
    kSetupDone = 0,
    kSetupSkipped = 1,
    kDoItLater = 2,
    kAddAnotherFinger = 3,
    kShowSensorLocation = 4,
    kMaxValue = kShowSensorLocation
  };

  static std::string GetResultString(Result result);

  using ScreenExitCallback = base::RepeatingCallback<void(Result result)>;
  FingerprintSetupScreen(FingerprintSetupScreenView* view,
                         const ScreenExitCallback& exit_callback);
  ~FingerprintSetupScreen() override;

  static FingerprintSetupScreen* Get(ScreenManager* manager);

  void set_exit_callback_for_testing(const ScreenExitCallback& exit_callback) {
    exit_callback_ = exit_callback;
  }

  const ScreenExitCallback& get_exit_callback_for_testing() {
    return exit_callback_;
  }

  // device::mojom::FingerprintObserver:
  void OnRestarted() override;
  void OnEnrollScanDone(device::mojom::ScanResult scan_result,
                        bool enroll_session_complete,
                        int percent_complete) override;
  void OnAuthScanDone(
      device::mojom::ScanResult scan_result,
      const base::flat_map<std::string, std::vector<std::string>>& matches)
      override;
  void OnSessionFailed() override;

  // BaseScreen:

 protected:
  // BaseScreen:
  bool MaybeSkip(WizardContext* context) override;
  void ShowImpl() override;
  void HideImpl() override;
  void OnUserAction(const std::string& action_id) override;

 private:
  void StartAddingFinger();
  void OnCancelCurrentEnrollSession(bool success);

  mojo::Remote<device::mojom::Fingerprint> fp_service_;
  mojo::Receiver<device::mojom::FingerprintObserver> receiver_{this};
  int enrolled_finger_count_ = 0;
  bool enroll_session_started_ = false;

  FingerprintSetupScreenView* const view_;
  ScreenExitCallback exit_callback_;

  base::WeakPtrFactory<FingerprintSetupScreen> weak_ptr_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(FingerprintSetupScreen);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_FINGERPRINT_SETUP_SCREEN_H_
