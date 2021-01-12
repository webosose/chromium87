// Copyright 2016-2019 LG Electronics, Inc.
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

#include "neva/app_runtime/renderer/app_runtime_content_renderer_client.h"

#include "base/command_line.h"
#include "base/strings/string_number_conversions.h"
#include "components/watchdog/switches.h"
#include "content/public/renderer/render_thread.h"
#include "neva/app_runtime/grit/app_runtime_network_error_resources.h"
#include "neva/app_runtime/renderer/app_runtime_localized_error.h"
#include "neva/app_runtime/renderer/app_runtime_page_load_timing_render_frame_observer.h"
#include "neva/app_runtime/renderer/app_runtime_render_frame_observer.h"
#include "third_party/blink/public/mojom/fetch/fetch_api_request.mojom.h"
#include "third_party/blink/public/platform/web_url_error.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/webui/jstemplate_builder.h"

#if defined(USE_NEVA_MEDIA)
#include "components/cdm/renderer/neva/key_systems_util.h"
#include "media/neva/media_preferences.h"
#endif

#if defined(USE_NEVA_EXTENSIONS)
#include "content/public/common/content_constants.h"
#include "extensions/renderer/dispatcher.h"
#include "extensions/renderer/extension_frame_helper.h"
#include "extensions/renderer/guest_view/extensions_guest_view_container.h"
#include "extensions/renderer/guest_view/extensions_guest_view_container_dispatcher.h"
#include "extensions/renderer/guest_view/mime_handler_view/mime_handler_view_container.h"
#include "extensions/shell/common/shell_extensions_client.h"
#include "extensions/shell/renderer/shell_extensions_renderer_client.h"

namespace neva_app_runtime {

class AppRuntimeExtensionsRendererClient
    : public extensions::ShellExtensionsRendererClient {
 public:
  AppRuntimeExtensionsRendererClient() = default;
  ~AppRuntimeExtensionsRendererClient() override = default;
  AppRuntimeExtensionsRendererClient(
      const AppRuntimeExtensionsRendererClient&) = delete;
  AppRuntimeExtensionsRendererClient& operator=(
      const AppRuntimeExtensionsRendererClient&) = delete;

  void OnExtensionLoaded(const extensions::Extension& extension) override;
};

}  // namespace neva_app_runtime
#endif

using blink::mojom::FetchCacheMode;

