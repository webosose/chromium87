// Copyright (c) 2014 Noser Engineering AG. All rights reserved.
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

#ifndef OZONE_WAYLAND_INPUT_TOUCHSCREEN_H_
#define OZONE_WAYLAND_INPUT_TOUCHSCREEN_H_

#include <wayland-client-protocol.h>
#include <wayland-util.h>

#include "base/macros.h"
#include "ozone/wayland/input/hotplug_device.h"
#include "ui/gfx/geometry/point.h"

namespace ozonewayland {

class WaylandTouchscreen : public HotplugDevice {
 public:
  WaylandTouchscreen();
  ~WaylandTouchscreen() override;

  void OnSeatCapabilities(wl_seat *seat, uint32_t caps);

 private:
  static void OnTouchDown(
      void *data,
      struct wl_touch *wl_touch,
      uint32_t serial,
      uint32_t time,
      struct wl_surface *surface,
      int32_t id,
      wl_fixed_t x,
      wl_fixed_t y);

  static void OnTouchUp(
      void *data,
      struct wl_touch *wl_touch,
      uint32_t serial,
      uint32_t time,
      int32_t id);

  static void OnTouchMotion(
      void *data,
      struct wl_touch *wl_touch,
      uint32_t time,
      int32_t id,
      wl_fixed_t x,
      wl_fixed_t y);

  static void OnTouchFrame(
      void *data,
      struct wl_touch *wl_touch);

  static void OnTouchCancel(
      void *data,
      struct wl_touch *wl_touch);

  gfx::Point pointer_position_;
  struct wl_touch* wl_touch_;

  DISALLOW_COPY_AND_ASSIGN(WaylandTouchscreen);
};

}  // namespace ozonewayland

#endif  // OZONE_WAYLAND_INPUT_TOUCHSCREEN_H_
