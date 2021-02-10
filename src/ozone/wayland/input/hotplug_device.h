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

#ifndef OZONE_WAYLAND_INPUT_HOTPLUG_DEVICE_H_
#define OZONE_WAYLAND_INPUT_HOTPLUG_DEVICE_H_

#include <string>

namespace ozonewayland {

// Class representing a Wayland hotplug input device. To be inherited
// from to supply device traits.
class HotplugDevice {
 public:
  HotplugDevice();
  HotplugDevice(const HotplugDevice&) = delete;
  HotplugDevice& operator=(const HotplugDevice&) = delete;
  virtual ~HotplugDevice() = default;

  int GetId() const { return id_; }
  void SetName(const std::string& name) { name_ = name; }
  std::string GetName() const { return name_; }

 private:
  int id_;
  std::string name_;
};

}  // namespace ozonewayland

#endif  // OZONE_WAYLAND_INPUT_HOTPLUG_DEVICE_H_
