// Copyright (c) 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/policy/system_proxy_manager.h"

#include "base/bind.h"
#include "base/strings/string16.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "chrome/browser/chromeos/login/ui/login_display_host.h"
#include "chrome/browser/chromeos/profiles/profile_helper.h"
#include "chrome/browser/chromeos/ui/request_system_proxy_credentials_view.h"
#include "chrome/browser/chromeos/ui/system_proxy_notification.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/common/pref_names.h"
#include "chromeos/dbus/system_proxy/system_proxy_client.h"
#include "chromeos/network/network_event_log.h"
#include "chromeos/settings/cros_settings_names.h"
#include "chromeos/settings/cros_settings_provider.h"
#include "components/arc/arc_prefs.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/user_manager/user.h"
#include "components/user_manager/user_manager.h"
#include "content/public/browser/storage_partition.h"
#include "net/base/host_port_pair.h"
#include "net/base/proxy_server.h"
#include "net/http/http_auth_scheme.h"
#include "net/http/http_util.h"
#include "services/network/public/mojom/network_context.mojom.h"
#include "ui/aura/window.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/views/widget/widget.h"
#include "ui/views/window/dialog_delegate.h"

namespace {
const char kSystemProxyService[] = "system-proxy-service";
}  // namespace

namespace policy {

SystemProxyManager::SystemProxyManager(chromeos::CrosSettings* cros_settings,
                                       PrefService* local_state)
    : cros_settings_(cros_settings),
      system_proxy_subscription_(cros_settings_->AddSettingsObserver(
          chromeos::kSystemProxySettings,
          base::BindRepeating(
              &SystemProxyManager::OnSystemProxySettingsPolicyChanged,
              base::Unretained(this)))) {
  // Connect to System-proxy signals.
  chromeos::SystemProxyClient::Get()->SetWorkerActiveSignalCallback(
      base::BindRepeating(&SystemProxyManager::OnWorkerActive,
                          weak_factory_.GetWeakPtr()));
  chromeos::SystemProxyClient::Get()->SetAuthenticationRequiredSignalCallback(
      base::BindRepeating(&SystemProxyManager::OnAuthenticationRequired,
                          weak_factory_.GetWeakPtr()));
  chromeos::SystemProxyClient::Get()->ConnectToWorkerSignals();
  local_state_ = local_state;

  // Listen to pref changes.
  local_state_pref_change_registrar_ = std::make_unique<PrefChangeRegistrar>();
  local_state_pref_change_registrar_->Init(local_state_);
  local_state_pref_change_registrar_->Add(
      prefs::kKerberosEnabled,
      base::BindRepeating(&SystemProxyManager::OnKerberosEnabledChanged,
                          weak_factory_.GetWeakPtr()));

  // Fire it once so we're sure we get an invocation on startup.
  OnSystemProxySettingsPolicyChanged();
}

SystemProxyManager::~SystemProxyManager() = default;

std::string SystemProxyManager::SystemServicesProxyPacString() const {
  return system_proxy_enabled_ && !system_services_address_.empty()
             ? "PROXY " + system_services_address_
             : std::string();
}

void SystemProxyManager::StartObservingPrimaryProfilePrefs(Profile* profile) {
  primary_profile_ = profile;
  // Listen to pref changes.
  profile_pref_change_registrar_ = std::make_unique<PrefChangeRegistrar>();
  profile_pref_change_registrar_->Init(primary_profile_->GetPrefs());
  profile_pref_change_registrar_->Add(
      prefs::kKerberosActivePrincipalName,
      base::BindRepeating(&SystemProxyManager::OnKerberosAccountChanged,
                          base::Unretained(this)));
  profile_pref_change_registrar_->Add(
      arc::prefs::kArcEnabled,
      base::BindRepeating(&SystemProxyManager::OnArcEnabledChanged,
                          weak_factory_.GetWeakPtr()));
  if (system_proxy_enabled_) {
    OnKerberosAccountChanged();
    OnArcEnabledChanged();
  }
}

void SystemProxyManager::StopObservingPrimaryProfilePrefs() {
  profile_pref_change_registrar_->RemoveAll();
  profile_pref_change_registrar_.reset();
  primary_profile_ = nullptr;
}

void SystemProxyManager::ClearUserCredentials() {
  if (!system_proxy_enabled_) {
    return;
  }

  system_proxy::ClearUserCredentialsRequest request;
  chromeos::SystemProxyClient::Get()->ClearUserCredentials(
      request, base::BindOnce(&SystemProxyManager::OnClearUserCredentials,
                              weak_factory_.GetWeakPtr()));
}

void SystemProxyManager::OnSystemProxySettingsPolicyChanged() {
  chromeos::CrosSettingsProvider::TrustedStatus status =
      cros_settings_->PrepareTrustedValues(base::BindOnce(
          &SystemProxyManager::OnSystemProxySettingsPolicyChanged,
          base::Unretained(this)));
  if (status != chromeos::CrosSettingsProvider::TRUSTED)
    return;

  const base::Value* proxy_settings =
      cros_settings_->GetPref(chromeos::kSystemProxySettings);

  if (!proxy_settings)
    return;

  system_proxy_enabled_ =
      proxy_settings->FindBoolKey(chromeos::kSystemProxySettingsKeyEnabled)
          .value_or(false);
  // System-proxy is inactive by default.
  if (!system_proxy_enabled_) {
    // Send a shut-down command to the daemon. Since System-proxy is started via
    // dbus activation, if the daemon is inactive, this command will start the
    // daemon and tell it to exit.
    // TODO(crbug.com/1055245,acostinas): Do not send shut-down command if
    // System-proxy is inactive.
    system_proxy::ShutDownRequest request;
    request.set_traffic_type(system_proxy::TrafficOrigin::ALL);
    chromeos::SystemProxyClient::Get()->ShutDownProcess(
        request, base::BindOnce(&SystemProxyManager::OnShutDownProcess,
                                weak_factory_.GetWeakPtr()));
    system_services_address_.clear();
    SetUserTrafficProxyPref(std::string());
    CloseAuthenticationUI();
    return;
  }
  system_proxy::SetAuthenticationDetailsRequest request;
  system_proxy::Credentials credentials;
  const std::string* username = proxy_settings->FindStringKey(
      chromeos::kSystemProxySettingsKeySystemServicesUsername);

  const std::string* password = proxy_settings->FindStringKey(
      chromeos::kSystemProxySettingsKeySystemServicesPassword);

  if (!username || username->empty() || !password || password->empty()) {
    NET_LOG(DEBUG) << "Proxy credentials for system traffic not set: "
                   << kSystemProxyService;
  } else {
    credentials.set_username(*username);
    credentials.set_password(*password);
    *request.mutable_credentials() = credentials;
  }

  request.set_traffic_type(system_proxy::TrafficOrigin::SYSTEM);

  chromeos::SystemProxyClient::Get()->SetAuthenticationDetails(
      request, base::BindOnce(&SystemProxyManager::OnSetAuthenticationDetails,
                              weak_factory_.GetWeakPtr()));
  // Fire once to cover the case where the SystemProxySetting policy is set
  // during a user session.
  if (IsArcEnabled()) {
    OnArcEnabledChanged();
  }
}

void SystemProxyManager::OnKerberosEnabledChanged() {
  SendKerberosAuthenticationDetails();
}

void SystemProxyManager::OnKerberosAccountChanged() {
  if (!local_state_->GetBoolean(prefs::kKerberosEnabled)) {
    return;
  }
  SendKerberosAuthenticationDetails();
}

void SystemProxyManager::OnArcEnabledChanged() {
  if (!system_proxy_enabled_) {
    return;
  }

  if (!IsArcEnabled()) {
    system_proxy::ShutDownRequest request;
    request.set_traffic_type(system_proxy::TrafficOrigin::USER);
    chromeos::SystemProxyClient::Get()->ShutDownProcess(
        request, base::BindOnce(&SystemProxyManager::OnShutDownProcess,
                                weak_factory_.GetWeakPtr()));
    return;
  }

  system_proxy::SetAuthenticationDetailsRequest request;
  request.set_traffic_type(system_proxy::TrafficOrigin::USER);
  chromeos::SystemProxyClient::Get()->SetAuthenticationDetails(
      request, base::BindOnce(&SystemProxyManager::OnSetAuthenticationDetails,
                              weak_factory_.GetWeakPtr()));
}

bool SystemProxyManager::IsArcEnabled() const {
  return primary_profile_ &&
         primary_profile_->GetPrefs()->GetBoolean(arc::prefs::kArcEnabled);
}

void SystemProxyManager::SendUserAuthenticationCredentials(
    const system_proxy::ProtectionSpace& protection_space,
    const std::string& username,
    const std::string& password) {
  // System-proxy is started via d-bus activation, meaning the first d-bus call
  // will start the daemon. Check that System-proxy was not disabled by policy
  // while looking for credentials so we don't accidentally restart it.
  if (!system_proxy_enabled_) {
    return;
  }

  system_proxy::Credentials user_credentials;
  user_credentials.set_username(username);
  user_credentials.set_password(password);

  system_proxy::SetAuthenticationDetailsRequest request;
  request.set_traffic_type(system_proxy::TrafficOrigin::ALL);
  *request.mutable_credentials() = user_credentials;
  *request.mutable_protection_space() = protection_space;

  chromeos::SystemProxyClient::Get()->SetAuthenticationDetails(
      request, base::BindOnce(&SystemProxyManager::OnSetAuthenticationDetails,
                              weak_factory_.GetWeakPtr()));
}

void SystemProxyManager::SendKerberosAuthenticationDetails() {
  if (!system_proxy_enabled_) {
    return;
  }

  system_proxy::SetAuthenticationDetailsRequest request;
  request.set_traffic_type(system_proxy::TrafficOrigin::SYSTEM);
  request.set_kerberos_enabled(
      local_state_->GetBoolean(prefs::kKerberosEnabled));
  if (primary_profile_) {
    request.set_active_principal_name(
        primary_profile_->GetPrefs()
            ->Get(prefs::kKerberosActivePrincipalName)
            ->GetString());
  }
  chromeos::SystemProxyClient::Get()->SetAuthenticationDetails(
      request, base::BindOnce(&SystemProxyManager::OnSetAuthenticationDetails,
                              weak_factory_.GetWeakPtr()));
}

void SystemProxyManager::SendEmptyCredentials(
    const system_proxy::ProtectionSpace& protection_space) {
  SendUserAuthenticationCredentials(protection_space,
                                    /*username=*/std::string(),
                                    /*password=*/std::string());
}

void SystemProxyManager::SetSystemProxyEnabledForTest(bool enabled) {
  system_proxy_enabled_ = enabled;
}

void SystemProxyManager::SetSystemServicesProxyUrlForTest(
    const std::string& local_proxy_url) {
  system_proxy_enabled_ = true;
  system_services_address_ = local_proxy_url;
}

void SystemProxyManager::SetSendAuthDetailsClosureForTest(
    base::RepeatingClosure closure) {
  send_auth_details_closure_for_test_ = closure;
}

chromeos::RequestSystemProxyCredentialsView*
SystemProxyManager::GetActiveAuthDialogForTest() {
  return active_auth_dialog_;
}

void SystemProxyManager::CloseAuthDialogForTest() {
  DCHECK(auth_widget_);
  auth_widget_->CloseNow();
}

// static
void SystemProxyManager::RegisterProfilePrefs(PrefRegistrySimple* registry) {
  registry->RegisterStringPref(prefs::kSystemProxyUserTrafficHostAndPort,
                               /*default_value=*/std::string());
}

void SystemProxyManager::OnSetAuthenticationDetails(
    const system_proxy::SetAuthenticationDetailsResponse& response) {
  if (response.has_error_message()) {
    NET_LOG(ERROR)
        << "Failed to set system traffic credentials for system proxy: "
        << kSystemProxyService << ", Error: " << response.error_message();
  }
  if (send_auth_details_closure_for_test_)
    send_auth_details_closure_for_test_.Run();
}

void SystemProxyManager::OnShutDownProcess(
    const system_proxy::ShutDownResponse& response) {
  if (response.has_error_message() && !response.error_message().empty()) {
    NET_LOG(ERROR) << "Failed to shutdown system proxy process: "
                   << kSystemProxyService
                   << ", error: " << response.error_message();
  }
}

void SystemProxyManager::OnClearUserCredentials(
    const system_proxy::ClearUserCredentialsResponse& response) {
  if (response.has_error_message() && !response.error_message().empty()) {
    NET_LOG(ERROR) << "Failed to clear user credentials: "
                   << kSystemProxyService
                   << ", error: " << response.error_message();
  }
}

void SystemProxyManager::OnWorkerActive(
    const system_proxy::WorkerActiveSignalDetails& details) {
  if (details.traffic_origin() == system_proxy::TrafficOrigin::SYSTEM) {
    system_services_address_ = details.local_proxy_url();
    return;
  }
  SetUserTrafficProxyPref(details.local_proxy_url());
}

void SystemProxyManager::SetUserTrafficProxyPref(
    const std::string& user_traffic_address) {
  if (!primary_profile_) {
    return;
  }
  primary_profile_->GetPrefs()->SetString(
      prefs::kSystemProxyUserTrafficHostAndPort, user_traffic_address);
}

void SystemProxyManager::OnAuthenticationRequired(
    const system_proxy::AuthenticationRequiredDetails& details) {
  system_proxy::ProtectionSpace protection_space =
      details.proxy_protection_space();

  if (!primary_profile_) {
    SendEmptyCredentials(protection_space);
    return;
  }

  // The previous authentication attempt failed.
  if (details.has_bad_cached_credentials() &&
      details.bad_cached_credentials()) {
    ShowAuthenticationNotification(protection_space,
                                   details.bad_cached_credentials());
    return;
  }

  // TODO(acostinas,chromium:1104818) |protection_space.origin()| is in a
  // URI-like format which may be incompatible between Chrome and libcurl, which
  // is used on the Chrome OS side. We should change |origin()| to be a PAC
  // string (a more "standard" way of representing proxies) and call
  // |FromPacString()| to create |proxy_server|.
  net::ProxyServer proxy_server = net::ProxyServer::FromURI(
      protection_space.origin(), net::ProxyServer::Scheme::SCHEME_HTTP);

  if (!proxy_server.is_valid()) {
    SendEmptyCredentials(protection_space);
    return;
  }
  content::BrowserContext::GetDefaultStoragePartition(primary_profile_)
      ->GetNetworkContext()
      ->LookupProxyAuthCredentials(
          proxy_server, protection_space.scheme(),
          net::HttpUtil::Unquote(protection_space.realm()),
          base::BindOnce(
              &SystemProxyManager::LookupProxyAuthCredentialsCallback,
              weak_factory_.GetWeakPtr(), protection_space));
}

void SystemProxyManager::LookupProxyAuthCredentialsCallback(
    const system_proxy::ProtectionSpace& protection_space,
    const base::Optional<net::AuthCredentials>& credentials) {
  if (!credentials) {
    // Ask the user for credentials
    ShowAuthenticationNotification(protection_space, /*show_error=*/false);
    return;
  }

  std::string username;
  std::string password;
  if (credentials) {
    username = base::UTF16ToUTF8(credentials->username());
    password = base::UTF16ToUTF8(credentials->password());

    // If there's a dialog requesting credentials for this proxy, close it.
    if (notification_handler_ ||
        (active_auth_dialog_ &&
         active_auth_dialog_->GetProxyServer() == protection_space.origin())) {
      CloseAuthenticationUI();
    }
  }
  SendUserAuthenticationCredentials(protection_space, username, password);
}

void SystemProxyManager::ShowAuthenticationNotification(
    const system_proxy::ProtectionSpace& protection_space,
    bool show_error) {
  if (active_auth_dialog_)
    return;
  notification_handler_ = std::make_unique<chromeos::SystemProxyNotification>(
      protection_space, show_error,
      base::BindOnce(&SystemProxyManager::ShowAuthenticationDialog,
                     weak_factory_.GetWeakPtr()));
  notification_handler_->Show();
}

void SystemProxyManager::ShowAuthenticationDialog(
    const system_proxy::ProtectionSpace& protection_space,
    bool show_error_label) {
  if (active_auth_dialog_)
    return;

  if (notification_handler_)
    notification_handler_->Close();

  active_auth_dialog_ = new chromeos::RequestSystemProxyCredentialsView(
      protection_space.origin(), show_error_label,
      base::BindOnce(&SystemProxyManager::OnDialogClosed,
                     weak_factory_.GetWeakPtr(), protection_space));

  active_auth_dialog_->SetAcceptCallback(
      base::BindRepeating(&SystemProxyManager::OnDialogAccepted,
                          weak_factory_.GetWeakPtr(), protection_space));
  active_auth_dialog_->SetCancelCallback(
      base::BindRepeating(&SystemProxyManager::OnDialogCanceled,
                          weak_factory_.GetWeakPtr(), protection_space));

  auth_widget_ = views::DialogDelegate::CreateDialogWidget(
      active_auth_dialog_, /*context=*/nullptr, /*parent=*/nullptr);
  auth_widget_->Show();
}

void SystemProxyManager::OnDialogAccepted(
    const system_proxy::ProtectionSpace& protection_space) {
  SendUserAuthenticationCredentials(
      protection_space, base::UTF16ToUTF8(active_auth_dialog_->GetUsername()),
      base::UTF16ToUTF8(active_auth_dialog_->GetPassword()));
}

void SystemProxyManager::OnDialogCanceled(
    const system_proxy::ProtectionSpace& protection_space) {
  SendEmptyCredentials(protection_space);
}

void SystemProxyManager::OnDialogClosed(
    const system_proxy::ProtectionSpace& protection_space) {
  active_auth_dialog_ = nullptr;
  auth_widget_ = nullptr;
}

void SystemProxyManager::CloseAuthenticationUI() {
  // Closes the notification if shown.
  if (notification_handler_) {
    notification_handler_->Close();
    notification_handler_.reset();
  }
  if (!auth_widget_)
    return;
  // Also deletes the |auth_widget_| instance.
  auth_widget_->CloseWithReason(views::Widget::ClosedReason::kUnspecified);
}

}  // namespace policy
