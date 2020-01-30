// Copyright 2016-2020 LG Electronics, Inc.
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

#include "neva/app_runtime/app/app_runtime_main_delegate.h"

#include "base/base_paths.h"
#include "base/command_line.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "components/viz/common/switches.h"
#include "content/public/common/content_switches.h"
#include "neva/app_runtime/browser/app_runtime_content_browser_client.h"
#include "neva/app_runtime/browser/app_runtime_file_access_delegate.h"
#include "neva/app_runtime/browser/app_runtime_quota_permission_delegate.h"
#include "neva/app_runtime/renderer/app_runtime_content_renderer_client.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/ui_base_paths.h"

namespace {

bool SubprocessNeedsResourceBundle(const std::string& process_type) {
  return process_type == switches::kZygoteProcess ||
         process_type == switches::kPpapiPluginProcess ||
         process_type == switches::kPpapiBrokerProcess ||
         process_type == switches::kGpuProcess ||
         process_type == switches::kRendererProcess ||
         process_type == switches::kUtilityProcess;
}

static neva_app_runtime::AppRuntimeQuotaPermissionDelegate*
    g_quota_permission_delegate = nullptr;
static neva_app_runtime::AppRuntimeFileAccessDelegate* g_file_access_delegate =
    nullptr;

struct BrowserClientTraits
    : public base::internal::DestructorAtExitLazyInstanceTraits<
          neva_app_runtime::AppRuntimeContentBrowserClient> {
  static neva_app_runtime::AppRuntimeContentBrowserClient* New(void* instance) {
    return new neva_app_runtime::AppRuntimeContentBrowserClient(
        g_quota_permission_delegate, g_file_access_delegate);
  }
};

base::LazyInstance<
    neva_app_runtime::AppRuntimeContentBrowserClient, BrowserClientTraits>
        g_app_runtime_content_browser_client = LAZY_INSTANCE_INITIALIZER;
base::LazyInstance<neva_app_runtime::AppRuntimeContentRendererClient>::DestructorAtExit
    g_app_runtime_content_renderer_client = LAZY_INSTANCE_INITIALIZER;

const char kLocaleResourcesDirName[] = "neva_locales";
const char kResourcesFileName[] = "app_runtime_content.pak";
}  // namespace

namespace neva_app_runtime {

AppRuntimeContentBrowserClient* GetAppRuntimeContentBrowserClient() {
  return g_app_runtime_content_browser_client.Pointer();
}

void SetQuotaPermissionDelegate(AppRuntimeQuotaPermissionDelegate* p) {
  g_quota_permission_delegate = p;
}

void SetFileAccessDelegate(AppRuntimeFileAccessDelegate* p) {
  g_file_access_delegate = p;
}

AppRuntimeMainDelegate::AppRuntimeMainDelegate() {}

AppRuntimeMainDelegate::~AppRuntimeMainDelegate() {}

void AppRuntimeMainDelegate::PreSandboxStartup() {
  InitializeResourceBundle();
}

void AppRuntimeMainDelegate::ProcessExiting(const std::string& process_type) {
  if (SubprocessNeedsResourceBundle(process_type))
    ui::ResourceBundle::CleanupSharedInstance();
}

void AppRuntimeMainDelegate::PreMainMessageLoopRun() {
}

bool AppRuntimeMainDelegate::BasicStartupComplete(int* /* exit_code */) {
  base::CommandLine::ForCurrentProcess()->AppendSwitchASCII(
      switches::kUseVizFMPWithTimeout, "0");

  return false;
}

void AppRuntimeMainDelegate::InitializeResourceBundle() {
  base::FilePath pak_file;
#if defined(USE_CBE)
  bool r = base::PathService::Get(base::DIR_ASSETS, &pak_file);
#else
  bool r = base::PathService::Get(base::DIR_MODULE, &pak_file);
#endif  // defined(USE_CBE)
  DCHECK(r);
  ui::ResourceBundle::InitSharedInstanceWithPakPath(
      pak_file.Append(FILE_PATH_LITERAL(kResourcesFileName)));

  base::PathService::Override(ui::DIR_LOCALES,
                              pak_file.AppendASCII(kLocaleResourcesDirName));
}

content::ContentBrowserClient*
AppRuntimeMainDelegate::CreateContentBrowserClient() {
  g_app_runtime_content_browser_client.Pointer()->SetBrowserExtraParts(this);
  return g_app_runtime_content_browser_client.Pointer();
}

content::ContentRendererClient*
AppRuntimeMainDelegate::CreateContentRendererClient() {
  return g_app_runtime_content_renderer_client.Pointer();
}

content::ContentClient*
AppRuntimeMainDelegate::CreateContentClient() {
  content_client_ = std::make_unique<AppRuntimeContentClient>();
  return content_client_.get();
}

}  // namespace neva_app_runtime
