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

#include "neva/app_runtime/browser/app_runtime_content_browser_client.h"

#include "base/command_line.h"
#include "base/logging.h"
#include "base/neva/base_switches.h"
#include "base/rand_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/system/sys_info.h"
#include "cc/base/switches_neva.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/public/browser/devtools_manager_delegate.h"
#include "content/public/browser/login_delegate.h"
#include "content/public/browser/network_service_instance.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/common/content_neva_switches.h"
#include "content/public/common/content_switches.h"
#include "neva/app_runtime/browser/app_runtime_browser_main_parts.h"
#include "neva/app_runtime/browser/app_runtime_browser_switches.h"
#include "neva/app_runtime/browser/app_runtime_devtools_manager_delegate.h"
#include "neva/app_runtime/browser/app_runtime_file_access_delegate.h"
#include "neva/app_runtime/browser/app_runtime_quota_permission_context.h"
#include "neva/app_runtime/browser/app_runtime_quota_permission_delegate.h"
#include "neva/app_runtime/browser/app_runtime_web_contents_view_delegate_creator.h"
#include "neva/app_runtime/common/app_runtime_user_agent.h"
#include "neva/app_runtime/public/proxy_settings.h"
#include "neva/app_runtime/webview.h"
#include "sandbox/policy/switches.h"
#include "services/network/public/mojom/network_service.mojom.h"
#include "third_party/blink/public/common/switches.h"
#include "ui/base/ui_base_neva_switches.h"

#if defined(USE_NEVA_EXTENSIONS)
#include "extensions/browser/guest_view/extensions_guest_view_message_filter.h"
#include "neva/app_runtime/browser/app_runtime_browser_context_adapter.h"
#endif

