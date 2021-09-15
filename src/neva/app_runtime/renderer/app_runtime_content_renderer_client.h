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

#ifndef NEVA_APP_RUNTIME_RENDERER_APP_RUNTIME_CONTENT_RENDERER_CLIENT_H_
#define NEVA_APP_RUNTIME_RENDERER_APP_RUNTIME_CONTENT_RENDERER_CLIENT_H_

#include <memory>

#include "components/watchdog/watchdog.h"
#include "content/public/renderer/content_renderer_client.h"

#if defined(USE_NEVA_EXTENSIONS)
namespace extensions {
class ExtensionsClient;
class ExtensionsGuestViewContainerDispatcher;
class ShellExtensionsRendererClient;
}  // namespace extensions
#endif

namespace neva_app_runtime {

class AppRuntimeRenderThreadObserver;

class AppRuntimeContentRendererClient : public content::ContentRendererClient {
 public:
  AppRuntimeContentRendererClient();
  ~AppRuntimeContentRendererClient() override;
  AppRuntimeContentRendererClient(const AppRuntimeContentRendererClient&) =
      delete;
  AppRuntimeContentRendererClient& operator=(
      const AppRuntimeContentRendererClient&) = delete;

  void RenderFrameCreated(content::RenderFrame* render_frame) override;
  void RenderThreadStarted() override;

  void PrepareErrorPage(content::RenderFrame* render_frame,
                        const blink::WebURLError& error,
                        const std::string& http_method,
                        std::string* error_html) override;

#if defined(USE_NEVA_MEDIA)
  void AddSupportedKeySystems(
      std::vector<std::unique_ptr<media::KeySystemProperties>>* key_systems)
      override;
  bool IsSupportedAudioType(const media::AudioType& type) override;
  bool IsSupportedVideoType(const media::VideoType& type) override;
#endif

#if defined(USE_NEVA_EXTENSIONS)
  void RunScriptsAtDocumentStart(content::RenderFrame* render_frame) override;
  void RunScriptsAtDocumentEnd(content::RenderFrame* render_frame) override;
#endif

  void DestructObserver();

 private:
  void ArmWatchdog();
  std::unique_ptr<AppRuntimeRenderThreadObserver> observer_;
  std::unique_ptr<watchdog::Watchdog> watchdog_;

#if defined(USE_NEVA_EXTENSIONS)
  std::unique_ptr<extensions::ExtensionsClient> extensions_client_;
  std::unique_ptr<extensions::ShellExtensionsRendererClient>
      extensions_renderer_client_;
  std::unique_ptr<extensions::ExtensionsGuestViewContainerDispatcher>
      guest_view_container_dispatcher_;
#endif
};

}  // namespace neva_app_runtime

#endif  // NEVA_APP_RUNTIME_RENDERER_APP_RUNTIME_CONTENT_RENDERER_CLIENT_H_
