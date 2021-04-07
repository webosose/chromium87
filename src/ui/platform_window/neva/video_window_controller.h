// Copyright 2019 LG Electronics, Inc.
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
#ifndef UI_PLATFORM_WINDOW_NEVA_VIDEO_WINDOW_CONTROLLER_H_
#define UI_PLATFORM_WINDOW_NEVA_VIDEO_WINDOW_CONTROLLER_H_

#include "base/single_thread_task_runner.h"
#include "base/unguessable_token.h"
#include "gpu/ipc/common/surface_handle.h"
#include "ui/gfx/geometry/rect.h"

namespace ui {

/* VideoWindowController lives in gpu process and it request
 * geomtry-update VideoWindow to VideoWindowProvider */
class VideoWindowController {
 public:
  VideoWindowController() {}
  virtual ~VideoWindowController() {}
  // Initialize VideoWindowController. |task_runner| will be dedicated
  // TaskRunner. All public interfaces of VideoWindowController will be done
  // on the given |task_runner|. If a function is called by another thread,
  // then task_runner->PostTask() will be called.
  virtual void Initialize(
      const scoped_refptr<base::SingleThreadTaskRunner>& task_runner) = 0;
  virtual bool IsInitialized() = 0;
  virtual void NotifyVideoWindowGeometryChanged(
      gpu::SurfaceHandle h,
      const base::UnguessableToken& window_id,
      const gfx::Rect& rect) = 0;
  virtual void BeginOverlayProcessor(gpu::SurfaceHandle h) = 0;
  virtual void EndOverlayProcessor(gpu::SurfaceHandle h) = 0;
};

}  // namespace ui

#endif  // UI_PLATFORM_WINDOW_NEVA_VIDEO_WINDOW_CONTROLLER_H_
