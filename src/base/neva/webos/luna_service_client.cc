// Copyright 2018-2019 LG Electronics, Inc.
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

#include "base/neva/webos/luna_service_client.h"

#include <glib.h>

#include "base/callback_helpers.h"
#include "base/logging.h"
#include "base/rand_util.h"

namespace base {

namespace {

const char kURIAudio[] = "luna://com.webos.audio";
const char kURISetting[] = "luna://com.webos.settingsservice";
const char kURIMediaController[] = "luna://com.webos.service.mediacontroller";

}  // namespace

// static
std::string LunaServiceClient::GetServiceURI(URIType type,
                                             const std::string& action) {
  if (type < 0 || type > URIType::URITypeMax)
    return std::string();

  static std::map<URIType, std::string> kURIMap = {
      {URIType::AUDIO, kURIAudio},
      {URIType::SETTING, kURISetting},
      {URIType::MEDIACONTROLLER, kURIMediaController}};

  auto luna_service_uri = [&type]() {
    std::map<URIType, std::string>::iterator it;
    it = kURIMap.find(type);
    if (it != kURIMap.end())
      return it->second;
    return std::string();
  };

  std::string uri = luna_service_uri();
  uri.append("/");
  uri.append(action);
  return uri;
}

// LunaServiceClient implematation
LunaServiceClient::LunaServiceClient(const std::string& identifier)
    : handle_(nullptr), context_(nullptr) {
  if (!RegisterService(identifier))
    LOG(INFO) << "Failed to register service " << identifier.c_str();
}

LunaServiceClient::~LunaServiceClient() {
  UnregisterService();
}

bool HandleAsync(LSHandle* sh, LSMessage* reply, void* ctx) {
  LunaServiceClient::ResponseHandlerWrapper* wrapper =
      static_cast<LunaServiceClient::ResponseHandlerWrapper*>(ctx);

  LSMessageRef(reply);
  std::string dump = LSMessageGetPayload(reply);
  LOG(INFO) << "[RES] - " << wrapper->uri << " " << dump;
  if (!wrapper->callback.is_null())
    std::move(wrapper->callback).Run(dump);

  LSMessageUnref(reply);

  delete wrapper;

  return true;
}

bool HandleSubscribe(LSHandle* sh, LSMessage* reply, void* ctx) {
  LunaServiceClient::ResponseHandlerWrapper* wrapper =
      static_cast<LunaServiceClient::ResponseHandlerWrapper*>(ctx);

  LSMessageRef(reply);
  std::string dump = LSMessageGetPayload(reply);
  LOG(INFO) << "[SUB-RES] - " << wrapper->uri << " " << dump;
  if (!wrapper->callback.is_null())
    wrapper->callback.Run(dump);

  LSMessageUnref(reply);

  return true;
}

bool LunaServiceClient::CallAsync(const std::string& uri,
                                  const std::string& param) {
  ResponseCB nullcb;
  return CallAsync(uri, param, nullcb);
}

bool LunaServiceClient::CallAsync(const std::string& uri,
                                  const std::string& param,
                                  const ResponseCB& callback) {
  AutoLSError error;
  ResponseHandlerWrapper* wrapper = new ResponseHandlerWrapper;
  if (!wrapper)
    return false;

  wrapper->callback = callback;
  wrapper->uri = uri;
  wrapper->param = param;

  if (!handle_) {
    delete wrapper;
    return false;
  }

  LOG(INFO) << "[REQ] - " << uri << " " << param;
  if (!LSCallOneReply(handle_, uri.c_str(), param.c_str(), HandleAsync, wrapper,
                      NULL, &error)) {
    LOG(ERROR) << "[SUB] " << uri << ": fail[" << error.message << "]";
    std::move(wrapper->callback).Run("");
    delete wrapper;
    return false;
  }

  return true;
}

bool LunaServiceClient::Subscribe(const std::string& uri,
                                  const std::string& param,
                                  LSMessageToken* subscribeKey,
                                  const ResponseCB& callback) {
  AutoLSError error;
  ResponseHandlerWrapper* wrapper = new ResponseHandlerWrapper;
  if (!wrapper)
    return false;

  wrapper->callback = callback;
  wrapper->uri = uri;
  wrapper->param = param;

  if (!handle_) {
    delete wrapper;
    return false;
  }

  VLOG(1) << "[REQ] - " << uri << " " << param;
  if (!LSCall(handle_, uri.c_str(), param.c_str(), HandleSubscribe, wrapper,
              subscribeKey, &error)) {
    LOG(ERROR) << "[SUB] " << uri << ": fail[" << error.message << "]";
    delete wrapper;
    return false;
  }

  handlers_[*subscribeKey] = std::unique_ptr<ResponseHandlerWrapper>(wrapper);

  return true;
}

bool LunaServiceClient::Unsubscribe(LSMessageToken subscribeKey) {
  AutoLSError error;

  if (!handle_)
    return false;

  if (!LSCallCancel(handle_, subscribeKey, &error)) {
    LOG(INFO) << "[UNSUB] " << subscribeKey << " fail[" << error.message << "]";
    handlers_.erase(subscribeKey);
    return false;
  }

  if (handlers_[subscribeKey])
    handlers_[subscribeKey]->callback.Reset();

  handlers_.erase(subscribeKey);

  return true;
}

bool LunaServiceClient::RegisterService(const std::string& identifier) {
  AutoLSError error;
  std::string service_name = identifier + '-' + std::to_string(getpid());
  if (!LSRegisterApplicationService(service_name.c_str(),
                                             identifier.c_str(), &handle_, &error)) {
    LogError("Fail to register to LS2", error);
    return false;
  }
  context_ = g_main_context_ref(g_main_context_default());
  if (!LSGmainContextAttach(handle_, context_, &error)) {
    UnregisterService();
    LogError("Fail to attach a service to a mainloop", error);
    return false;
  }

  return true;
}

bool LunaServiceClient::UnregisterService() {
  AutoLSError error;
  if (!handle_)
    return false;
  if (!LSUnregister(handle_, &error)) {
    LogError("Fail to unregister service", error);
    return false;
  }
  g_main_context_unref(context_);
  return true;
}

void LunaServiceClient::LogError(const std::string& message,
                                 AutoLSError& lserror) {
  LOG(ERROR) << message.c_str() << " " << lserror.error_code << " : "
             << lserror.message << "(" << lserror.func << " @ " << lserror.file
             << ":" << lserror.line << ")";
}

}  // namespace base
