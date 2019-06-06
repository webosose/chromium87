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

#include "neva/pal_service/webos/luna/luna_client_impl.h"

#include "base/logging.h"
#include "neva/pal_service/webos/luna/luna_names.h"

#include <glib.h>
#include <memory>
#include <stdlib.h>
#include <unistd.h>

namespace pal {
namespace luna {
namespace {

struct Error : LSError {
  Error() {
    LSErrorInit(this);
  }

  ~Error() {
    LSErrorFree(this);
  }
};

void LogError(Error& error) {
    LOG(ERROR) << error.error_code << ": "
               << error.message << " ("
               << error.func << " @ "
               << error.file << ":"
               << error.line << ")";
}

}  // namespace

ClientImpl::ClientImpl(const Params& params)
  : params_(params) {
  Error error;
  bool registered = false;

  if (params.appid.empty() || params.name.find(service_name::kSettingsClient) == 0) {
    registered = LSRegister(params.name.c_str(),
        &handle_,
        &error);
  } else {
    registered = LSRegisterApplicationService(
        params.name.c_str(),
        params.appid.c_str(),
        &handle_,
        &error);
  }

  if (!registered) {
    LogError(error);
    return;
  }

  context_ = g_main_context_ref(g_main_context_default());
  if (!LSGmainContextAttach(handle_, context_, &error)) {
    LogError(error);
    if (!LSUnregister(handle_, &error))
      LogError(error);
    g_main_context_unref(context_);
    context_ = nullptr;
    handle_ = nullptr;
  }
}

ClientImpl::~ClientImpl() {
  CancelAllSubscriptions();
  CancelWaitingCalls();

  Error error;
  if (handle_) {
    if (!LSUnregister(handle_, &error))
      LogError(error);
  }
  if (context_)
    g_main_context_unref(context_);
}

bool ClientImpl::IsInitialized() const {
  return handle_ != nullptr;
}

Bus ClientImpl::GetBusType() const {
  return params_.bus;
}

std::string ClientImpl::GetName() const {
  return params_.name;
}

std::string ClientImpl::GetAppId() const {
  return params_.appid;
}

bool ClientImpl::Call(std::string uri,
                      std::string param,
                      OnceResponse callback,
                      std::string on_cancel_value,
                      unsigned* token) {
  return CallFromApp(std::move(uri),
                     std::move(param),
                     std::string(),
                     std::move(callback),
                     std::move(on_cancel_value),
                     token);
}

bool ClientImpl::CallFromApp(std::string uri,
                             std::string param,
                             std::string app_id,
                             OnceResponse callback,
                             std::string on_cancel_value,
                             unsigned* token) {
  if (!handle_)
    return false;

  Error error;
  auto response = std::make_unique<Response>();
  response->callback = std::move(callback);
  response->context.ptr = this;
  response->context.uri = std::move(uri);
  response->context.param = std::move(param);
  response->context.app_id = std::move(app_id);
  response->context.on_cancel_value = std::move(on_cancel_value);

  const char* app_id_str = response->context.app_id.empty()
                         ? nullptr : response->context.app_id.c_str();

  if (!LSCallFromApplicationOneReply(handle_,
                                     response->context.uri.c_str(),
                                     response->context.param.c_str(),
                                     app_id_str,
                                     HandleResponse,
                                     response.get(),
                                     &(response->context.token),
                                     &error)) {
    LogError(error);
    std::move(callback).Run(ResponseStatus::ERROR,
                            static_cast<unsigned>(response->context.token),
                            response->context.on_cancel_value);
    return false;
  }

  unsigned ret_token = static_cast<unsigned>(response->context.token);
  responses_[ret_token] = std::move(response);

  if (token)
    *token = ret_token;
  return true;
}

bool ClientImpl::Subscribe(std::string uri,
                           std::string param,
                           RepeatingResponse callback,
                           std::string on_cancel_value,
                           unsigned* token) {
  return SubscribeFromApp(std::move(uri),
                          std::move(param),
                          std::string(),
                          std::move(callback),
                          std::move(on_cancel_value),
                          token);
}

bool ClientImpl::SubscribeFromApp(std::string uri,
                                  std::string param,
                                  std::string app_id,
                                  RepeatingResponse callback,
                                  std::string on_cancel_value,
                                  unsigned* token) {
  if (!handle_)
    return false;

  Error error;
  auto subscription = std::make_unique<Subscription>();
  subscription->callback = std::move(callback);
  subscription->context.ptr = this;
  subscription->context.uri = std::move(uri);
  subscription->context.param = std::move(param);
  subscription->context.app_id = std::move(app_id);
  subscription->context.on_cancel_value = std::move(on_cancel_value);

  const char* app_id_str = subscription->context.app_id.empty()
                         ? nullptr : subscription->context.app_id.c_str();

  if (!LSCallFromApplication(handle_,
              subscription->context.uri.c_str(),
              subscription->context.param.c_str(),
              app_id_str,
              HandleSubscribe,
              subscription.get(),
              &(subscription->context.token),
              &error)) {
    LogError(error);
    callback.Run(ResponseStatus::ERROR,
                 static_cast<unsigned>(subscription->context.token),
                 subscription->context.on_cancel_value);
    return false;
  }

  unsigned ret_token = static_cast<unsigned>(subscription->context.token);
  subscriptions_[ret_token] = std::move(subscription);

  if (token)
    *token = ret_token;
  return true;
}

void ClientImpl::Cancel(unsigned token) {
  if (!handle_)
    return;

  auto it = responses_.find(token);
  if (it == responses_.end())
    return;

  LSMessageToken key = it->second->context.token;
  Error error;
  if (!LSCallCancel(handle_, key, &error))
    LOG(INFO) << "[CANCEL] " << key << " fail [" << error.message << "]";

  std::move(it->second->callback)
      .Run(ResponseStatus::CANCELED, token,
           it->second->context.on_cancel_value);
  responses_.erase(it);
}

void ClientImpl::Unsubscribe(unsigned token) {
  if (!handle_)
    return;

  auto it = subscriptions_.find(token);
  if (it == subscriptions_.end())
    return;

  Error error;
  LSMessageToken key = it->second->context.token;
  if (!LSCallCancel(handle_, key, &error))
    LOG(INFO) << "[UNSUB] " << key << " fail [" << error.message << "]";
  std::move(it->second->callback)
      .Run(ResponseStatus::CANCELED, token,
           it->second->context.on_cancel_value);
  subscriptions_.erase(it);
}

void ClientImpl::CancelAllSubscriptions() {
  if (!handle_)
    return;

  Error error;
  for (auto& subscription : subscriptions_) {
    LSMessageToken key = subscription.second->context.token;
    if (!LSCallCancel(handle_, key, &error))
      LOG(INFO) << "[UNSUB] " << key << " fail [" << error.message << "]";

    std::move(subscription.second->callback)
        .Run(ResponseStatus::CANCELED,
             subscription.second->context.token,
             subscription.second->context.on_cancel_value);
  }
  subscriptions_.clear();
}

void ClientImpl::CancelWaitingCalls() {
  if (!handle_)
    return;

  Error error;
  for (auto& response : responses_) {
    LSMessageToken key = response.second->context.token;
    if (!LSCallCancel(handle_, key, &error))
      LOG(INFO) << "[CANCEL] " << key << " fail [" << error.message << "]";
    std::move(response.second->callback)
        .Run(ResponseStatus::CANCELED,
             response.second->context.token,
             response.second->context.on_cancel_value);
  }
  responses_.clear();
}

// static
bool ClientImpl::HandleResponse(LSHandle* sh, LSMessage* reply, void* ctx) {
  ClientImpl::Response* response = static_cast<ClientImpl::Response*>(ctx);
  if (response && response->context.ptr) {
    auto self = response->context.ptr;
    auto it = self->responses_.find(response->context.token);
    if (it != self->responses_.end()) {
      LSMessageRef(reply);
      const std::string dump = LSMessageGetPayload(reply);
      std::move(response->callback).Run(
          ResponseStatus::SUCCESS, response->context.token, dump);
      self->responses_.erase(it);
      LSMessageUnref(reply);
    }
  } else {
    NOTREACHED();
    delete response;
  }

  return true;
}

// static
bool ClientImpl::HandleSubscribe(LSHandle* sh, LSMessage* reply, void* ctx) {
  ClientImpl::Subscription* subscription =
    static_cast<ClientImpl::Subscription*>(ctx);
  if (subscription) {
    LSMessageRef(reply);
    const std::string dump = LSMessageGetPayload(reply);
    subscription->callback.Run(
        ResponseStatus::SUCCESS, subscription->context.token, dump);
    LSMessageUnref(reply);
  } else {
    NOTREACHED();
    delete subscription;
  }

  return true;
}

}  // namespace luna
}  // namespace pal
