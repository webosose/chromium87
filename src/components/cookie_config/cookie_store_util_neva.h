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

#ifndef COMPONENTS_COOKIE_CONFIG_COOKIE_STORE_UTIL_NEVA_H_
#define COMPONENTS_COOKIE_CONFIG_COOKIE_STORE_UTIL_NEVA_H_

#include <memory>

#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "net/extras/sqlite/cookie_crypto_delegate.h"
#include "neva/pal_service/public/mojom/os_crypt.mojom.h"

namespace cookie_config {

class CookieNevaCryptoDelegate : public net::CookieCryptoDelegate {
 public:
  CookieNevaCryptoDelegate();

  void SetOSCrypt(mojo::PendingRemote<pal::mojom::OSCrypt> os_crypt);
  void SetDefaultCryptoDelegate(net::CookieCryptoDelegate*);

  // net::CookieCryptoDelegate
  bool ShouldEncrypt() override;
  bool EncryptString(const std::string& plaintext,
                     std::string* ciphertext) override;
  bool DecryptString(const std::string& ciphertext,
                     std::string* plaintext) override;

 private:
  net::CookieCryptoDelegate* default_delegate_ = nullptr;
  mojo::Remote<pal::mojom::OSCrypt> os_crypt_;
};

}  // namespace cookie_config

#endif  // COMPONENTS_COOKIE_CONFIG_COOKIE_STORE_UTIL_NEVA_H_