namespace neva_app_runtime {

class AppRuntimeContentRendererClient::AppRuntimeRenderThreadObserver
    : public content::RenderThreadObserver {
 public:
  AppRuntimeRenderThreadObserver(
      content::RenderFrame* render_frame,
      AppRuntimeContentRendererClient* renderer_client)
      : render_frame_(render_frame), renderer_client_(renderer_client) {
    content::RenderThread::Get()->AddObserver(this);
  }

  ~AppRuntimeRenderThreadObserver() override {
    content::RenderThread::Get()->RemoveObserver(this);
  }

  void NetworkStateChanged(bool online) override {
    if (!render_frame_ || !render_frame_->GetWebFrame() || !renderer_client_)
      return;
    if (online) {
      render_frame_->GetWebFrame()->StartReload(
          blink::WebFrameLoadType::kReload);
      renderer_client_->OnNetworkAppear();
    }
  }

 private:
  content::RenderFrame* render_frame_ = nullptr;
  AppRuntimeContentRendererClient* renderer_client_;
};

AppRuntimeContentRendererClient::AppRuntimeContentRendererClient() {}

AppRuntimeContentRendererClient::~AppRuntimeContentRendererClient() {}

void AppRuntimeContentRendererClient::RenderFrameCreated(
    content::RenderFrame* render_frame) {
#if defined(USE_NEVA_EXTENSIONS)
  // ExtensionFrameHelper destroys itself when the RenderFrame is destroyed.
  new extensions::ExtensionFrameHelper(
      render_frame, extensions_renderer_client_->GetDispatcher());

  extensions_renderer_client_->GetDispatcher()->OnRenderFrameCreated(
      render_frame);
#endif
  // AppRuntimeRenderFrameObserver destroys itself when the RenderFrame is
  // destroyed.
  new AppRuntimeRenderFrameObserver(render_frame);
  // Only attach AppRuntimePageLoadTimingRenderFrameObserver to the main frame,
  // since we only want to observe page load timing for the main frame.
  if (render_frame->IsMainFrame()) {
    new AppRuntimePageLoadTimingRenderFrameObserver(render_frame);
  }
}

void AppRuntimeContentRendererClient::PrepareErrorPage(
    content::RenderFrame* render_frame,
    const blink::WebURLError& error,
    const std::string& http_method,
    std::string* error_html) {
  if (error_html) {
    error_html->clear();

    // Resource will change to net error specific page
    int resource_id = IDR_APP_RUNTIME_NETWORK_ERROR_PAGE;
    const std::string template_html =
        ui::ResourceBundle::GetSharedInstance().LoadDataResourceString(
            resource_id);
    if (template_html.empty()) {
      LOG(ERROR) << "unable to load template.";
    } else {
      base::DictionaryValue error_strings;
      AppRuntimeLocalizedError::GetStrings(error.reason(), error_strings);
      // "t" is the id of the template's root node.
      *error_html = webui::GetTemplatesHtml(template_html,
          &error_strings, "t");
    }

    render_thread_observer_ =
        std::make_unique<AppRuntimeRenderThreadObserver>(render_frame, this);
  }
}

void AppRuntimeContentRendererClient::OnNetworkAppear() {
  render_thread_observer_ = nullptr;
}

#if defined(USE_NEVA_MEDIA)
void AppRuntimeContentRendererClient::AddSupportedKeySystems(
    std::vector<std::unique_ptr<media::KeySystemProperties>>* key_systems) {
  if (key_systems)
    cdm::AddSupportedKeySystems(*key_systems);
}

bool AppRuntimeContentRendererClient::IsSupportedAudioType(
    const media::AudioType& type) {
  return media::MediaPreferences::Get()->IsSupportedAudioType(type);
}

bool AppRuntimeContentRendererClient::IsSupportedVideoType(
    const media::VideoType& type) {
  return media::MediaPreferences::Get()->IsSupportedVideoType(type);
}
#endif

#if defined(USE_NEVA_EXTENSIONS)
void AppRuntimeExtensionsRendererClient::OnExtensionLoaded(
    const extensions::Extension& extension) {
  URLPattern pattern(URLPattern::SCHEME_FILE);
  pattern.SetMatchAllURLs(true);
  const_cast<extensions::Extension*>(&extension)->AddWebExtentPattern(pattern);
}
#endif  // defined(USE_NEVA_EXTENSIONS)

void AppRuntimeContentRendererClient::RenderThreadStarted() {
  content::RenderThread* thread = content::RenderThread::Get();

#if defined(USE_NEVA_EXTENSIONS)
  extensions_client_.reset(new ShellExtensionsClient);
  extensions::ExtensionsClient::Set(extensions_client_.get());

  extensions_renderer_client_.reset(new AppRuntimeExtensionsRendererClient);
  extensions::ExtensionsRendererClient::Set(extensions_renderer_client_.get());
  thread->AddObserver(extensions_renderer_client_->GetDispatcher());

  guest_view_container_dispatcher_.reset(
      new extensions::ExtensionsGuestViewContainerDispatcher());
  thread->AddObserver(guest_view_container_dispatcher_.get());
#endif  // defined(USE_NEVA_EXTENSIONS)

  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch(watchdog::switches::kEnableWatchdog)) {
    watchdog_.reset(new watchdog::Watchdog());

    std::string env_timeout = command_line->GetSwitchValueASCII(
        watchdog::switches::kWatchdogRendererTimeout);
    if (!env_timeout.empty()) {
      int timeout;
      if (base::StringToInt(env_timeout, &timeout))
        watchdog_->SetTimeout(timeout);
    }

    std::string env_period = command_line->GetSwitchValueASCII(
        watchdog::switches::kWatchdogRendererPeriod);
    if (!env_period.empty()) {
      int period;
      if (base::StringToInt(env_period, &period))
        watchdog_->SetPeriod(period);
    }

    watchdog_->StartWatchdog();

    // Check it's currently running on RenderThread.
    CHECK(thread);
    scoped_refptr<base::SingleThreadTaskRunner> task_runner =
        base::ThreadTaskRunnerHandle::Get();
    task_runner->PostTask(
        FROM_HERE, base::Bind(&AppRuntimeContentRendererClient::ArmWatchdog,
                              base::Unretained(this)));
  }
}

void AppRuntimeContentRendererClient::ArmWatchdog() {
  watchdog_->Arm();
  if (!watchdog_->HasThreadInfo())
    watchdog_->SetCurrentThreadInfo();

  // Check it's currently running on RenderThread.
  CHECK(content::RenderThread::Get());
  scoped_refptr<base::SingleThreadTaskRunner> task_runner =
      base::ThreadTaskRunnerHandle::Get();
  task_runner->PostDelayedTask(
      FROM_HERE,
      base::Bind(&AppRuntimeContentRendererClient::ArmWatchdog,
                 base::Unretained(this)),
      base::TimeDelta::FromSeconds(watchdog_->GetPeriod()));
}

#if defined(USE_NEVA_EXTENSIONS)
void AppRuntimeContentRendererClient::RunScriptsAtDocumentStart(
    content::RenderFrame* render_frame) {
  extensions_renderer_client_->GetDispatcher()->RunScriptsAtDocumentStart(
      render_frame);
}

void AppRuntimeContentRendererClient::RunScriptsAtDocumentEnd(
    content::RenderFrame* render_frame) {
  extensions_renderer_client_->GetDispatcher()->RunScriptsAtDocumentEnd(
      render_frame);
}
#endif  // defined(USE_NEVA_EXTENSIONS)

}  // namespace neva_app_runtime
