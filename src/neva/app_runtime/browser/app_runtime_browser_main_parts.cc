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

#include "neva/app_runtime/browser/app_runtime_browser_main_parts.h"

#include "base/command_line.h"
#include "base/logging.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/task/post_task.h"
#include "components/os_crypt/key_storage_config_linux.h"
#include "components/os_crypt/os_crypt.h"
#include "components/watchdog/switches.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/common/result_codes.h"
#include "net/base/network_change_notifier_factory.h"
#include "neva/app_runtime/browser/app_runtime_browser_context_adapter.h"
#include "neva/app_runtime/browser/app_runtime_browser_main_extra_parts.h"
#include "neva/app_runtime/browser/app_runtime_devtools_manager_delegate.h"
#include "neva/app_runtime/browser/app_runtime_shared_memory_manager.h"
#include "neva/app_runtime/browser/net/app_runtime_network_change_notifier.h"
#include "ui/views/linux_ui/linux_ui.h"
#include "ui/views/widget/desktop_aura/neva/views_delegate_stub.h"

#if defined(ENABLE_PLUGINS)
#include "content/public/browser/plugin_service.h"
#endif

#if defined(OZONE_PLATFORM_WAYLAND_EXTERNAL)
#include "ozone/ui/webui/ozone_webui.h"
#endif  // defined(OZONE_PLATFORM_WAYLAND_EXTERNAL)

#if defined(USE_AURA)
#include "ui/aura/env.h"
#include "ui/base/ime/init/input_method_initializer.h"
#include "ui/display/screen.h"
#include "ui/views/widget/desktop_aura/desktop_screen.h"
#endif

#if defined(USE_OZONE)
#include "ui/ozone/public/ozone_platform.h"
#endif  // defined(USE_OZONE)

#if defined(USE_NEVA_EXTENSIONS)
#include "components/prefs/pref_service.h"
#include "extensions/browser/api/extensions_api_client.h"
#include "extensions/browser/guest_view/extensions_guest_view_manager_delegate.h"
#include "extensions/browser/process_manager_delegate.h"
#include "extensions/shell/browser/shell_browser_context_keyed_service_factories.h"
#include "extensions/shell/browser/shell_extension_system.h"
#include "extensions/shell/browser/shell_extensions_browser_client.h"
#include "extensions/shell/browser/shell_prefs.h"
#include "extensions/shell/common/shell_extensions_client.h"

namespace neva_app_runtime {
class AppRuntimeProcessManagerDelegate
    : public extensions::ProcessManagerDelegate {
 public:
  AppRuntimeProcessManagerDelegate() = default;
  ~AppRuntimeProcessManagerDelegate() override = default;
  AppRuntimeProcessManagerDelegate(const AppRuntimeProcessManagerDelegate&) =
      delete;
  AppRuntimeProcessManagerDelegate& operator=(
      const AppRuntimeProcessManagerDelegate&) = delete;

  bool AreBackgroundPagesAllowedForContext(
      content::BrowserContext* context) const override {
    return false;
  }
  bool IsExtensionBackgroundPageAllowed(
      content::BrowserContext* context,
      const extensions::Extension& extension) const override {
    return false;
  }
  bool DeferCreatingStartupBackgroundHosts(
      content::BrowserContext* context) const override {
    return false;
  }
};

class AppRuntimeExtensionsGuestViewManagerDelegate
    : public extensions::ExtensionsGuestViewManagerDelegate {
 public:
  AppRuntimeExtensionsGuestViewManagerDelegate(content::BrowserContext* context)
      : extensions::ExtensionsGuestViewManagerDelegate(context) {}
  ~AppRuntimeExtensionsGuestViewManagerDelegate() override = default;
  AppRuntimeExtensionsGuestViewManagerDelegate(
      const AppRuntimeExtensionsGuestViewManagerDelegate&) = delete;
  AppRuntimeExtensionsGuestViewManagerDelegate& operator=(
      const AppRuntimeExtensionsGuestViewManagerDelegate&) = delete;

  // ExtensionsGuestViewManagerDelegate implementation
  bool IsGuestAvailableToContext(guest_view::GuestViewBase* guest) override {
    return true;
  }
};

class AppRuntimeExtensionsAPIClient : public extensions::ExtensionsAPIClient {
 public:
  AppRuntimeExtensionsAPIClient() = default;
  ~AppRuntimeExtensionsAPIClient() override = default;
  AppRuntimeExtensionsAPIClient(const AppRuntimeExtensionsAPIClient&) = delete;
  AppRuntimeExtensionsAPIClient& operator=(
      const AppRuntimeExtensionsAPIClient&) = delete;

