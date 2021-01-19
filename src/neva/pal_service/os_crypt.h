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

#ifndef NEVA_PAL_SERVICE_OS_CRYPT_
#define NEVA_PAL_SERVICE_OS_CRYPT_

#include <memory>

#include "neva/pal_service/os_crypt_delegate.h"
#include "neva/pal_service/public/mojom/os_crypt.mojom.h"

namespace pal {

class OSCryptImpl : public mojom::OSCrypt {
 public:
  OSCryptImpl();
  ~OSCryptImpl() override;

  OSCryptImpl(const OSCryptImpl&) = delete;
  OSCryptImpl& operator=(const OSCryptImpl&) = delete;

  bool IsEncryptionAvailable() const;

  // mojom::OSCrypt
  void EncryptString(
      const std::string& plaintext,
      pal::mojom::OSCrypt::EncryptStringCallback callback) override;
  void DecryptString(
      const std::string& ciphertext,
      pal::mojom::OSCrypt::DecryptStringCallback callback) override;

 private:
  int GenerateTracingId();
  std::unique_ptr<OSCryptDelegate> delegate_;
};

}  // namespace pal

#endif  // NEVA_PAL_SERVICE_PLATFORM_SYSTEM_HANDLER_
