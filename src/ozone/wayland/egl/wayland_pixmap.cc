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

#include "ozone/wayland/egl/wayland_pixmap.h"

#include <gbm.h>

#include "base/logging.h"

namespace ozonewayland {
namespace {

int GetGbmFormatFromBufferFormat(ui::SurfaceFactoryOzone::BufferFormat fmt) {
  switch (fmt) {
    case ui::SurfaceFactoryOzone::RGBA_8888:
      return GBM_BO_FORMAT_ARGB8888;
    case ui::SurfaceFactoryOzone::RGBX_8888:
      return GBM_BO_FORMAT_XRGB8888;
    default:
      NOTREACHED();
      return 0;
  }
}

}  // namespace

WaylandPixmap::WaylandPixmap()
    : bo_(NULL), dma_buf_(-1)  {
}

bool WaylandPixmap::Initialize(gbm_device* device,
                               ui::SurfaceFactoryOzone::BufferFormat format,
                               const gfx::Size& size) {
  unsigned flags = GBM_BO_USE_RENDERING;
  bo_ = gbm_bo_create(device,
                      size.width(),
                      size.height(),
                      GetGbmFormatFromBufferFormat(format),
                      flags);
  if (!bo_) {
    LOG(ERROR) << "Failed to create GBM buffer object.";
    return false;
  }

  dma_buf_ = gbm_bo_get_fd(bo_);
  if (dma_buf_ < 0) {
    LOG(ERROR) << "Failed to export buffer to dma_buf.";
    return false;
  }

  return true;
}

WaylandPixmap::~WaylandPixmap() {
  if (bo_)
    gbm_bo_destroy(bo_);

  if (dma_buf_ > 0)
    close(dma_buf_);
}

void* WaylandPixmap::GetEGLClientBuffer() {
  return bo_;
}

int WaylandPixmap::GetDmaBufFd() {
  return dma_buf_;
}

int WaylandPixmap::GetDmaBufPitch() {
  return gbm_bo_get_stride(bo_);
}

}  // namespace ozonewayland
