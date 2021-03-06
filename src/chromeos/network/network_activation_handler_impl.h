// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_NETWORK_NETWORK_ACTIVATION_HANDLER_IMPL_H_
#define CHROMEOS_NETWORK_NETWORK_ACTIVATION_HANDLER_IMPL_H_

#include <string>

#include "base/component_export.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "chromeos/network/network_activation_handler.h"
#include "chromeos/network/network_handler_callbacks.h"

namespace chromeos {

// The NetworkActivationHandlerImpl class allows making service specific
// calls required for activation on mobile networks.
class COMPONENT_EXPORT(CHROMEOS_NETWORK) NetworkActivationHandlerImpl
    : public NetworkActivationHandler,
      public base::SupportsWeakPtr<NetworkActivationHandlerImpl> {
 public:
  ~NetworkActivationHandlerImpl() override;

 private:
  // NetworkActivationHandler:
  void CompleteActivation(
      const std::string& service_path,
      base::OnceClosure success_callback,
      network_handler::ErrorCallback error_callback) override;

 private:
  friend class NetworkHandler;

  NetworkActivationHandlerImpl();

  void HandleShillSuccess(base::OnceClosure success_callback);

  DISALLOW_COPY_AND_ASSIGN(NetworkActivationHandlerImpl);
};

}  // namespace chromeos

#endif  // CHROMEOS_NETWORK_NETWORK_ACTIVATION_HANDLER_IMPL_H_
