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

#include "neva/pal_service/system_servicebridge.h"

#include "mojo/public/cpp/bindings/associated_receiver.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "neva/pal_service/pal_platform_factory.h"
#include "neva/pal_service/system_servicebridge_delegate.h"

namespace pal {

// SystemServiceBridgeImpl

SystemServiceBridgeImpl::SystemServiceBridgeImpl()
  : weak_factory_(this) {
}

SystemServiceBridgeImpl::~SystemServiceBridgeImpl() {
}

void SystemServiceBridgeImpl::Connect(const std::string& name,
                                      const std::string& appid,
                                      ConnectCallback callback) {
  if (delegate_) {
    LOG(ERROR) << "SystemServiceBridge [ name = " << name
               << ", appid = " << appid
               << "] is already connected";
    std::move(callback).Run(mojo::NullAssociatedReceiver());
    return;
  }

  delegate_ = PlatformFactory::Get()->CreateSystemServiceBridgeDelegate(
      name,
      appid,
      base::BindRepeating(&SystemServiceBridgeImpl::OnResponse,
                          weak_factory_.GetWeakPtr()));
  std::move(callback).Run(remote_client_.BindNewEndpointAndPassReceiver());
}

void SystemServiceBridgeImpl::Call(const std::string& uri,
                                   const std::string& payload) {
  if (delegate_)
    delegate_->Call(uri, payload);
}

void SystemServiceBridgeImpl::Cancel() {
  if (delegate_)
    delegate_->Cancel();
}

void SystemServiceBridgeImpl::OnResponse(mojom::ResponseStatus status,
                                         const std::string& payload) {
  if (remote_client_)
    remote_client_->Response(status, payload);
}

// SystemServiceBridgeProviderImpl

SystemServiceBridgeProviderImpl::SystemServiceBridgeProviderImpl() {
}

SystemServiceBridgeProviderImpl::~SystemServiceBridgeProviderImpl() {
}

void SystemServiceBridgeProviderImpl::AddBinding(
    mojo::PendingReceiver<mojom::SystemServiceBridgeProvider> receiver) {
  receivers_.Add(this, std::move(receiver));
}

void SystemServiceBridgeProviderImpl::GetSystemServiceBridge(
    mojo::PendingReceiver<mojom::SystemServiceBridge> receiver) {
  bridges_receivers_.Add(
    std::make_unique<SystemServiceBridgeImpl>(),
    std::move(receiver));
}

}  // namespace pal
