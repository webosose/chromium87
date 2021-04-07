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

#include "neva/injection/public/renderer/injection_install.h"

#include "base/check_op.h"
#include "base/logging.h"
#include "neva/injection/public/common/webapi_names.h"

#if defined(OS_WEBOS)
#if defined(USE_GAV)
#include "neva/injection/public/renderer/webosgavplugin_webapi.h"
#endif  // defined(USE_GAV)
#include "neva/injection/public/renderer/webosservicebridge_webapi.h"
#include "neva/injection/public/renderer/webossystem_webapi.h"
#endif  // defined(OS_WEBOS)

#if defined(ENABLE_SAMPLE_WEBAPI)
#include "neva/injection/public/renderer/sample_webapi.h"
#endif  // defined(ENABLE_SAMPLE_WEBAPI)

#if defined(ENABLE_BROWSER_CONTROL_WEBAPI)
#include "neva/injection/public/renderer/browser_control_webapi.h"
#endif  // defined(ENABLE_BROWSER_CONTROL_WEBAPI)

#if defined(ENABLE_MEMORYMANAGER_WEBAPI)
#include "neva/injection/public/renderer/memorymanager_webapi.h"
#endif  // defined(ENABLE_MEMORYMANAGER_WEBAPI)

namespace injections {

bool GetInjectionInstallAPI(const std::string& name, InstallAPI* api) {
  DCHECK(api != nullptr);
#if defined(OS_WEBOS)
  if ((name == webapi::kWebOSSystem) ||
      (name == webapi::kWebOSSystemObsolete)) {
    api->install_func = WebOSSystemWebAPI::Install;
    api->uninstall_func = WebOSSystemWebAPI::Uninstall;
    return true;
  }

  if ((name == webapi::kWebOSServiceBridge) ||
      (name == webapi::kWebOSServiceBridgeObsolete)) {
    api->install_func = WebOSServiceBridgeWebAPI::Install;
    api->uninstall_func = WebOSServiceBridgeWebAPI::Uninstall;
    return true;
  }

#if defined(USE_GAV)
  if (name == webapi::kWebOSGAV) {
    api->install_func = WebOSGAVWebAPI::Install;
    api->uninstall_func = WebOSGAVWebAPI::Uninstall;
    return true;
  }
#endif  // defined(USE_GAV)
#endif  // defined(OS_WEBOS)

#if defined(ENABLE_SAMPLE_WEBAPI)
  if (name == webapi::kSample) {
    api->install_func = SampleWebAPI::Install;
    api->uninstall_func = SampleWebAPI::Uninstall;
    return true;
  }
#endif
#if defined(ENABLE_MEMORYMANAGER_WEBAPI)
  if (name == webapi::kMemoryManager) {
    api->install_func = MemoryManagerWebAPI::Install;
    api->uninstall_func = MemoryManagerWebAPI::Uninstall;
    return true;
  }
#endif
#if defined(ENABLE_BROWSER_CONTROL_WEBAPI)
  if (name == webapi::kBrowserControl) {
    api->install_func = BrowserControlWebAPI::Install;
    api->uninstall_func = BrowserControlWebAPI::Uninstall;
    return true;
  }
#endif
  return false;
}

}  // namespace injections
