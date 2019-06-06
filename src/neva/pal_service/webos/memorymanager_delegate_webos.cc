// Copyright 2019 LG Electronics, Inc.
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

#include "neva/pal_service/webos/memorymanager_delegate_webos.h"

#include "neva/pal_service/webos/luna/luna_names.h"

namespace pal {
namespace webos {

const char kSubscribeToThresholdChanged[] =
    R"JSON({"category":"/com/webos/memory", "method":"thresholdChanged"})JSON";
const char kGetMemoryStatusMethod[] = "getCurrentMemState";
const char kGetMemoryStatusRequest[] = "{}";
const char kSignalAddMatch[] = "signal/addmatch";
const char kDefaultResponse[] = "{}";

MemoryManagerDelegateWebOS::MemoryManagerDelegateWebOS() {
  using namespace luna;
  Client::Params params;
  params.bus = Bus::Private;
  params.name = luna::GetServiceNameWithRandSuffix(service_name::kChromiumMemory);
  luna_client_ = CreateClient(params);
}

MemoryManagerDelegateWebOS::~MemoryManagerDelegateWebOS() {
}

void MemoryManagerDelegateWebOS::GetMemoryStatus(OnceResponse callback) {
  luna_client_->Call(
      luna::GetServiceURI(luna::service_uri::kMemoryManager,
                          kGetMemoryStatusMethod),
      std::string(kGetMemoryStatusRequest),
      base::BindOnce(&MemoryManagerDelegateWebOS::OnMemoryStatus,
                     base::Unretained(this),
                     std::move(callback)),
      std::string(kDefaultResponse));
}

void MemoryManagerDelegateWebOS::SubscribeToLevelChanged(
    RepeatingResponse callback) {
  subscription_callback_ = std::move(callback);
  if (!subscribed_ && luna_client_->IsInitialized()) {
    subscription_token_ = luna_client_->Subscribe(
        luna::GetServiceURI(luna::service_uri::kPalmBus, kSignalAddMatch),
        std::string(kSubscribeToThresholdChanged),
        base::BindRepeating(&MemoryManagerDelegateWebOS::OnLevelChanged,
                            base::Unretained(this)));
    subscribed_ = true;
  }
}

void MemoryManagerDelegateWebOS::UnsubscribeFromLevelChanged() {
  if (subscribed_) {
    luna_client_->Unsubscribe(subscription_token_);
    subscribed_ = false;
  }
}

bool MemoryManagerDelegateWebOS::IsSubscribed() const {
  return subscribed_;
}

void MemoryManagerDelegateWebOS::OnMemoryStatus(OnceResponse callback,
                                                luna::Client::ResponseStatus,
                                                unsigned,
                                                const std::string& json) {
  // TODO(pikulik): convert to unified structure
  // ...
  std::move(callback).Run(json);
}

void MemoryManagerDelegateWebOS::OnLevelChanged(luna::Client::ResponseStatus,
                                                unsigned,
                                                const std::string& json) {
  // TODO(pikulik): convert to unified structure
  // ...
  if (subscribed_)
    subscription_callback_.Run(json);
}

}  // namespace webos
}  // namespace pal
