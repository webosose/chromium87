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

#include "neva/pal_service/webos/network_error_page_controller_delegate_webos.h"

#include "base/json/json_writer.h"
#include "base/values.h"
#include "neva/pal_service/webos/luna/luna_names.h"

namespace pal {
namespace webos {

const char kLaunchingMethod[] = "launch";
const char kSettingsApplicationId[] = "com.palm.app.settings";
const char kParams[] = "params";
const char kTarget[] = "target";

// This enum is strictly related to the TargetEnum = {NETWORK: 1, GENERAL: 2}
// in webos/browser/resources/webos_network_error_page_template.js file.
enum LAUNCH_TARGET {
  NETWORK = 1,
  GENERAL,
};

std::string MapTargetIdToString(int target_id) {
  std::string ret;
  switch (target_id) {
    case NETWORK:
      ret = "network";
      break;
    case GENERAL:
      ret = "general";
      break;
  }
  return ret;
}

NetworkErrorPageControllerDelegateWebOS::
    NetworkErrorPageControllerDelegateWebOS() {
  using namespace luna;
  Client::Params params;
  params.bus = Bus::Private;
  luna_client_ = CreateClient(params);
}

NetworkErrorPageControllerDelegateWebOS::
    ~NetworkErrorPageControllerDelegateWebOS() {}

void NetworkErrorPageControllerDelegateWebOS::LaunchNetworkSettings(
    int target_id) {
  std::string target_string = MapTargetIdToString(target_id);
  if (target_string.empty())
    return;

  const std::unique_ptr<base::DictionaryValue> resource(
      new base::DictionaryValue());
  resource->SetString("id", kSettingsApplicationId);

  const std::unique_ptr<base::DictionaryValue> params(
      new base::DictionaryValue());
  const std::unique_ptr<base::DictionaryValue> target(
      new base::DictionaryValue());
  target->SetString(kTarget, target_string);
  resource->Set(kParams, base::Value::ToUniquePtrValue(target->Clone()));

  std::string payload;
  base::JSONWriter::Write(*(resource.get()), &payload);
  unsigned token;
  luna_client_->Call(
      luna::GetServiceURI(luna::service_uri::kApplicationManager,
                          kLaunchingMethod),
      payload,
      base::BindOnce(
          &NetworkErrorPageControllerDelegateWebOS::OnLaunchNetworkSettings,
          base::Unretained(this)),
      std::string("{}"));
}

void NetworkErrorPageControllerDelegateWebOS::OnLaunchNetworkSettings(
    luna::Client::ResponseStatus,
    unsigned,
    const std::string& json) {
  NOTIMPLEMENTED();
}

}  // namespace webos
}  // namespace pal