namespace neva_app_runtime {

namespace {
const char kCacheStoreFile[] = "Cache";
const char kCookieStoreFile[] = "Cookies";
const int kDefaultDiskCacheSize = 16 * 1024 * 1024;  // default size is 16MB
}  // namespace

namespace {

bool GetConfiguredValueBySwitchName(const char switch_name[], double* value) {
  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();
  if (!command_line.HasSwitch(switch_name))
    return false;
  if (!base::StringToDouble(command_line.GetSwitchValueASCII(switch_name),
                            value))
    return false;
  return true;
}

// Skews |value| by +/- |percent|.
int64_t RandomizeByPercent(int64_t value, int percent) {
  double random_percent = (base::RandDouble() - 0.5) * percent * 2;
  return value + (value * (random_percent / 100.0));
}

base::Optional<storage::QuotaSettings> GetConfiguredQuotaSettings(
    const base::FilePath& partition_path) {
  int64_t total = base::SysInfo::AmountOfTotalDiskSpace(partition_path);
  const int kRandomizedPercentage = 10;
  const double kShouldRemainAvailableRatio = 0.1;  // 10%
  const double kMustRemainAvailableRatio = 0.01;   // 1%

  storage::QuotaSettings settings;
  double ratio;
  if (!GetConfiguredValueBySwitchName(kQuotaPoolSizeRatio, &ratio))
    return base::Optional<storage::QuotaSettings>();

  settings.pool_size =
      std::min(RandomizeByPercent(total, kRandomizedPercentage),
               static_cast<int64_t>(total * ratio));

  if (!GetConfiguredValueBySwitchName(kPerHostQuotaRatio, &ratio))
    return base::Optional<storage::QuotaSettings>();

  settings.per_host_quota =
      std::min(RandomizeByPercent(total, kRandomizedPercentage),
               static_cast<int64_t>(settings.pool_size * ratio));
  settings.session_only_per_host_quota = settings.per_host_quota;
  settings.should_remain_available =
      static_cast<int64_t>(total * kShouldRemainAvailableRatio);
  settings.must_remain_available =
      static_cast<int64_t>(total * kMustRemainAvailableRatio);
  settings.refresh_interval = base::TimeDelta::Max();

  return base::make_optional<storage::QuotaSettings>(std::move(settings));
}

}  // namespace

AppRuntimeContentBrowserClient::AppRuntimeContentBrowserClient(
    AppRuntimeQuotaPermissionDelegate* quota_permission_delegate,
    AppRuntimeFileAccessDelegate* file_access_delegate)
    : quota_permission_delegate_(quota_permission_delegate),
      file_access_delegate_(file_access_delegate) {}

AppRuntimeContentBrowserClient::~AppRuntimeContentBrowserClient() {}

void AppRuntimeContentBrowserClient::SetBrowserExtraParts(
    AppRuntimeBrowserMainExtraParts* browser_extra_parts) {
  browser_extra_parts_ = browser_extra_parts;
}

std::unique_ptr<content::BrowserMainParts>
AppRuntimeContentBrowserClient::CreateBrowserMainParts(
    const content::MainFunctionParams& parameters) {
  main_parts_ = new AppRuntimeBrowserMainParts();

  if (browser_extra_parts_)
    main_parts_->AddParts(browser_extra_parts_);

  return base::WrapUnique(main_parts_);
}

content::WebContentsViewDelegate*
AppRuntimeContentBrowserClient::GetWebContentsViewDelegate(
    content::WebContents* web_contents) {
  return CreateAppRuntimeWebContentsViewDelegate(web_contents);
}

void AppRuntimeContentBrowserClient::AllowCertificateError(
    content::WebContents* web_contents,
    int cert_error,
    const net::SSLInfo& ssl_info,
    const GURL& request_url,
    bool is_main_frame_request,
    bool strict_enforcement,
    base::OnceCallback<void(CertificateRequestResultType)> callback) {
  // HCAP requirements: For SSL Certificate error, follows the policy settings
  if (web_contents && web_contents->GetDelegate()) {
    WebView* webView = static_cast<WebView*>(web_contents->GetDelegate());
    switch (webView->GetSSLCertErrorPolicy()) {
      case SSL_CERT_ERROR_POLICY_IGNORE:
        std::move(callback).Run(content::CERTIFICATE_REQUEST_RESULT_TYPE_CONTINUE);
        return;
      case SSL_CERT_ERROR_POLICY_DENY:
        std::move(callback).Run(content::CERTIFICATE_REQUEST_RESULT_TYPE_DENY);
        return;
      default:
        break;
    }
  }

  // A certificate error. The user doesn't really have a context for making the
  // right decision, so block the request hard, without adding info bar that
  // provides possibility to show the insecure content.
  std::move(callback).Run(content::CERTIFICATE_REQUEST_RESULT_TYPE_DENY);
}

bool AppRuntimeContentBrowserClient::ShouldEnableStrictSiteIsolation() {
  // TODO(neva): Temporarily disabled until we support site isolation.
  return false;
}

bool AppRuntimeContentBrowserClient::IsFileAccessAllowedForRequest(
    const base::FilePath& path,
    const base::FilePath& absolute_path,
    const base::FilePath& profile_path,
    const network::ResourceRequest& request) {
  if (!file_access_delegate_)
    return base::CommandLine::ForCurrentProcess()->HasSwitch(kAllowFileAccess);

  if (request.process_id != network::ResourceRequest::kBrowserProcessId)
    return true;
  // If ResourceRequest was created by the browser process, then process_id
  // should be kBrowserProcessId and render_frame_id corresponds to the
  // frame_tree_node_id
  content::FrameTreeNode* frame_tree_node =
      content::FrameTreeNode::GloballyFindByID(request.render_frame_id);
  int process_id = frame_tree_node->current_frame_host()->GetProcess()->GetID();
  int route_id = frame_tree_node->current_frame_host()
                     ->GetRenderViewHost()
                     ->GetRoutingID();

  return file_access_delegate_->IsAccessAllowed(path, process_id, route_id,
                                                request.render_frame_id);
}

void AppRuntimeContentBrowserClient::AppendExtraCommandLineSwitches(
    base::CommandLine* command_line,
    int child_process_id) {
  // Append v8 snapshot path if exists
  auto iter = v8_snapshot_pathes_.find(child_process_id);
  if (iter != v8_snapshot_pathes_.end()) {
    command_line->AppendSwitchPath(switches::kV8SnapshotBlobPath,
                                   base::FilePath(iter->second));
    v8_snapshot_pathes_.erase(iter);
  }

  // Append v8 extra flags if exists
  iter = v8_extra_flags_.find(child_process_id);
  if (iter != v8_extra_flags_.end()) {
    std::string js_flags = iter->second;
    // If already has, append it also
    if (command_line->HasSwitch(switches::kJavaScriptFlags)) {
      js_flags.append(" ");
      js_flags.append(
          command_line->GetSwitchValueASCII(switches::kJavaScriptFlags));
    }
    command_line->AppendSwitchASCII(switches::kJavaScriptFlags, js_flags);
    v8_extra_flags_.erase(iter);
  }

  // Append native scroll related flags if native scroll is on by appinfo.json
  auto iter_ns = use_native_scroll_map_.find(child_process_id);
  if (iter_ns != use_native_scroll_map_.end()) {
    bool use_native_scroll = iter_ns->second;
    if (use_native_scroll) {
      // Enables EnableNativeScroll, which is only enabled when there is
      // 'useNativeScroll': true in appinfo.json. If this flag is enabled,
      if (!command_line->HasSwitch(cc::switches::kEnableWebOSNativeScroll))
        command_line->AppendSwitch(cc::switches::kEnableWebOSNativeScroll);

      // Enables SmoothScrolling, which is mandatory to enable
      // CSSOMSmoothScroll.
      if (!command_line->HasSwitch(switches::kEnableSmoothScrolling))
        command_line->AppendSwitch(switches::kEnableSmoothScrolling);

      // Enables PreferCompositingToLCDText. If this flag is enabled, Compositor
      // thread handles scrolling and disable LCD-text(AntiAliasing) in the
      // scroll area.
      // See PaintLayerScrollableArea.cpp::layerNeedsCompositingScrolling()
      if (!command_line->HasSwitch(blink::switches::kEnablePreferCompositingToLCDText))
        command_line->AppendSwitch(blink::switches::kEnablePreferCompositingToLCDText);

      // Sets kCustomMouseWheelGestureScrollDeltaOnWebOSNativeScroll.
      // If this value is provided from command line argument, then propagate
      // the value to render process. If not, initialize this flag as default
      // value.
      static const int kDefaultGestureScrollDistanceOnNativeScroll = 180;
      // We should find in browser's switch value.
      if (base::CommandLine::ForCurrentProcess()->HasSwitch(
              cc::switches::
                  kCustomMouseWheelGestureScrollDeltaOnWebOSNativeScroll)) {
        std::string propagated_value(
            base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
                cc::switches::
                    kCustomMouseWheelGestureScrollDeltaOnWebOSNativeScroll));
        command_line->AppendSwitchASCII(
            cc::switches::
                kCustomMouseWheelGestureScrollDeltaOnWebOSNativeScroll,
            propagated_value);
      } else
        command_line->AppendSwitchASCII(
            cc::switches::
                kCustomMouseWheelGestureScrollDeltaOnWebOSNativeScroll,
            std::to_string(kDefaultGestureScrollDistanceOnNativeScroll));
    }

    use_native_scroll_map_.erase(iter_ns);
  }

  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kUseOzoneWaylandVkb))
    command_line->AppendSwitch(switches::kUseOzoneWaylandVkb);

  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kOzoneWaylandUseXDGShell))
    command_line->AppendSwitch(switches::kOzoneWaylandUseXDGShell);
}

