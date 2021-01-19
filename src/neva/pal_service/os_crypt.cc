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

#include "neva/pal_service/os_crypt.h"

#include "base/trace_event/trace_event.h"
#include "neva/pal_service/pal_platform_factory.h"

namespace {
int g_next_trace_id = 1;
}

namespace pal {

OSCryptImpl::OSCryptImpl()
    : delegate_(PlatformFactory::Get()->CreateOSCryptDelegate()) {}

OSCryptImpl::~OSCryptImpl() {}

int OSCryptImpl::GenerateTracingId() {
  static int next_id = 0;
  return next_id++;
}

bool OSCryptImpl::IsEncryptionAvailable() const {
  return delegate_ ? delegate_->IsEncryptionAvailable() : false;
}

void OSCryptImpl::EncryptString(
    const std::string& plaintext,
    pal::mojom::OSCrypt::EncryptStringCallback callback) {
  if (!delegate_) {
    VLOG(3) << __func__ << "failed: no delegate";
    std::move(callback).Run(false, std::string());
  }

  int trace_id = GenerateTracingId();
  TRACE_EVENT_NESTABLE_ASYNC_BEGIN0("pal", "OSCryptImpl::EncryptString",
                                    TRACE_ID_LOCAL(trace_id));
  delegate_->EncryptString(
      plaintext,
      base::BindOnce(
          [](pal::mojom::OSCrypt::EncryptStringCallback callback, int trace_id,
             bool success, const std::string& ciphertext) {
            std::move(callback).Run(success, ciphertext);
            VLOG(3) << __func__ << " using delegate";
            TRACE_EVENT_NESTABLE_ASYNC_END0("pal", "OSCryptImpl::EncryptString",
                                            TRACE_ID_LOCAL(trace_id));
          },
          std::move(callback), trace_id));
}

void OSCryptImpl::DecryptString(
    const std::string& ciphertext,
    pal::mojom::OSCrypt::DecryptStringCallback callback) {
  if (!delegate_) {
    VLOG(3) << __func__ << " failed: no delegate";
    std::move(callback).Run(false, std::string());
  }

  int trace_id = GenerateTracingId();
  TRACE_EVENT_NESTABLE_ASYNC_BEGIN0("pal", "OSCryptImpl::DecryptString",
                                    TRACE_ID_LOCAL(trace_id));
  delegate_->DecryptString(
      ciphertext,
      base::BindOnce(
          [](pal::mojom::OSCrypt::DecryptStringCallback callback, int trace_id,
             bool success, const std::string& plaintext) {
            VLOG(3) << __func__ << " using delegate";
            std::move(callback).Run(success, plaintext);
            TRACE_EVENT_ASYNC_END0("pal", "OSCryptImpl::DecryptString",
                                   TRACE_ID_LOCAL(trace_id));
          },
          std::move(callback), trace_id));
}

}  // namespace pal
