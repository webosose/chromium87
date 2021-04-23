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

#include <map>
#include <vector>

#include "base/macros.h"
#include "ozone/platform/input_content_type.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/base/ime/neva/input_method_common.h"
#include "ui/gfx/geometry/rect.h"

namespace ozonewayland {

class WaylandDataDevice;
class WaylandKeyboard;
class WaylandPointer;
class WaylandDisplay;
class WaylandTouchscreen;
class WaylandTextInput;

struct GrabWindowInfo {
  GrabWindowInfo() = default;
  ~GrabWindowInfo() = default;
  GrabWindowInfo(unsigned window_handle, uint32_t button)
      : grab_window_handle(window_handle), grab_button(button) {}
  unsigned grab_window_handle = 0;
  uint32_t grab_button = 0;
};

class WaylandSeat {
 public:
  WaylandSeat(WaylandDisplay* display, uint32_t id);
  ~WaylandSeat();

  wl_seat* GetWLSeat() const { return seat_; }
#if defined(USE_DATA_DEVICE_MANAGER)
  WaylandDataDevice* GetDataDevice() const { return data_device_; }
#endif
  std::string GetName() const { return name_; }
  WaylandKeyboard* GetKeyBoard() const { return input_keyboard_; }
  WaylandPointer* GetPointer() const { return input_pointer_; }
  WaylandTouchscreen* GetTouchscreen() const { return input_touch_; }
  WaylandTextInput* GetTextInput() const { return text_input_; }
  unsigned GetActiveInputWindow() const { return active_input_window_handle_; }
  unsigned GetEnteredWindowHandle(uint32_t device_id) const;
  void ResetEnteredWindowHandle(unsigned window_handle);
  unsigned GetGrabWindowHandle(uint32_t device_id) const;
  uint32_t GetGrabButton(uint32_t device_id) const;
  void ResetGrabWindow(unsigned window_handle);
  void SetEnteredWindowHandle(uint32_t device_id, unsigned windowhandle);
  void SetActiveInputWindow(unsigned windowhandle);
  void SetGrabWindow(uint32_t device_id, const GrabWindowInfo& grab_window);
  void SetCursorBitmap(const std::vector<SkBitmap>& bitmaps,
                       const gfx::Point& location);
  void MoveCursor(const gfx::Point& location);

  void ResetIme(unsigned handle);
  void ImeCaretBoundsChanged(gfx::Rect rect);
  void ShowInputPanel(unsigned handle);
  void HideInputPanel(unsigned handle, ui::ImeHiddenType);
  void SetInputContentType(ui::InputContentType content_type,
                           int text_input_flags,
                           unsigned handle);
  void SetSurroundingText(unsigned handle,
                          const std::string& text,
                          size_t cursor_position,
                          size_t anchor_position);

 private:
  static void OnSeatCapabilities(void *data,
                                 wl_seat *seat,
                                 uint32_t caps);
  static void OnName(void* data, wl_seat* seat, const char* name);

  // Keeps track of current focused window.
  unsigned active_input_window_handle_;
  std::map<uint32_t, unsigned> entered_window_handle_map_;
  std::map<uint32_t, GrabWindowInfo> grab_window_map_;
  struct wl_seat* seat_;
  std::string name_;
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