void AppRuntimeContentBrowserClient::SetUseNativeScroll(
    int child_process_id,
    bool use_native_scroll) {
  use_native_scroll_map_.insert(
      std::pair<int, bool>(child_process_id, use_native_scroll));
}

void AppRuntimeContentBrowserClient::AppendExtraWebSocketHeader(
    const std::string& key,
    const std::string& value) {
  if (network_delegate_)
    network_delegate_->SetWebSocketHeader(key, value);
}

content::DevToolsManagerDelegate*
AppRuntimeContentBrowserClient::GetDevToolsManagerDelegate() {
  return new AppRuntimeDevToolsManagerDelegate();
}

void AppRuntimeContentBrowserClient::OverrideWebkitPrefs(
    content::RenderViewHost* render_view_host,
    blink::web_pref::WebPreferences* prefs) {
  if (!render_view_host)
    return;

  RenderViewHostDelegate* delegate = render_view_host->GetDelegate();
  if (delegate)
    delegate->OverrideWebkitPrefs(prefs);
}

scoped_refptr<content::QuotaPermissionContext>
AppRuntimeContentBrowserClient::CreateQuotaPermissionContext() {
  return new AppRuntimeQuotaPermissionContext(quota_permission_delegate_);
}

bool AppRuntimeContentBrowserClient::HasQuotaSettings() const {
  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();
  return command_line.HasSwitch(kQuotaPoolSizeRatio) &&
         command_line.HasSwitch(kPerHostQuotaRatio);
}

