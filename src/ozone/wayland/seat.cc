// Copyright 2013 The Chromium Authors. All rights reserved.
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

#include "ozone/wayland/seat.h"

#include "base/logging.h"
#include "ozone/wayland/display.h"
#include "ozone/wayland/input/cursor.h"
#include "ozone/wayland/input/keyboard.h"
#include "ozone/wayland/input/pointer.h"
#include "ozone/wayland/input/touchscreen.h"

#if defined(OS_WEBOS)
#include "ozone/wayland/input/webos_text_input.h"
#else
#include "ozone/wayland/input/text_input.h"
#endif

#if defined(USE_DATA_DEVICE_MANAGER)
#include "ozone/wayland/data_device.h"
#endif

namespace {
const char kKeyboardSuffix[] = "_keyboard";
const char kPointerSuffix[] = "_pointer";
const char kTouchscreenSuffix[] = "_touch";
}  // namespace

namespace ozonewayland {

WaylandSeat::WaylandSeat(WaylandDisplay* display,
                         uint32_t id)
    : focused_window_handle_(0),
      grab_window_handle_(0),
      grab_button_(0),
      seat_(NULL),
#if defined(USE_DATA_DEVICE_MANAGER)
      data_device_(NULL),
#endif
      input_keyboard_(NULL),
      input_pointer_(NULL),
      input_touch_(NULL),
      text_input_(NULL) {
  static const struct wl_seat_listener kInputSeatListener = {
      WaylandSeat::OnSeatCapabilities,
      WaylandSeat::OnName,
  };

  seat_ = static_cast<wl_seat*>(
      wl_registry_bind(display->registry(), id, &wl_seat_interface, 1));
  DCHECK(seat_);
  wl_seat_add_listener(seat_, &kInputSeatListener, this);
  wl_seat_set_user_data(seat_, this);

#if defined(USE_DATA_DEVICE_MANAGER)
  if (display->GetDataDeviceManager())
    data_device_ = new WaylandDataDevice(display, seat_);
#endif
  text_input_ = new WaylandTextInput(this);
}

WaylandSeat::~WaylandSeat() {
#if defined(USE_DATA_DEVICE_MANAGER)
  if (data_device_ != NULL)
    delete data_device_;
#endif
  delete input_keyboard_;
  delete input_pointer_;
  delete text_input_;
  if (input_touch_ != NULL) {
    delete input_touch_;
  }
  wl_seat_destroy(seat_);
}

void WaylandSeat::OnSeatCapabilities(void *data, wl_seat *seat, uint32_t caps) {
  WaylandSeat* device = static_cast<WaylandSeat*>(data);
  WaylandDisplay* display = WaylandDisplay::GetInstance();
  VLOG(3) << __func__ << " name=" << device->name_ << " caps=" << caps;
  if ((caps & WL_SEAT_CAPABILITY_KEYBOARD) && !device->input_keyboard_) {
    device->input_keyboard_ = new WaylandKeyboard();
    device->input_keyboard_->SetName(device->name_ + kKeyboardSuffix);
    device->input_keyboard_->OnSeatCapabilities(seat, caps);
    display->KeyboardAdded(device->input_keyboard_->GetId(),
                           device->input_keyboard_->GetName());
  } else if (!(caps & WL_SEAT_CAPABILITY_KEYBOARD) && device->input_keyboard_) {
    display->KeyboardRemoved(device->input_keyboard_->GetId());
    delete device->input_keyboard_;
    device->input_keyboard_ = NULL;
  }

  if ((caps & WL_SEAT_CAPABILITY_POINTER) && !device->input_pointer_) {
    device->input_pointer_ = new WaylandPointer();
    device->input_pointer_->OnSeatCapabilities(seat, caps);
    device->input_pointer_->SetName(device->name_ + kPointerSuffix);
    display->PointerAdded(device->input_pointer_->GetId(),
                          device->input_pointer_->GetName());
  } else if (!(caps & WL_SEAT_CAPABILITY_POINTER) && device->input_pointer_) {
    display->PointerRemoved(device->input_pointer_->GetId());
    delete device->input_pointer_;
    device->input_pointer_ = NULL;
  }

  if ((caps & WL_SEAT_CAPABILITY_TOUCH) && !device->input_touch_) {
    device->input_touch_ = new WaylandTouchscreen();
    device->input_touch_->OnSeatCapabilities(seat, caps);
    device->input_touch_->SetName(device->name_ + kTouchscreenSuffix);
    display->TouchscreenAdded(device->input_touch_->GetId(),
                              device->input_touch_->GetName());
  } else if (!(caps & WL_SEAT_CAPABILITY_TOUCH) && device->input_touch_) {
    display->TouchscreenRemoved(device->input_touch_->GetId());
    delete device->input_touch_;
    device->input_touch_ = NULL;
  }
}

void WaylandSeat::OnName(void* data, wl_seat* seat, const char* name) {
  WaylandSeat* device = static_cast<WaylandSeat*>(data);
  device->name_ = name;

  VLOG(3) << __func__ << " name=" << name;

  // A name change implies the need to update all the devices reported
  // to hotplug
  WaylandDisplay* display = WaylandDisplay::GetInstance();
  if (device->input_keyboard_) {
    device->input_keyboard_->SetName(device->name_ + kKeyboardSuffix);
    display->KeyboardRemoved(device->input_keyboard_->GetId());
    display->KeyboardAdded(device->input_keyboard_->GetId(),
                           device->input_keyboard_->GetName());
  }
  if (device->input_pointer_) {
    device->input_pointer_->SetName(device->name_ + kPointerSuffix);
    display->PointerRemoved(device->input_pointer_->GetId());
    display->PointerAdded(device->input_pointer_->GetId(),
                          device->input_pointer_->GetName());
  }
  if (device->input_touch_) {
    device->input_touch_->SetName(device->name_ + kTouchscreenSuffix);
    display->TouchscreenRemoved(device->input_touch_->GetId());
    display->TouchscreenAdded(device->input_touch_->GetId(),
                              device->input_touch_->GetName());
  }
}

void WaylandSeat::SetFocusWindowHandle(unsigned windowhandle) {
  focused_window_handle_ = windowhandle;
  WaylandWindow* window = NULL;
  if (windowhandle)
    window = WaylandDisplay::GetInstance()->GetWindow(windowhandle);
  text_input_->SetActiveWindow(window);
}

void WaylandSeat::SetGrabWindowHandle(unsigned windowhandle, uint32_t button) {
  grab_window_handle_ = windowhandle;
  grab_button_ = button;
}

void WaylandSeat::SetCursorBitmap(const std::vector<SkBitmap>& bitmaps,
                                  const gfx::Point& location) {
  if (!input_pointer_) {
    LOG(WARNING) << "Tried to change cursor without input configured";
    return;
  }
  input_pointer_->Cursor()->UpdateBitmap(
      bitmaps, location, WaylandDisplay::GetInstance()->GetSerial());
}

void WaylandSeat::MoveCursor(const gfx::Point& location) {
  if (!input_pointer_) {
    LOG(WARNING) << "Tried to move cursor without input configured";
    return;
  }

  input_pointer_->Cursor()->MoveCursor(
      location, WaylandDisplay::GetInstance()->GetSerial());
}

void WaylandSeat::ResetIme() {
  text_input_->ResetIme();
}

void WaylandSeat::ImeCaretBoundsChanged(gfx::Rect rect) {
  NOTIMPLEMENTED();
}

void WaylandSeat::ShowInputPanel(unsigned handle) {
  text_input_->ShowInputPanel(seat_, handle);
}

void WaylandSeat::HideInputPanel(ui::ImeHiddenType hidden_type) {
  text_input_->HideInputPanel(seat_, hidden_type);
}

void WaylandSeat::SetInputContentType(ui::InputContentType content_type,
                                      int text_input_flags,
                                      unsigned handle) {
#if defined(OS_WEBOS)
  text_input_->SetInputContentType(content_type, text_input_flags, handle);
#endif
}

void WaylandSeat::SetSurroundingText(const std::string& text,
                                     size_t cursor_position,
                                     size_t anchor_position) {
  text_input_->SetSurroundingText(text, cursor_position, anchor_position);
}

}  // namespace ozonewayland
