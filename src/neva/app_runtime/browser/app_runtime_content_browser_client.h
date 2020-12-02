// Copyright 2016 LG Electronics, Inc.
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

#ifndef NEVA_APP_RUNTIME_BROWSER_APP_RUNTIME_CONTENT_BROWSER_CLIENT_H_
#define NEVA_APP_RUNTIME_BROWSER_APP_RUNTIME_CONTENT_BROWSER_CLIENT_H_

#include "content/public/browser/content_browser_client.h"
#include "neva/app_runtime/browser/app_runtime_browser_main_parts.h"
#include "services/network/public/mojom/network_context.mojom.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "storage/browser/quota/quota_settings.h"

namespace content {
struct GlobalRequestID;
class LoginDelegate;
}  // namespace content

namespace neva_app_runtime {

class AppRuntimeBrowserMainExtraParts;
class AppRuntimeFileAccessDelegate;
class AppRuntimeQuotaPermissionDelegate;
struct ProxySettings;

class AppRuntimeContentBrowserClient : public content::ContentBrowserClient {
 public:
  explicit AppRuntimeContentBrowserClient(
      AppRuntimeQuotaPermissionDelegate* quota_permission_delegate,
      AppRuntimeFileAccessDelegate* file_access_delegate);
  ~AppRuntimeContentBrowserClient() override;

  void SetBrowserExtraParts(
      AppRuntimeBrowserMainExtraParts* browser_extra_parts);

  // content::ContentBrowserClient implementations
  std::unique_ptr<content::BrowserMainParts> CreateBrowserMainParts(
      const content::MainFunctionParams& parameters) override;

  void AllowCertificateError(
      content::WebContents* web_contents,
      int cert_error,
      const net::SSLInfo& ssl_info,
      const GURL& request_url,
      bool is_main_frame_request,
      bool strict_enforcement,
      base::OnceCallback<void(content::CertificateRequestResultType)> callback) override;

  content::WebContentsViewDelegate* GetWebContentsViewDelegate(
      content::WebContents* web_contents) override;

  content::DevToolsManagerDelegate* GetDevToolsManagerDelegate() override;

  bool ShouldEnableStrictSiteIsolation() override;
  bool ShouldIsolateErrorPage(bool is_main_frame) override;

  bool IsFileAccessAllowedFromNetwork() const override;
  bool IsFileAccessAllowedForRequest(
      const base::FilePath& path,
      const base::FilePath& absolute_path,
      const base::FilePath& profile_path,
      const network::ResourceRequest& request) override;

  void AppendExtraCommandLineSwitches(base::CommandLine* command_line,
                                      int child_process_id) override;

  void OverrideWebkitPrefs(content::RenderViewHost* render_view_host,
                           blink::web_pref::WebPreferences* prefs) override;
  scoped_refptr<content::QuotaPermissionContext> CreateQuotaPermissionContext()
      override;

  bool HasQuotaSettings() const override;
  void GetQuotaSettings(
      content::BrowserContext* context,
      content::StoragePartition* partition,
      storage::OptionalQuotaSettingsCallback callback) const override;

  content::GeneratedCodeCacheSettings GetGeneratedCodeCacheSettings(
      content::BrowserContext* context) override;

  void GetAdditionalAllowedSchemesForFileSystem(
      std::vector<std::string>* additional_schemes) override;

  std::unique_ptr<content::LoginDelegate> CreateLoginDelegate(
      const net::AuthChallengeInfo& auth_info,
      content::WebContents* web_contents,
      const content::GlobalRequestID& request_id,
      bool is_main_frame,
      const GURL& url,
      scoped_refptr<net::HttpResponseHeaders> response_headers,
      bool first_auth_attempt,
      LoginAuthRequiredCallback auth_required_callback) override;

  std::string GetUserAgent() override;

  void OnNetworkServiceCreated(
      network::mojom::NetworkService* network_service) override;

  void ConfigureNetworkContextParams(
      content::BrowserContext* context,
      bool in_memory,
      const base::FilePath& relative_partition_path,
      network::mojom::NetworkContextParams* network_context_params,
      network::mojom::CertVerifierCreationParams* cert_verifier_creation_params)
      override;

  AppRuntimeBrowserMainParts* GetMainParts() { return main_parts_; }

  void SetProxyServer(const ProxySettings& proxy_settings);

#if defined(ENABLE_PLUGINS)
  bool PluginLoaded() const { return plugin_loaded_; }
  void SetPluginLoaded(bool loaded) { plugin_loaded_ = loaded; }
#endif
#if defined(USE_NEVA_EXTENSIONS)
  void RenderProcessWillLaunch(content::RenderProcessHost* host) override;
#endif

  void SetV8SnapshotPath(int child_process_id, const std::string& path);
  void SetV8ExtraFlags(int child_process_id, const std::string& flags);
  void SetUseNativeScroll(int child_process_id, bool use_native_scroll);

  void AppendExtraWebSocketHeader(const std::string& key,
                                  const std::string& value);

 private:
  AppRuntimeBrowserMainExtraParts* browser_extra_parts_ = nullptr;
  AppRuntimeBrowserMainParts* main_parts_ = nullptr;

  AppRuntimeQuotaPermissionDelegate* quota_permission_delegate_ = nullptr;
  AppRuntimeFileAccessDelegate* file_access_delegate_ = nullptr;
  mojo::Remote<network::mojom::CustomProxyConfigClient>
      custom_proxy_config_client_;
  network::mojom::ExtraHeaderNetworkDelegatePtr network_delegate_;

#if defined(ENABLE_PLUGINS)
  bool plugin_loaded_ = false;
#endif

  std::map<int, std::string> v8_snapshot_pathes_;
  std::map<int, std::string> v8_extra_flags_;
  net::AuthCredentials credentials_;

  // Stores (int child_process_id, bool use_native_scroll) and apply the flags
  // related to native scroll when use_native_scroll flag for the render process
  // is true.
  std::map<int, bool> use_native_scroll_map_;
};

}  // namespace neva_app_runtime

#endif  // NEVA_APP_RUNTIME_BROWSER_APP_RUNTIME_CONTENT_BROWSER_CLIENT_H_
