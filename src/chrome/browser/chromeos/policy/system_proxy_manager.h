// Copyright (c) 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_POLICY_SYSTEM_PROXY_MANAGER_H_
#define CHROME_BROWSER_CHROMEOS_POLICY_SYSTEM_PROXY_MANAGER_H_

#include <memory>
#include <string>

#include "base/callback_forward.h"
#include "base/gtest_prod_util.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "chrome/browser/chromeos/settings/cros_settings.h"
#include "chromeos/dbus/system_proxy/system_proxy_service.pb.h"
#include "net/base/auth.h"

namespace chromeos {
class RequestSystemProxyCredentialsView;
class SystemProxyNotification;
}  // namespace chromeos

namespace system_proxy {
class SetAuthenticationDetailsResponse;
class ShutDownResponse;
}  // namespace system_proxy

namespace views {
class Widget;
}

class PrefRegistrySimple;
class PrefService;
class PrefChangeRegistrar;
class Profile;

namespace policy {

// This class observes the device setting |SystemProxySettings|, and controls
// the availability of System-proxy service and the configuration of the web
// proxy credentials for system services connecting through System-proxy. It
// also listens for the |WorkerActive| dbus signal sent by the System-proxy
// daemon and stores connection information regarding the active worker
// processes.
class SystemProxyManager {
 public:
  SystemProxyManager(chromeos::CrosSettings* cros_settings,
                     PrefService* local_state);
  SystemProxyManager(const SystemProxyManager&) = delete;

  SystemProxyManager& operator=(const SystemProxyManager&) = delete;

  ~SystemProxyManager();

  // If System-proxy is enabled by policy, it returns the URL of the local proxy
  // instance that authenticates system services, in PAC format, e.g.
  //     PROXY localhost:3128
  // otherwise it returns an empty string.
  std::string SystemServicesProxyPacString() const;
  void StartObservingPrimaryProfilePrefs(Profile* profile);
  void StopObservingPrimaryProfilePrefs();
  // If System-proxy is enabled, it will send a request via D-Bus to clear the
  // user's proxy credentials cached by the local proxy workers. System-proxy
  // requests proxy credentials from the browser by sending an
  // |AuthenticationRequired| D-Bus signal.
  void ClearUserCredentials();

  void SetSystemProxyEnabledForTest(bool enabled);
  void SetSystemServicesProxyUrlForTest(const std::string& local_proxy_url);
  void SetSendAuthDetailsClosureForTest(base::RepeatingClosure closure);
  chromeos::RequestSystemProxyCredentialsView* GetActiveAuthDialogForTest();
  void CloseAuthDialogForTest();

  // Registers prefs stored in user profiles.
  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

 private:
  void OnSetAuthenticationDetails(
      const system_proxy::SetAuthenticationDetailsResponse& response);
  void OnShutDownProcess(const system_proxy::ShutDownResponse& response);
  void OnClearUserCredentials(
      const system_proxy::ClearUserCredentialsResponse& response);

  void OnKerberosEnabledChanged();
  void OnKerberosAccountChanged();
  void OnArcEnabledChanged();
  // Sets the value of the pref |kSystemProxyUserTrafficHostAndPort|.
  void SetUserTrafficProxyPref(const std::string& user_traffic_address);
  bool IsArcEnabled() const;

  // Sends the authentication details for |protection_space| to System-proxy via
  // D-Bus.
  void SendUserAuthenticationCredentials(
      const system_proxy::ProtectionSpace& protection_space,
      const std::string& username,
      const std::string& password);
  // Send the Kerberos enabled state and active principal name to System-proxy
  // via D-Bus.
  void SendKerberosAuthenticationDetails();
  // Sends empty credentials for |protection_space| to System-proxy via D-Bus.
  // This can mean that a user is not signed into Chrome OS or they didn't
  // provide proxy authentication credentials. In this case, System-proxy will
  // forward the authentication failure (HTTP 407 status code) to the Chrome OS
  // client.
  void SendEmptyCredentials(
      const system_proxy::ProtectionSpace& protection_space);

  // Once a trusted set of policies is established, this function calls
  // the System-proxy dbus client to start/shutdown the daemon and, if
  // necessary, to configure the web proxy credentials for system services.
  void OnSystemProxySettingsPolicyChanged();

  // This function is called when the |WorkerActive| dbus signal is received.
  void OnWorkerActive(const system_proxy::WorkerActiveSignalDetails& details);

  // Requests from the NetworkService the user credentials associated with the
  // protection space specified in |details|. This function is called when the
  // |AuthenticationRequired| dbus signal is received.
  void OnAuthenticationRequired(
      const system_proxy::AuthenticationRequiredDetails& details);

  // Forwards the user credentials to System-proxy. |credentials| may be empty
  // indicating the credentials for the specified |protection_space| are not
  // available.
  void LookupProxyAuthCredentialsCallback(
      const system_proxy::ProtectionSpace& protection_space,
      const base::Optional<net::AuthCredentials>& credentials);

  void ShowAuthenticationNotification(
      const system_proxy::ProtectionSpace& protection_space,
      bool show_error);

  // Shows a dialog which prompts the user to introduce proxy authentication
  // credentials for OS level traffic. If |show_error_label| is true, the
  // dialog will show a label that indicates the previous attempt to
  // authenticate has failed due to invalid credentials.
  void ShowAuthenticationDialog(
      const system_proxy::ProtectionSpace& protection_space,
      bool show_error_label);
  void OnDialogAccepted(const system_proxy::ProtectionSpace& protection_space);
  void OnDialogCanceled(const system_proxy::ProtectionSpace& protection_space);
  void OnDialogClosed(const system_proxy::ProtectionSpace& protection_space);

  // Closes the authentication notification or dialog if shown.
  void CloseAuthenticationUI();

  chromeos::CrosSettings* cros_settings_;
  std::unique_ptr<chromeos::CrosSettings::ObserverSubscription>
      system_proxy_subscription_;

  bool system_proxy_enabled_ = false;
  // The authority URI in the format host:port of the local proxy worker for
  // system services.
  std::string system_services_address_;

  // Local state prefs, not owned.
  PrefService* local_state_ = nullptr;

  // Notification which informs the user that System-proxy requires credentials
  // for authentication to the remote proxy.
  std::unique_ptr<chromeos::SystemProxyNotification> notification_handler_;

  // Owned by |auth_widget_|.
  chromeos::RequestSystemProxyCredentialsView* active_auth_dialog_ = nullptr;
  // Owned by the UI code (NativeWidget).
  views::Widget* auth_widget_ = nullptr;

  // Primary profile, not owned.
  Profile* primary_profile_ = nullptr;

  // Observer for Kerberos-related prefs.
  std::unique_ptr<PrefChangeRegistrar> local_state_pref_change_registrar_;
  std::unique_ptr<PrefChangeRegistrar> profile_pref_change_registrar_;

  base::RepeatingClosure send_auth_details_closure_for_test_;

  base::WeakPtrFactory<SystemProxyManager> weak_factory_{this};
};

}  // namespace policy

#endif  // CHROME_BROWSER_CHROMEOS_POLICY_SYSTEM_PROXY_MANAGER_H_
