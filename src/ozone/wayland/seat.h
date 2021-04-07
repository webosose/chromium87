// Copyright 2013 The Chromium Authors. All rights reserved.
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

#ifndef OZONE_WAYLAND_SEAT_H_
#define OZONE_WAYLAND_SEAT_H_

#include <wayland-client.h>

#include <vector>

#include "base/macros.h"
#include "ozone/platform/input_content_type.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/geometry/rect.h"

namespace ozonewayland {

class WaylandDataDevice;
class WaylandKeyboard;
class WaylandPointer;
class WaylandDisplay;
class WaylandTouchscreen;
class WaylandTextInput;

class WaylandSeat {
 public:
  WaylandSeat(WaylandDisplay* display, uint32_t id);
  ~WaylandSeat();

  wl_seat* GetWLSeat() const { return seat_; }
#if defined(USE_DATA_DEVICE_MANAGER)
  WaylandDataDevice* GetDataDevice() const { return data_device_; }
#endif
  WaylandKeyboard* GetKeyBoard() const { return input_keyboard_; }
  WaylandPointer* GetPointer() const { return input_pointer_; }
  WaylandTextInput* GetTextInput() const { return text_input_; }
  unsigned GetFocusWindowHandle() const { return focused_window_handle_; }
  unsigned GetGrabWindowHandle() const { return grab_window_handle_; }
  uint32_t GetGrabButton() const { return grab_button_; }
  void SetFocusWindowHandle(unsigned windowhandle);
  void SetGrabWindowHandle(unsigned windowhandle, uint32_t button);
  void SetCursorBitmap(const std::vector<SkBitmap>& bitmaps,
                       const gfx::Point& location);
  void MoveCursor(const gfx::Point& location);

  void ResetIme();
  void ImeCaretBoundsChanged(gfx::Rect rect);
  void ShowInputPanel(unsigned handle);
  void HideInputPanel();
  void SetInputContentType(ui::InputContentType content_type,
                           int text_input_flags,
                           unsigned handle);
  void SetSurroundingText(const std::string& text,
                          size_t cursor_position,
                          size_t anchor_position);

 private:
  static void OnSeatCapabilities(void *data,
                                 wl_seat *seat,
                                 uint32_t caps);

  // Keeps track of current focused window.
  unsigned focused_window_handle_;
  unsigned grab_window_handle_;
  uint32_t grab_button_;
  struct wl_seat* seat_;
#if defined(USE_DATA_DEVICE_MANAGER)
  WaylandDataDevice* data_device_;
#endif
  WaylandKeyboard* input_keyboard_;
  WaylandPointer* input_pointer_;
  WaylandTouchscreen* input_touch_;
  WaylandTextInput* text_input_;

  DISALLOW_COPY_AND_ASSIGN(WaylandSeat);
};

}  // namespace ozonewayland

#endif  // OZONE_WAYLAND_SEAT_H_
