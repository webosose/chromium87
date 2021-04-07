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

#ifndef OZONE_WAYLAND_EGL_GL_SURFACE_WAYLAND_H_
#define OZONE_WAYLAND_EGL_GL_SURFACE_WAYLAND_H_

#include <memory>

#include "base/macros.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gl/gl_surface_egl.h"

namespace ozonewayland {

// GLSurface class implementation for wayland.
class GLSurfaceWayland : public gl::NativeViewGLSurfaceEGL {
 public:
  explicit GLSurfaceWayland(unsigned widget);

  // gl::GLSurface:
  bool Resize(const gfx::Size& size,
              float scale_factor,
              const gfx::ColorSpace& color_space,
              bool has_alpha) override;
  EGLConfig GetConfig() override;
  gfx::SwapResult SwapBuffers(PresentationCallback callback) override;

 private:
  ~GLSurfaceWayland() override;

  unsigned widget_;

  DISALLOW_COPY_AND_ASSIGN(GLSurfaceWayland);
};

}  // namespace ozonewayland

#endif  // OZONE_WAYLAND_EGL_GL_SURFACE_WAYLAND_H_