  // ExtensionsAPIClient implementation.
  std::unique_ptr<guest_view::GuestViewManagerDelegate>
  CreateGuestViewManagerDelegate(
      content::BrowserContext* context) const override {
    return std::make_unique<AppRuntimeExtensionsGuestViewManagerDelegate>(
        context);
  }
};

class AppRuntimeExtensionsBrowserClient
    : public extensions::ShellExtensionsBrowserClient {
 public:
  AppRuntimeExtensionsBrowserClient() {
    process_manager_delegate_.reset(new AppRuntimeProcessManagerDelegate);
  }
  ~AppRuntimeExtensionsBrowserClient() override = default;
  AppRuntimeExtensionsBrowserClient(const AppRuntimeExtensionsBrowserClient&) =
      delete;
  AppRuntimeExtensionsBrowserClient& operator=(
      const AppRuntimeExtensionsBrowserClient&) = delete;
  extensions::ProcessManagerDelegate* GetProcessManagerDelegate()
      const override {
    return process_manager_delegate_.get();
  }

 private:
  std::unique_ptr<AppRuntimeProcessManagerDelegate> process_manager_delegate_;
};

}  // namespace neva_app_runtime
#endif

namespace neva_app_runtime {

namespace {

void InitializeUI() {
#if defined(OZONE_PLATFORM_WAYLAND_EXTERNAL)
  // Initialization of input method factory
  views::LinuxUI::SetInstance(BuildWebUI());
#endif  // defined(OZONE_PLATFORM_WAYLAND_EXTERNAL)
}

bool IsWayland() {
#if defined(USE_OZONE)
  return ui::OzonePlatform::IsWayland();
#else  // defined(USE_OZONE)
  return false;
#endif  // !defined(USE_OZONE)
}

bool IsWaylandExternal() {
#if defined(USE_OZONE)
  return ui::OzonePlatform::IsWaylandExternal();
#else  // defined(USE_OZONE)
  return false;
#endif  // !defined(USE_OZONE)
}

}  // namespace

class AppRuntimeNetworkChangeNotifierFactory
    : public net::NetworkChangeNotifierFactory {
 public:
  // net::NetworkChangeNotifierFactory overrides.
  std::unique_ptr<net::NetworkChangeNotifier> CreateInstance() override {
    return base::WrapUnique(AppRuntimeNetworkChangeNotifier::GetInstance());
  }
};

AppRuntimeBrowserMainParts::AppRuntimeBrowserMainParts() : BrowserMainParts() {}

AppRuntimeBrowserMainParts::~AppRuntimeBrowserMainParts() {}

void AppRuntimeBrowserMainParts::AddParts(
    AppRuntimeBrowserMainExtraParts* parts) {
  app_runtime_extra_parts_.push_back(parts);
}

int AppRuntimeBrowserMainParts::DevToolsPort() const {
  return AppRuntimeDevToolsManagerDelegate::GetHttpHandlerPort();
}

void AppRuntimeBrowserMainParts::EnableDevTools() {
  if (dev_tools_enabled_)
    return;

  AppRuntimeDevToolsManagerDelegate::StartHttpHandler(
    browser_context_adapter_->GetBrowserContext());
  dev_tools_enabled_ = true;
}

void AppRuntimeBrowserMainParts::DisableDevTools() {
  if (!dev_tools_enabled_)
    return;

  AppRuntimeDevToolsManagerDelegate::StopHttpHandler();
  dev_tools_enabled_ = false;
}

int AppRuntimeBrowserMainParts::PreEarlyInitialization() {
  if (IsWaylandExternal())
    InitializeUI();
  else if (!IsWayland())
    // Only for testing. As stub for other platforms.
    ui::InitializeInputMethodForTesting();
  return RESULT_CODE_NORMAL_EXIT;
}

void AppRuntimeBrowserMainParts::ToolkitInitialized() {
  if (IsWaylandExternal())
    views::LinuxUI::instance()->Initialize();

  if (!views::ViewsDelegate::GetInstance())
    views_delegate_ = std::make_unique<views::ViewsDelegateStub>();
}

void AppRuntimeBrowserMainParts::PreMainMessageLoopStart() {
  // Replace the default NetworkChangeNotifierFactory with app runtime
  // implementation. This must be done before BrowserMainLoop calls
  // net::NetworkChangeNotifier::Create() in PostMainMessageLoopStart().
  net::NetworkChangeNotifier::SetFactory(
      new neva_app_runtime::AppRuntimeNetworkChangeNotifierFactory());
}

void AppRuntimeBrowserMainParts::PostMainMessageLoopStart() {
  app_runtime_mem_manager_.reset(new AppRuntimeSharedMemoryManager);
}

void AppRuntimeBrowserMainParts::PreMainMessageLoopRun() {
  browser_context_adapter_.reset(new BrowserContextAdapter("Default"));
  browser_context_adapter_->Initialize();

#if defined(ENABLE_PLUGINS)
  plugin_service_filter_.reset(new AppRuntimePluginServiceFilter);
  content::PluginService::GetInstance()->SetFilter(
      plugin_service_filter_.get());
#endif

#if defined(USE_AURA)
  if (!display::Screen::GetScreen())
    display::Screen::SetScreenInstance(views::CreateDesktopScreen());

  aura::Env::GetInstance();
#endif

  CreateOSCryptConfig();

#if defined(USE_NEVA_EXTENSIONS)
  extensions_client_ = std::make_unique<extensions::ShellExtensionsClient>();
  extensions::ExtensionsClient::Set(extensions_client_.get());

  extensions_browser_client_ =
      std::make_unique<AppRuntimeExtensionsBrowserClient>();

  extensions_browser_client_->SetAPIClientForTest(
      nullptr);  // First, free old ExtensionsAPIClient object
  extensions_browser_client_->SetAPIClientForTest(
      new AppRuntimeExtensionsAPIClient());  // Second, set own one

  extensions::ExtensionsBrowserClient::Set(extensions_browser_client_.get());

  extensions::shell::EnsureBrowserContextKeyedServiceFactoriesBuilt();

  content::BrowserContext* browser_context =
      browser_context_adapter_->GetBrowserContext();

  user_pref_service_ =
      extensions::shell_prefs::CreateUserPrefService(browser_context);

  extensions_browser_client_->InitWithBrowserContext(browser_context,
                                                     user_pref_service_.get());
#endif
  for (auto* extra_part : app_runtime_extra_parts_)
    extra_part->PreMainMessageLoopRun();

  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch(watchdog::switches::kEnableWatchdog)) {
    ui_watchdog_.reset(new watchdog::Watchdog());
    io_watchdog_.reset(new watchdog::Watchdog());

    std::string env_timeout = command_line->GetSwitchValueASCII(
        watchdog::switches::kWatchdogBrowserTimeout);
    if (!env_timeout.empty()) {
      int timeout;
      if (base::StringToInt(env_timeout, &timeout)) {
        ui_watchdog_->SetTimeout(timeout);
        io_watchdog_->SetTimeout(timeout);
      }
    }

    std::string env_period = command_line->GetSwitchValueASCII(
        watchdog::switches::kWatchdogBrowserPeriod);
    if (!env_period.empty()) {
      int period;
      if (base::StringToInt(env_period, &period)) {
        ui_watchdog_->SetPeriod(period);
        io_watchdog_->SetPeriod(period);
      }
    }

    ui_watchdog_->StartWatchdog();
    io_watchdog_->StartWatchdog();

    base::PostTask(FROM_HERE, {BrowserThread::UI},
                   base::Bind(&AppRuntimeBrowserMainParts::ArmWatchdog,
                              base::Unretained(this),
                              content::BrowserThread::UI, ui_watchdog_.get()));

    base::PostTask(FROM_HERE, {BrowserThread::IO},
                   base::Bind(&AppRuntimeBrowserMainParts::ArmWatchdog,
                              base::Unretained(this),
                              content::BrowserThread::IO, io_watchdog_.get()));
  }
}

