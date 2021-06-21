// Copyright 2021 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#include "components/cookie_config/cookie_store_util_neva.h"

#include <string>

namespace cookie_config {

CookieNevaCryptoDelegate::CookieNevaCryptoDelegate() = default;

void CookieNevaCryptoDelegate::SetOSCrypt(
    mojo::PendingRemote<pal::mojom::OSCrypt> os_crypt) {
  VLOG(3) << __func__ << ": attached remote mojo OSCrypt";
  os_crypt_.reset();
  os_crypt_.Bind(std::move(os_crypt));
}

void CookieNevaCryptoDelegate::SetDefaultCryptoDelegate(
    net::CookieCryptoDelegate* default_delegate) {
  default_delegate_ = default_delegate;
}

bool CookieNevaCryptoDelegate::ShouldEncrypt() {
  if (os_crypt_.is_bound() && os_crypt_.is_connected())
    return true;
  return default_delegate_ ? default_delegate_->ShouldEncrypt() : false;
}

bool CookieNevaCryptoDelegate::EncryptString(const std::string& plaintext,
                                             std::string* ciphertext) {
  if (os_crypt_.is_bound() && os_crypt_.is_connected()) {
    bool success = true;
    base::WaitableEvent finished(
        base::WaitableEvent::ResetPolicy::MANUAL,
        base::WaitableEvent::InitialState::NOT_SIGNALED);
    os_crypt_->EncryptString(
        plaintext, base::BindOnce(
                       [](base::WaitableEvent* finished, bool* success,
                          std::string* ciphertext, bool remote_success,
                          const std::string& remote_ciphertext) {
                         *success = remote_success;
                         *ciphertext = remote_ciphertext;
                         finished->Signal();
                       },
                       &finished, &success, ciphertext));
    finished.Wait();
    if (success) {
      VLOG(3) << __func__ << ": used PAL encryption.";
      return true;
    }
  }
  VLOG(3) << __func__
          << ": no PAL encryption. Fallback to default implementation.";
  return default_delegate_
             ? default_delegate_->EncryptString(plaintext, ciphertext)
             : false;
}

bool CookieNevaCryptoDelegate::DecryptString(const std::string& ciphertext,
                                             std::string* plaintext) {
  if (os_crypt_.is_bound() && os_crypt_.is_connected()) {
    bool success = true;
    base::WaitableEvent finished(
        base::WaitableEvent::ResetPolicy::MANUAL,
        base::WaitableEvent::InitialState::NOT_SIGNALED);
    os_crypt_->DecryptString(
        ciphertext, base::BindOnce(
                        [](base::WaitableEvent* finished, bool* success,
                           std::string* plaintext, bool remote_success,
                           const std::string& remote_plaintext) {
                          *success = remote_success;
                          *plaintext = remote_plaintext;
                          finished->Signal();
                        },
                        &finished, &success, plaintext));
    finished.Wait();
    if (success) {
      VLOG(3) << __func__ << ": used PAL encryption.";
      return true;
    }
  }
  VLOG(3) << __func__
          << ": no PAL encryption. Fallback to default implementation.";
  return default_delegate_
             ? default_delegate_->DecryptString(ciphertext, plaintext)
             : false;
}

}  // namespace cookie_config
