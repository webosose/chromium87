// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_IN_SESSION_AUTH_IN_SESSION_AUTH_DIALOG_CONTROLLER_IMPL_H_
#define ASH_IN_SESSION_AUTH_IN_SESSION_AUTH_DIALOG_CONTROLLER_IMPL_H_

#include <memory>

#include "ash/in_session_auth/in_session_auth_dialog.h"
#include "ash/public/cpp/in_session_auth_dialog_controller.h"
#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"

class AccountId;

namespace ash {

class InSessionAuthDialogClient;

// InSessionAuthDialogControllerImpl persists as long as UI is running.
class InSessionAuthDialogControllerImpl : public InSessionAuthDialogController {
 public:
  InSessionAuthDialogControllerImpl();
  InSessionAuthDialogControllerImpl(const InSessionAuthDialogControllerImpl&) =
      delete;
  InSessionAuthDialogControllerImpl& operator=(
      const InSessionAuthDialogControllerImpl&) = delete;
  ~InSessionAuthDialogControllerImpl() override;

  // InSessionAuthDialogController overrides
  void SetClient(InSessionAuthDialogClient* client) override;
  void ShowAuthenticationDialog(FinishCallback finish_callback) override;
  void DestroyAuthenticationDialog() override;
  void AuthenticateUserWithPasswordOrPin(
      const std::string& password,
      OnAuthenticateCallback callback) override;
  void AuthenticateUserWithFingerprint(
      base::OnceCallback<void(bool, FingerprintState)> callback) override;
  void Cancel() override;

 private:
  bool IsFingerprintAvailable(const AccountId& account_id);
  void OnStartFingerprintAuthSession(AccountId account_id,
                                     uint32_t auth_methods,
                                     bool success);
  void OnPinCanAuthenticate(uint32_t auth_methods, bool pin_auth_available);

  // Callback to execute when auth on ChromeOS side completes.
  void OnAuthenticateComplete(OnAuthenticateCallback callback, bool success);

  void OnFingerprintAuthComplete(
      base::OnceCallback<void(bool, FingerprintState)> views_callback,
      bool success,
      FingerprintState fingerprint_state);

  InSessionAuthDialogClient* client_ = nullptr;

  // Callback to provide result of the entire authentication flow to
  // UserAuthenticationServiceProvider.
  FinishCallback finish_callback_;

  std::unique_ptr<InSessionAuthDialog> dialog_;

  base::WeakPtrFactory<InSessionAuthDialogControllerImpl> weak_factory_{this};
};

}  // namespace ash

#endif  // ASH_IN_SESSION_AUTH_IN_SESSION_AUTH_DIALOG_CONTROLLER_IMPL_H_
