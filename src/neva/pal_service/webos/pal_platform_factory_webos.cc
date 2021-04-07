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

#include "neva/pal_service/pal_platform_factory.h"

#include <memory>

#include "neva/pal_service/webos/memorymanager_delegate_webos.h"
#include "neva/pal_service/webos/network_error_page_controller_delegate_webos.h"
#include "neva/pal_service/webos/system_servicebridge_delegate_webos.h"

#if defined(USE_WEBOS_STARFISH)
#include "neva/pal_service/webos/platform_system_delegate_starfish.h"
#else
#include "neva/pal_service/webos/platform_system_delegate_webos.h"
#endif

namespace pal {

std::unique_ptr<MemoryManagerDelegate>
PlatformFactory::CreateMemoryManagerDelegate() {
  return std::make_unique<webos::MemoryManagerDelegateWebOS>();
}

std::unique_ptr<SystemServiceBridgeDelegate>
PlatformFactory::CreateSystemServiceBridgeDelegate(
    std::string name,
    std::string appid,
    SystemServiceBridgeDelegate::Response callback) {
  return std::make_unique<webos::SystemServiceBridgeDelegateWebOS>(
      std::move(name), std::move(appid), std::move(callback));
}

std::unique_ptr<PlatformSystemDelegate>
PlatformFactory::CreatePlatformSystemDelegate() {
#if defined(USE_WEBOS_STARFISH)
  return std::make_unique<webos::PlatformSystemDelegateStarfish>();
#else
  return std::make_unique<webos::PlatformSystemDelegateWebOS>();
#endif
}

std::unique_ptr<NetworkErrorPageControllerDelegate>
PlatformFactory::CreateNetworkErrorPageControllerDelegate() {
  return std::make_unique<webos::NetworkErrorPageControllerDelegateWebOS>();
}

}  // namespace pal
