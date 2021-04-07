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

#ifndef OZONE_WAYLAND_INPUT_KEYBOARD_H_
#define OZONE_WAYLAND_INPUT_KEYBOARD_H_

#include "ozone/wayland/display.h"

namespace ozonewayland {

class WaylandKeyboard {
 public:
  WaylandKeyboard();
  ~WaylandKeyboard();

  void OnSeatCapabilities(wl_seat *seat, uint32_t caps);

 private:
  static void OnKeyNotify(void* data,
                          wl_keyboard* input_keyboard,
                          uint32_t serial,
                          uint32_t time,
                          uint32_t key,
                          uint32_t state);

  static void OnKeyboardKeymap(void* data,
                               struct wl_keyboard* keyboard,
                               uint32_t format,
                               int32_t raw_fd,
                               uint32_t size);

  static void OnKeyboardEnter(void* data,
                              wl_keyboard* input_keyboard,
                              uint32_t serial,
                              wl_surface* surface,
                              wl_array* keys);

  static void OnKeyboardLeave(void* data,
                              wl_keyboard* input_keyboard,
                              uint32_t serial,
                              wl_surface* surface);

  static void OnKeyModifiers(void *data,
                             wl_keyboard *keyboard,
                             uint32_t serial,
                             uint32_t mods_depressed,
                             uint32_t mods_latched,
                             uint32_t mods_locked,
                             uint32_t group);

  wl_keyboard* input_keyboard_;
  WaylandDisplay* dispatcher_;

  DISALLOW_COPY_AND_ASSIGN(WaylandKeyboard);
};

}  // namespace ozonewayland

#endif  // OZONE_WAYLAND_INPUT_KEYBOARD_H_
