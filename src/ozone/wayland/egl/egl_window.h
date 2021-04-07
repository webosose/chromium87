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

#ifndef OZONE_WAYLAND_EGL_EGL_WINDOW_H_
#define OZONE_WAYLAND_EGL_EGL_WINDOW_H_

#include <wayland-client.h>

#include "base/macros.h"

struct wl_egl_window;

namespace ozonewayland {

class WaylandSurface;
class EGLWindow {
 public:
  EGLWindow(struct wl_surface* surface, int32_t width, int32_t height);
  ~EGLWindow();

  wl_egl_window* egl_window() const { return window_; }
  void Resize(int width, int height);
  void Move(int width, int height, int x, int y);

 private:
  struct wl_egl_window* window_;
  DISALLOW_COPY_AND_ASSIGN(EGLWindow);
};

}  // namespace ozonewayland

#endif  // OZONE_WAYLAND_EGL_EGL_WINDOW_H_
