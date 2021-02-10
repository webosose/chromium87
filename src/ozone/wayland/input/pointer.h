// Copyright 2013 Intel Corporation. All rights reserved.
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

#ifndef OZONE_WAYLAND_INPUT_POINTER_H_
#define OZONE_WAYLAND_INPUT_POINTER_H_

#include <wayland-client-protocol.h>
#include <wayland-util.h>

#include "base/macros.h"
#include "ozone/wayland/input/hotplug_device.h"
#include "ui/gfx/geometry/point.h"

namespace ozonewayland {

class WaylandCursor;

class WaylandPointer : public HotplugDevice {
 public:
  WaylandPointer();
  ~WaylandPointer() override;

  void OnSeatCapabilities(wl_seat *seat, uint32_t caps);
  WaylandCursor* Cursor() const { return cursor_; }

 private:
  static void OnMotionNotify(
      void* data,
      wl_pointer* input_pointer,
      uint32_t time,
      wl_fixed_t sx_w,
      wl_fixed_t sy_w);

  static void OnButtonNotify(
      void* data,
      wl_pointer* input_pointer,
      uint32_t serial,
      uint32_t time,
      uint32_t button,
      uint32_t state);

  static void OnAxisNotify(
      void* data,
      wl_pointer* input_pointer,
      uint32_t time,
      uint32_t axis,
      int32_t value);

  static void OnPointerEnter(
      void* data,
      wl_pointer* input_pointer,
      uint32_t serial,
      wl_surface* surface,
      wl_fixed_t sx_w,
      wl_fixed_t sy_w);

  static void OnPointerLeave(
      void* data,
      wl_pointer* input_pointer,
      uint32_t serial,
      wl_surface* surface);

  WaylandCursor* cursor_;
  // Keeps track of the last position for the motion event. We want to
  // dispatch this with events such as wheel or button which don't have a
  // position associated on Wayland.
  gfx::Point pointer_position_;
  struct wl_pointer *input_pointer_;

  DISALLOW_COPY_AND_ASSIGN(WaylandPointer);
};

}  // namespace ozonewayland

#endif  // OZONE_WAYLAND_INPUT_POINTER_H_
