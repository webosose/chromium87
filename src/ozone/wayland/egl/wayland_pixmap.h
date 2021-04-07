// Copyright 2014 The Chromium Authors. All rights reserved.
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

#ifndef OZONE_WAYLAND_EGL_WAYLAND_PIXMAP_H_
#define OZONE_WAYLAND_EGL_WAYLAND_PIXMAP_H_

#include "base/macros.h"
#include "ui/gfx/geometry/size.h"
#include "ui/ozone/public/native_pixmap.h"
#include "ui/ozone/public/surface_factory_ozone.h"

struct gbm_bo;
struct gbm_device;

namespace ozonewayland {

class WaylandPixmap : public ui::NativePixmap {
 public:
  WaylandPixmap();
  bool Initialize(gbm_device* device,
                  ui::SurfaceFactoryOzone::BufferFormat format,
                  const gfx::Size& size);

  // NativePixmap:
  void* GetEGLClientBuffer() override;
  int GetDmaBufFd() override;
  int GetDmaBufPitch() override;

 private:
  ~WaylandPixmap() override;

  gbm_bo* bo_;
  int dma_buf_;

  DISALLOW_COPY_AND_ASSIGN(WaylandPixmap);
};

}  // namespace ozonewayland

#endif  // OZONE_WAYLAND_EGL_WAYLAND_PIXMAP_H_