void AppRuntimeBrowserMainParts::ArmWatchdog(content::BrowserThread::ID thread,
                                             watchdog::Watchdog* watchdog) {
  watchdog->Arm();
  if (!watchdog->HasThreadInfo())
    watchdog->SetCurrentThreadInfo();

  base::PostDelayedTask(FROM_HERE, {thread},
                        base::Bind(&AppRuntimeBrowserMainParts::ArmWatchdog,
                                   base::Unretained(this), thread, watchdog),
                        base::TimeDelta::FromSeconds(watchdog->GetPeriod()));
}

bool AppRuntimeBrowserMainParts::MainMessageLoopRun(int* result_code) {
  base::RunLoop run_loop;
  run_loop.Run();
  return true;
}

void AppRuntimeBrowserMainParts::PostMainMessageLoopRun() {
  DisableDevTools();
#if defined(USE_NEVA_EXTENSIONS)
  user_pref_service_->CommitPendingWrite();
  user_pref_service_.reset();
#endif
}

void AppRuntimeBrowserMainParts::CreateOSCryptConfig() {
  std::unique_ptr<os_crypt::Config> config(new os_crypt::Config());
  // Forward to os_crypt the flag to use a specific password store.
  config->store = "";
  // Forward the product name
  config->product_name = "";
  // OSCrypt may target keyring, which requires calls from the main thread.
  config->main_thread_runner =
      base::CreateSingleThreadTaskRunner({content::BrowserThread::UI});
  // OSCrypt can be disabled in a special settings file.
  config->should_use_preference = false;
  config->user_data_path = base::FilePath();
  OSCrypt::SetConfig(std::move(config));
}

}  // namespace neva_app_runtime
