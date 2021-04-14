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

#include "ozone/wayland/input/touchscreen.h"

#include <linux/input.h>

#include "ozone/wayland/display.h"
#include "ozone/wayland/input/cursor.h"
#include "ozone/wayland/seat.h"
#include "ozone/wayland/window.h"
#include "ui/events/event.h"

namespace ozonewayland {

WaylandTouchscreen::WaylandTouchscreen() : wl_touch_(nullptr) {}

WaylandTouchscreen::~WaylandTouchscreen() {
  if (wl_touch_)
    wl_touch_destroy(wl_touch_);
}

void WaylandTouchscreen::OnSeatCapabilities(wl_seat *seat, uint32_t caps) {
  static const struct wl_touch_listener kInputTouchListener = {
    WaylandTouchscreen::OnTouchDown,
    WaylandTouchscreen::OnTouchUp,
    WaylandTouchscreen::OnTouchMotion,
    WaylandTouchscreen::OnTouchFrame,
    WaylandTouchscreen::OnTouchCancel,
  };

  if ((caps & WL_SEAT_CAPABILITY_TOUCH)) {
    wl_touch_ = wl_seat_get_touch(seat);
    wl_touch_set_user_data(wl_touch_, this);
    wl_touch_add_listener(wl_touch_, &kInputTouchListener, this);
  }
}

void WaylandTouchscreen::OnTouchDown(void *data,
                                     struct wl_touch *wl_touch,
                                     uint32_t serial,
                                     uint32_t time,
                                     struct wl_surface *surface,
                                     int32_t id,
                                     wl_fixed_t x,
                                     wl_fixed_t y) {
  WaylandTouchscreen* device = static_cast<WaylandTouchscreen*>(data);
  WaylandDisplay::GetInstance()->SetSerial(serial);
  WaylandSeat* seat = WaylandDisplay::GetInstance()->PrimarySeat();

  const uint32_t device_id =
      wl_proxy_get_id(reinterpret_cast<wl_proxy*>(wl_touch));

  if (!surface) {
    seat->SetEnteredWindowHandle(device_id, 0);
    return;
  }

  WaylandWindow* window =
      static_cast<WaylandWindow*>(wl_surface_get_user_data(surface));

  if (window) {
    seat->SetEnteredWindowHandle(device_id, window->Handle());
    seat->SetActiveInputWindow(window->GetDisplayId(), window->Handle());
  }

  if (seat->GetEnteredWindowHandle(device_id) &&
      seat->GetGrabButton(device_id) == 0)
    seat->SetGrabWindow(
        device_id, GrabWindowInfo(seat->GetEnteredWindowHandle(device_id), id));

  float sx = wl_fixed_to_double(x);
  float sy = wl_fixed_to_double(y);

  device->pointer_position_.SetPoint(sx, sy);

  WaylandDisplay::GetInstance()->TouchNotify(ui::ET_TOUCH_PRESSED, sx, sy, id,
                                             time);
}

void WaylandTouchscreen::OnTouchUp(void* data,
                                   struct wl_touch* wl_touch,
                                   uint32_t serial,
                                   uint32_t time,
                                   int32_t id) {
  WaylandTouchscreen* device = static_cast<WaylandTouchscreen*>(data);
  WaylandDisplay::GetInstance()->SetSerial(serial);
  WaylandSeat* seat = WaylandDisplay::GetInstance()->PrimarySeat();
  const uint32_t device_id =
      wl_proxy_get_id(reinterpret_cast<wl_proxy*>(wl_touch));

  WaylandDisplay::GetInstance()->TouchNotify(
      ui::ET_TOUCH_RELEASED, device->pointer_position_.x(),
      device->pointer_position_.y(), id, time);

  if (int32_t(seat->GetGrabWindowHandle(device_id) &&
              seat->GetGrabButton(device_id)) == id)
    seat->SetGrabWindow(device_id, GrabWindowInfo(0, 0));
}

void WaylandTouchscreen::OnTouchMotion(void *data,
                                      struct wl_touch *wl_touch,
                                      uint32_t time,
                                      int32_t id,
                                      wl_fixed_t x,
                                      wl_fixed_t y) {
  WaylandTouchscreen* device = static_cast<WaylandTouchscreen*>(data);
  WaylandSeat* seat = WaylandDisplay::GetInstance()->PrimarySeat();
  const uint32_t device_id =
      wl_proxy_get_id(reinterpret_cast<wl_proxy*>(wl_touch));

  float sx = wl_fixed_to_double(x);
  float sy = wl_fixed_to_double(y);

  device->pointer_position_.SetPoint(sx, sy);

  if (seat->GetGrabWindowHandle(device_id) &&
      seat->GetGrabWindowHandle(device_id) !=
          seat->GetEnteredWindowHandle(device_id)) {
    return;
  }

  WaylandDisplay::GetInstance()->TouchNotify(ui::ET_TOUCH_MOVED, sx, sy, id,
                                             time);
}

void WaylandTouchscreen::OnTouchFrame(void *data,
                                      struct wl_touch *wl_touch) {
  // TODO(speedpat): find out what should be done here
}

void WaylandTouchscreen::OnTouchCancel(void *data,
                                       struct wl_touch *wl_touch) {
  WaylandTouchscreen* device = static_cast<WaylandTouchscreen*>(data);
  WaylandSeat* seat = WaylandDisplay::GetInstance()->PrimarySeat();

  const uint32_t device_id =
      wl_proxy_get_id(reinterpret_cast<wl_proxy*>(wl_touch));

  WaylandDisplay::GetInstance()->TouchNotify(
      ui::ET_TOUCH_CANCELLED, device->pointer_position_.x(),
      device->pointer_position_.y(), seat->GetGrabButton(device_id), 0);

  if (seat->GetGrabWindowHandle(device_id) &&
      seat->GetGrabButton(device_id) != 0)
    seat->SetGrabWindow(device_id, GrabWindowInfo(0, 0));
}

}  // namespace ozonewayland
