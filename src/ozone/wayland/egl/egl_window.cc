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

#include "ozone/wayland/egl/egl_window.h"

#include <EGL/egl.h>
#include <wayland-egl.h>

namespace ozonewayland {

EGLWindow::EGLWindow(struct wl_surface* surface, int32_t width, int32_t height)
    : window_(NULL) {
  window_ = wl_egl_window_create(surface, width, height);
}

EGLWindow::~EGLWindow() {
  wl_egl_window_destroy(window_);
}

void EGLWindow::Resize(int width, int height) {
  wl_egl_window_resize(window_, width, height, 0, 0);
}

void EGLWindow::Move(int width, int height, int x, int y) {
  wl_egl_window_resize(window_, width, height, x, y);
}

}  // namespace ozonewayland
