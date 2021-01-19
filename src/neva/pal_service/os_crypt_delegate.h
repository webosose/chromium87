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

#ifndef NEVA_PAL_SERVICE_OS_CRYPT_DELEGATE_H_
#define NEVA_PAL_SERVICE_OS_CRYPT_DELEGATE_H_

#include <string>

#include "base/callback.h"
#include "base/component_export.h"

namespace pal {

class COMPONENT_EXPORT(PAL_SERVICE) OSCryptDelegate {
 public:
  using OperationCallback = base::OnceCallback<void(bool, const std::string&)>;
  virtual ~OSCryptDelegate() {}

  virtual bool IsEncryptionAvailable() const = 0;
  virtual void EncryptString(const std::string& plaintext,
                             OperationCallback callback) = 0;
  virtual void DecryptString(const std::string& ciphertext,
                             OperationCallback callback) = 0;
};

}  // namespace pal

#endif  // NEVA_PAL_SERVICE_OS_CRYPT_DELEGATE_H_
