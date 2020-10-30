// Copyright 2021 LG Electronics, Inc.
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
#ifndef SERVICES_NETWORK_PUBLIC_CPP_NEVA_CORB_CORS_EXCEPTION_H_
#define SERVICES_NETWORK_PUBLIC_CPP_NEVA_CORB_CORS_EXCEPTION_H_

#include "base/optional.h"
#include "services/network/public/cpp/cors/cors_error_status.h"

namespace network {
namespace neva {

class CorsCorbException {
 public:
  static void AddForProcess(int process_id);
  static void RemoveForProcess(int process_id);
  static bool ShouldAllowExceptionForProcess(int process_id);
  static void ApplyException(base::Optional<CorsErrorStatus>& error_status);

 private:
  CorsCorbException() = delete;
  CorsCorbException(CorsCorbException&) = delete;
  CorsCorbException& operator=(const CorsCorbException&) = delete;
};

}  // namespace neva
}  // namespace network
#endif  // SERVICES_NETWORK_PUBLIC_CPP_NEVA_CORB_CORS_EXCEPTION_H_
