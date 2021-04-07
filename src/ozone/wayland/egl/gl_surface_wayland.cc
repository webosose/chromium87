// Copyright 2016 The Chromium Authors. All rights reserved.
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

#include "ozone/wayland/egl/gl_surface_wayland.h"

#include <utility>

#include "ozone/wayland/display.h"
#include "ozone/wayland/window.h"
#include "third_party/khronos/EGL/egl.h"
#include "ui/ozone/common/egl_util.h"

namespace ozonewayland {

GLSurfaceWayland::GLSurfaceWayland(unsigned widget)
    : NativeViewGLSurfaceEGL(
          reinterpret_cast<EGLNativeWindowType>(
              WaylandDisplay::GetInstance()->GetEglWindow(widget)),
          nullptr),
      widget_(widget) {}

bool GLSurfaceWayland::Resize(const gfx::Size& size,
                              float scale_factor,
                              const gfx::ColorSpace& color_space,
                              bool has_alpha) {
  if (size_ == size)
    return true;

  WaylandWindow* window = WaylandDisplay::GetInstance()->GetWindow(widget_);
  DCHECK(window);
  window->Resize(size.width(), size.height());
  size_ = size;
  return true;
}

EGLConfig GLSurfaceWayland::GetConfig() {
  if (!config_) {
    GLint config_attribs[] = {EGL_BUFFER_SIZE,
                              32,
                              EGL_ALPHA_SIZE,
                              8,
                              EGL_BLUE_SIZE,
                              8,
                              EGL_GREEN_SIZE,
                              8,
                              EGL_RED_SIZE,
                              8,
                              EGL_RENDERABLE_TYPE,
                              EGL_OPENGL_ES2_BIT,
                              EGL_SURFACE_TYPE,
                              EGL_WINDOW_BIT,
                              EGL_NONE};
    config_ = ui::ChooseEGLConfig(GetDisplay(), config_attribs);
  }
  return config_;
}

gfx::SwapResult GLSurfaceWayland::SwapBuffers(PresentationCallback callback) {
  gfx::SwapResult result =
      NativeViewGLSurfaceEGL::SwapBuffers(std::move(callback));
  WaylandDisplay::GetInstance()->FlushDisplay();
  return result;
}

GLSurfaceWayland::~GLSurfaceWayland() {
  // Destroy surface first
  Destroy();
  // Then wl egl window if window instance is still around
  WaylandWindow* window = WaylandDisplay::GetInstance()->GetWindow(widget_);
  if (window) {
    window->DestroyAcceleratedWidget();
    WaylandDisplay::GetInstance()->FlushDisplay();
  }
}

}  // namespace ozonewayland
