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

#include "ozone/wayland/input/pointer.h"

#include <linux/input.h>

#include "base/logging.h"
#include "ozone/wayland/display.h"
#include "ozone/wayland/input/cursor.h"
#include "ozone/wayland/seat.h"
#include "ozone/wayland/window.h"
#include "ui/events/event.h"

namespace ozonewayland {

WaylandPointer::WaylandPointer() : cursor_(nullptr), input_pointer_(nullptr) {}

WaylandPointer::~WaylandPointer() {
  delete cursor_;
  if (input_pointer_)
    wl_pointer_destroy(input_pointer_);
}

void WaylandPointer::OnSeatCapabilities(wl_seat *seat, uint32_t caps) {
  static const struct wl_pointer_listener kInputPointerListener = {
    WaylandPointer::OnPointerEnter,
    WaylandPointer::OnPointerLeave,
    WaylandPointer::OnMotionNotify,
    WaylandPointer::OnButtonNotify,
    WaylandPointer::OnAxisNotify,
  };

  if (!cursor_)
    cursor_ = new WaylandCursor();

  if ((caps & WL_SEAT_CAPABILITY_POINTER) && !cursor_->GetInputPointer()) {
    input_pointer_ = wl_seat_get_pointer(seat);
      cursor_->SetInputPointer(input_pointer_);
    wl_pointer_set_user_data(input_pointer_, this);
    wl_pointer_add_listener(input_pointer_, &kInputPointerListener, this);
  }
}

void WaylandPointer::OnMotionNotify(void* data,
                                    wl_pointer* input_pointer,
                                    uint32_t time,
                                    wl_fixed_t sx_w,
                                    wl_fixed_t sy_w) {
  WaylandPointer* device = static_cast<WaylandPointer*>(data);
  WaylandSeat* seat = WaylandDisplay::GetInstance()->PrimarySeat();
  float sx = wl_fixed_to_double(sx_w);
  float sy = wl_fixed_to_double(sy_w);

  const uint32_t device_id =
      wl_proxy_get_id(reinterpret_cast<wl_proxy*>(input_pointer));

  device->pointer_position_.SetPoint(sx, sy);
  if (seat->GetGrabWindowHandle(device_id) &&
      seat->GetGrabWindowHandle(device_id) !=
          seat->GetEnteredWindowHandle(device_id)) {
    return;
  }

  WaylandDisplay::GetInstance()->MotionNotify(sx, sy);
}

void WaylandPointer::OnButtonNotify(void* data,
                                    wl_pointer* input_pointer,
                                    uint32_t serial,
                                    uint32_t time,
                                    uint32_t button,
                                    uint32_t state) {
  LOG(INFO) << __PRETTY_FUNCTION__
            << ": key=" << ((button == BTN_LEFT) ? "Left" : "Other")
            << ", type=" << ((state == WL_POINTER_BUTTON_STATE_PRESSED)
                                 ? "Press"
                                 : "Release");
  WaylandPointer* device = static_cast<WaylandPointer*>(data);
  WaylandDisplay::GetInstance()->SetSerial(serial);
  WaylandSeat* seat = WaylandDisplay::GetInstance()->PrimarySeat();
  const uint32_t device_id =
      wl_proxy_get_id(reinterpret_cast<wl_proxy*>(input_pointer));
  if (seat->GetEnteredWindowHandle(device_id) &&
      seat->GetGrabButton(device_id) == 0 &&
      state == WL_POINTER_BUTTON_STATE_PRESSED)
    seat->SetGrabWindow(
        device_id,
        GrabWindowInfo(seat->GetEnteredWindowHandle(device_id), button));

  if (seat->GetGrabWindowHandle(device_id)) {
    ui::EventType type = ui::ET_MOUSE_PRESSED;
    if (state == WL_POINTER_BUTTON_STATE_RELEASED)
      type = ui::ET_MOUSE_RELEASED;

    // TODO(vignatti): simultaneous clicks fail
    ui::EventFlags flags = ui::EF_NONE;
    if (button == BTN_LEFT)
      flags = ui::EF_LEFT_MOUSE_BUTTON;
    else if (button == BTN_RIGHT)
      flags = ui::EF_RIGHT_MOUSE_BUTTON;
    else if (button == BTN_MIDDLE)
      flags = ui::EF_MIDDLE_MOUSE_BUTTON;

    WaylandDisplay::GetInstance()->ButtonNotify(
        seat->GetEnteredWindowHandle(device_id), type, flags,
        device->pointer_position_.x(), device->pointer_position_.y());
  }

  if (seat->GetGrabWindowHandle(device_id) &&
      seat->GetGrabButton(device_id) == button &&
      state == WL_POINTER_BUTTON_STATE_RELEASED)
    seat->SetGrabWindow(device_id, GrabWindowInfo(0, 0));
}

void WaylandPointer::OnAxisNotify(void* data,
                                  wl_pointer* input_pointer,
                                  uint32_t time,
                                  uint32_t axis,
                                  int32_t value) {
  int x_offset = 0, y_offset = 0;
  WaylandPointer* device = static_cast<WaylandPointer*>(data);
  const int delta = ui::MouseWheelEvent::kWheelDelta;

  switch (axis) {
    case WL_POINTER_AXIS_HORIZONTAL_SCROLL:
      x_offset = value > 0 ? -delta : delta;
      break;
    case WL_POINTER_AXIS_VERTICAL_SCROLL:
      y_offset = value > 0 ? -delta : delta;
      break;
    default:
      break;
  }

  WaylandDisplay::GetInstance()->AxisNotify(device->pointer_position_.x(),
                                            device->pointer_position_.y(),
                                            x_offset, y_offset);
}

void WaylandPointer::OnPointerEnter(void* data,
                                    wl_pointer* input_pointer,
                                    uint32_t serial,
                                    wl_surface* surface,
                                    wl_fixed_t sx_w,
                                    wl_fixed_t sy_w) {
  WaylandSeat* seat = WaylandDisplay::GetInstance()->PrimarySeat();
  const uint32_t device_id =
      wl_proxy_get_id(reinterpret_cast<wl_proxy*>(input_pointer));

  if (!surface) {
    seat->SetEnteredWindowHandle(device_id, 0);
    return;
  }

  WaylandPointer* device = static_cast<WaylandPointer*>(data);

  if (seat && device) {
    float sx = wl_fixed_to_double(sx_w);
    float sy = wl_fixed_to_double(sy_w);

    WaylandDisplay::GetInstance()->SetSerial(serial);
    device->pointer_position_.SetPoint(sx, sy);

    WaylandWindow* window =
        static_cast<WaylandWindow*>(wl_surface_get_user_data(surface));

    if (window) {
      seat->SetEnteredWindowHandle(device_id, window->Handle());
      seat->SetActiveInputWindow(window->GetDisplayId(), window->Handle());
      WaylandDisplay::GetInstance()->PointerEnter(
          device_id, window->Handle(), device->pointer_position_.x(),
          device->pointer_position_.y());
    }
  }
}

void WaylandPointer::OnPointerLeave(void* data,
                                    wl_pointer* input_pointer,
                                    uint32_t serial,
                                    wl_surface* surface) {
  WaylandSeat* seat = WaylandDisplay::GetInstance()->PrimarySeat();
  WaylandPointer* device = static_cast<WaylandPointer*>(data);

  const uint32_t device_id =
      wl_proxy_get_id(reinterpret_cast<wl_proxy*>(input_pointer));

  if (seat && device) {
    WaylandDisplay::GetInstance()->SetSerial(serial);

    WaylandWindow* window =
        static_cast<WaylandWindow*>(wl_surface_get_user_data(surface));

    if (window) {
      WaylandDisplay::GetInstance()->PointerLeave(
          device_id, window->Handle(), device->pointer_position_.x(),
          device->pointer_position_.y());
      seat->SetEnteredWindowHandle(device_id, 0);
    }
  }
}

}  // namespace ozonewayland
