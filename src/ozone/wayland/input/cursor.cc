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

#include "ozone/wayland/input/cursor.h"

#include <vector>

#include "base/logging.h"
#include "base/memory/unsafe_shared_memory_region.h"
#include "ozone/wayland/display.h"

namespace ozonewayland {

WaylandCursor::WaylandCursor() : input_pointer_(NULL),
    buffer_(NULL),
    width_(0),
    height_(0) {
  WaylandDisplay* display = WaylandDisplay::GetInstance();
  shm_ = display->GetShm();
  pointer_surface_ = wl_compositor_create_surface(display->GetCompositor());
}

WaylandCursor::~WaylandCursor() {
  wl_surface_destroy(pointer_surface_);
  if (buffer_)
    wl_buffer_destroy(buffer_);
}

void WaylandCursor::UpdateBitmap(const std::vector<SkBitmap>& cursor_image,
                                 const gfx::Point& location,
                                 uint32_t serial) {
  if (!input_pointer_)
    return;

  if (!cursor_image.size()) {
#if defined(OS_WEBOS)
    if (webos::lsm_cursor_hide_location == location)
      HideLSMCursor(serial);
    else if (webos::lsm_cursor_restore_location == location)
      RestoreLSMCursor(serial);
    else
#endif
      HideCursor(serial);

    return;
  }

  const SkBitmap& image = cursor_image[0];
  int width = image.width();
  int height = image.height();
  if (!width || !height) {
    HideCursor(serial);
    return;
  }

  if (!CreateSHMBuffer(width, height)) {
    LOG(INFO) << "Failed to create SHM buffer for Cursor Bitmap.";
    wl_pointer_set_cursor(input_pointer_, serial, NULL, 0, 0);
    return;
  }

  // The |bitmap| contains ARGB image, so just copy it.
  memcpy(sh_memory_.memory(), image.getPixels(), width_ * height_ * 4);

  wl_pointer_set_cursor(input_pointer_, serial, pointer_surface_,
                        location.x(), location.y());
  wl_surface_attach(pointer_surface_, buffer_, 0, 0);
  wl_surface_damage(pointer_surface_, 0, 0, width_, height_);
  wl_surface_commit(pointer_surface_);
}

void WaylandCursor::MoveCursor(const gfx::Point& location, uint32_t serial) {
  if (!input_pointer_)
    return;

  wl_pointer_set_cursor(input_pointer_, serial, pointer_surface_,
                         location.x(), location.y());
}

bool WaylandCursor::CreateSHMBuffer(int width, int height) {
  if (width == width_ && height == height_)
    return true;

  struct wl_shm_pool *pool;
  int size, stride;

  width_ = width;
  height_ = height;
  stride = width_ * 4;
  SkImageInfo info = SkImageInfo::MakeN32Premul(width_, height_);
  size = info.computeByteSize(stride);

  base::UnsafeSharedMemoryRegion shared_memory_region =
      base::UnsafeSharedMemoryRegion::Create(size);
  sh_memory_ = shared_memory_region.Map();
  if (!sh_memory_.IsValid()) {
    LOG(INFO) << "Create and mmap failed.";
    return false;
  }
  base::subtle::PlatformSharedMemoryRegion platform_shared_memory =
      base::UnsafeSharedMemoryRegion::TakeHandleForSerialization(
          std::move(shared_memory_region));

  pool = wl_shm_create_pool(shm_,
                            platform_shared_memory.PassPlatformHandle().fd.release(),
                            sh_memory_.size());

  buffer_ = wl_shm_pool_create_buffer(pool, 0,
                                      width_, height_,
                                      stride, WL_SHM_FORMAT_ARGB8888);
  wl_shm_pool_destroy(pool);
  return true;
}

void WaylandCursor::HideCursor(uint32_t serial) {
  width_ = 0;
  height_ = 0;
  wl_pointer_set_cursor(input_pointer_, serial, NULL, 0, 0);

  if (buffer_) {
    wl_buffer_destroy(buffer_);
    buffer_ = NULL;
  }
}

void WaylandCursor::SetInputPointer(wl_pointer* pointer) {
  if (input_pointer_ == pointer)
    return;

  if (input_pointer_)
    wl_pointer_destroy(input_pointer_);

  input_pointer_ = pointer;
}

#if defined(OS_WEBOS)
void WaylandCursor::HideLSMCursor(uint32_t serial) {
  wl_pointer_set_cursor(input_pointer_, serial, pointer_surface_,
                        webos::lsm_cursor_hide_location.x(),
                        webos::lsm_cursor_hide_location.y());
  wl_surface_commit(pointer_surface_);
}

void WaylandCursor::RestoreLSMCursor(uint32_t serial) {
  wl_pointer_set_cursor(input_pointer_, serial, pointer_surface_,
                        webos::lsm_cursor_restore_location.x(),
                        webos::lsm_cursor_restore_location.y());
  wl_surface_commit(pointer_surface_);
}
#endif

}  // namespace ozonewayland
