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

#ifndef OZONE_WAYLAND_INPUT_CURSOR_H_
#define OZONE_WAYLAND_INPUT_CURSOR_H_

#include <wayland-client.h>

#include <vector>

#include "base/macros.h"
#include "base/memory/shared_memory_mapping.h"
#include "third_party/skia/include/core/SkBitmap.h"

namespace gfx {
class Point;
}

namespace ozonewayland {

class WaylandCursor {
 public:
  WaylandCursor();
  ~WaylandCursor();

  void UpdateBitmap(const std::vector<SkBitmap>& bitmaps,
                    const gfx::Point& location,
                    uint32_t serial);
  void MoveCursor(const gfx::Point& location, uint32_t serial);

  wl_pointer* GetInputPointer() const { return input_pointer_; }
  void SetInputPointer(wl_pointer* pointer);

 private:
  bool CreateSHMBuffer(int width, int height);
  void HideCursor(uint32_t serial);

#if defined(OS_WEBOS)
  void HideLSMCursor(uint32_t serial);
  void RestoreLSMCursor(uint32_t serial);
#endif

  struct wl_pointer* input_pointer_;
  struct wl_surface* pointer_surface_;
  struct wl_buffer* buffer_;
  struct wl_shm* shm_;
  base::WritableSharedMemoryMapping sh_memory_;
  int width_;
  int height_;
  DISALLOW_COPY_AND_ASSIGN(WaylandCursor);
};

}  // namespace ozonewayland

#endif  // OZONE_WAYLAND_INPUT_CURSOR_H_