void AppRuntimeContentBrowserClient::GetQuotaSettings(
    content::BrowserContext* context,
    content::StoragePartition* partition,
    storage::OptionalQuotaSettingsCallback callback) const {
  base::Optional<storage::QuotaSettings> quota_settings;
  if ((quota_settings = GetConfiguredQuotaSettings(partition->GetPath())) &&
      quota_settings.has_value()) {
    const int64_t kMBytes = 1024 * 1024;
    LOG(INFO) << "QuotaSettings pool_size: "
              << quota_settings->pool_size / kMBytes << "MB"
              << ", shoud_remain_available: "
              << quota_settings->should_remain_available / kMBytes << "MB"
              << ", must_remain_available: "
              << quota_settings->must_remain_available / kMBytes << "MB"
              << ", per_host_quota: "
              << quota_settings->per_host_quota / kMBytes << "MB"
              << ", session_only_per_host_quota: "
              << quota_settings->session_only_per_host_quota / kMBytes << "MB";

    std::move(callback).Run(*quota_settings);
    return;
  }

  LOG(ERROR) << __func__
             << "(), usage of default quota settings instead of configured one";
  storage::GetNominalDynamicSettings(
      partition->GetPath(), context->IsOffTheRecord(),
      storage::GetDefaultDeviceInfoHelper(), std::move(callback));
}

content::GeneratedCodeCacheSettings
AppRuntimeContentBrowserClient::GetGeneratedCodeCacheSettings(
    content::BrowserContext* context) {
  return content::GeneratedCodeCacheSettings(true, 0, context->GetPath());
}

void AppRuntimeContentBrowserClient::GetAdditionalAllowedSchemesForFileSystem(
    std::vector<std::string>* additional_schemes) {
  ContentBrowserClient::GetAdditionalAllowedSchemesForFileSystem(
      additional_schemes);
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kEnableFileAPIDirectoriesAndSystem))
    additional_schemes->push_back(url::kFileScheme);
}

std::unique_ptr<content::LoginDelegate>
AppRuntimeContentBrowserClient::CreateLoginDelegate(
    const net::AuthChallengeInfo& auth_info,
    content::WebContents* web_contents,
    const content::GlobalRequestID& request_id,
    bool is_request_for_main_frame,
    const GURL& url,
    scoped_refptr<net::HttpResponseHeaders> response_headers,
    bool first_auth_attempt,
    LoginAuthRequiredCallback auth_required_callback) {
  if (!auth_required_callback.is_null() && !credentials_.Empty()) {
    content::GetUIThreadTaskRunner({})->PostTask(
        FROM_HERE,
        base::BindOnce(std::move(auth_required_callback), credentials_));
    return std::make_unique<content::LoginDelegate>();
  }
  return nullptr;
}

