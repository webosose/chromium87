// Copyright 2014 Intel Corporation. All rights reserved.
// Copyright 2016-2018 LG Electronics, Inc.
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

#ifndef OZONE_PLATFORM_DESKTOP_PLATFORM_SCREEN_DELEGATE_H_
#define OZONE_PLATFORM_DESKTOP_PLATFORM_SCREEN_DELEGATE_H_

#include <string>

#include "ozone/platform/ozone_export_wayland.h"

namespace ui {

// A simple observer interface for all clients interested in receiving various
// output change notifications like size changes, when a new output is added,
// etc.
class OZONE_WAYLAND_EXPORT DesktopPlatformScreenDelegate {
 public:
  virtual void OnScreenChanged(const std::string& display_id,
                               const std::string& display_name,
                               unsigned width, unsigned height,
                               int rotation) = 0;

 protected:
  virtual ~DesktopPlatformScreenDelegate() {}
};

}  // namespace ui

#endif  // OZONE_PLATFORM_DESKTOP_PLATFORM_SCREEN_DELEGATE_H_
