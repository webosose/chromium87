// Copyright 2018 LG Electronics, Inc.
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

#include "neva/app_runtime/browser/net/app_runtime_network_change_notifier.h"

#include "base/memory/singleton.h"

namespace neva_app_runtime {

net::NetworkChangeNotifier::ConnectionType
AppRuntimeNetworkChangeNotifier::GetCurrentConnectionType() const {
  return network_connected_ ? net::NetworkChangeNotifier::CONNECTION_UNKNOWN
                            : net::NetworkChangeNotifier::CONNECTION_NONE;
}

void AppRuntimeNetworkChangeNotifier::OnNetworkStateChanged(
    bool is_connected) {
  if (network_connected_ != is_connected) {
    network_connected_ = is_connected;
    net::NetworkChangeNotifier::NotifyObserversOfMaxBandwidthChange(
        network_connected_ ? std::numeric_limits<double>::infinity() : 0.0,
        GetCurrentConnectionType());
  }
}

// static
AppRuntimeNetworkChangeNotifier*
AppRuntimeNetworkChangeNotifier::GetInstance() {
  return base::Singleton<AppRuntimeNetworkChangeNotifier>::get();
}

}  // namespace neva_app_runtime