void AppRuntimeContentBrowserClient::SetV8SnapshotPath(
    int child_process_id,
    const std::string& path) {
  v8_snapshot_pathes_.insert(
      std::make_pair(child_process_id, path));
}

void AppRuntimeContentBrowserClient::SetV8ExtraFlags(int child_process_id,
                                                     const std::string& flags) {
  v8_extra_flags_.insert(std::make_pair(child_process_id, flags));
}

std::string AppRuntimeContentBrowserClient::GetUserAgent() {
  return neva_app_runtime::GetUserAgent();
}

void AppRuntimeContentBrowserClient::ConfigureNetworkContextParams(
    content::BrowserContext* context,
    bool in_memory,
    const base::FilePath& relative_partition_path,
    network::mojom::NetworkContextParams* network_context_params,
    network::mojom::CertVerifierCreationParams* cert_verifier_creation_params) {
  network_context_params->user_agent = GetUserAgent();
  network_context_params->accept_language = "en-us,en";
  int disk_cache_size = kDefaultDiskCacheSize;
  base::CommandLine* cmd_line = base::CommandLine::ForCurrentProcess();
  if (cmd_line->HasSwitch(kDiskCacheSize))
    base::StringToInt(cmd_line->GetSwitchValueASCII(kDiskCacheSize),
                      &disk_cache_size);
  network_context_params->cookie_path =
      context->GetPath().Append(kCookieStoreFile);
  network_context_params->enable_encrypted_cookies = false;
  network_context_params->custom_proxy_config_client_receiver =
      custom_proxy_config_client_.BindNewPipeAndPassReceiver();
  network_context_params->network_delegate_request =
      mojo::MakeRequest(&network_delegate_);
  network_context_params->http_cache_max_size = disk_cache_size;
  network_context_params->http_cache_path =
      context->GetPath().Append(kCacheStoreFile);
}

void AppRuntimeContentBrowserClient::SetProxyServer(
    const ProxySettings& proxy_settings) {
  if (custom_proxy_config_client_) {
    network::mojom::CustomProxyConfigPtr proxy_config =
        network::mojom::CustomProxyConfig::New();

    credentials_ = net::AuthCredentials();
    if (proxy_settings.enabled) {
      credentials_ =
          net::AuthCredentials(base::UTF8ToUTF16(proxy_settings.username),
                               base::UTF8ToUTF16(proxy_settings.password));
      std::string proxy_string = proxy_settings.ip + ":" + proxy_settings.port;
      net::ProxyConfig::ProxyRules proxy_rules;
      proxy_rules.ParseFromString(proxy_string);

      std::string proxy_bypass_list = proxy_settings.bypass_list;
      // Merge given settings bypass list with one from command line.
      if (base::CommandLine::ForCurrentProcess()->HasSwitch(kProxyBypassList)) {
        std::string cmd_line_proxy_bypass_list =
            base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
                kProxyBypassList);
        if (!proxy_bypass_list.empty())
          proxy_bypass_list += ',';
        proxy_bypass_list += cmd_line_proxy_bypass_list;
      }

      if (!proxy_bypass_list.empty())
        proxy_rules.bypass_rules.ParseFromString(proxy_bypass_list);

      proxy_config->rules = proxy_rules;
    }
    custom_proxy_config_client_->OnCustomProxyConfigUpdated(
        std::move(proxy_config));
  }
}

#if defined(USE_NEVA_EXTENSIONS)
void AppRuntimeContentBrowserClient::RenderProcessWillLaunch(
    content::RenderProcessHost* host) {
  using namespace extensions;
  int render_process_id = host->GetID();
  BrowserContext* browser_context =
      main_parts_->GetDefaultBrowserContext()->GetBrowserContext();
  host->AddFilter(
      new ExtensionsGuestViewMessageFilter(render_process_id, browser_context));
}
#endif
}  // namespace neva_app_runtime
