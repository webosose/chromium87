// Copyright 2014-2019 LG Electronics, Inc.
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

#ifndef BASE_NEVA_WEBOS_LUNA_SERVICE_CLIENT_H_
#define BASE_NEVA_WEBOS_LUNA_SERVICE_CLIENT_H_

#include <lunaservice.h>

#include <map>
#include <string>

#include "base/base_export.h"
#include "base/callback.h"

namespace base {

class LunaServiceClient {
 public:
  enum URIType {
    VSM = 0,
    DISPLAY,
    AVBLOCK,
    AUDIO,
    BROADCAST,
    CHANNEL,
    EXTERNALDEVICE,
    DVR,
    SOUND,
    SUBTITLE,
    DRM,
    SETTING,
    PHOTORENDERER,
    URITypeMax = PHOTORENDERER,
  };

  enum BusType {
    PrivateBus = 0,
    PublicBus,
  };

  using ResponseCB = base::Callback<void(const std::string&)>;

  struct ResponseHandlerWrapper {
    LunaServiceClient::ResponseCB callback;
    std::string uri;
    std::string param;
  };

  static std::string GetServiceURI(URIType type, const std::string& action);

  explicit LunaServiceClient(BusType type, bool need_service_name = false);
  ~LunaServiceClient();
  bool CallAsync(const std::string& uri, const std::string& param);
  bool CallAsync(const std::string& uri,
                 const std::string& param,
                 const ResponseCB& callback);
  bool Subscribe(const std::string& uri,
                 const std::string& param,
                 LSMessageToken* subscribeKey,
                 const ResponseCB& callback);
  bool Unsubscribe(LSMessageToken subscribeKey);

 private:
  LSHandle* handle;
  GMainContext* context;
  std::map<LSMessageToken, std::unique_ptr<ResponseHandlerWrapper>> handlers;

  DISALLOW_COPY_AND_ASSIGN(LunaServiceClient);
};

}  // namespace base

#endif  // BASE_NEVA_WEBOS_LUNA_SERVICE_CLIENT_H_
