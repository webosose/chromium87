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
#ifndef OZONE_MEDIA_VIDEO_WINDOW_CONTROLLER_IMPL_H_
#define OZONE_MEDIA_VIDEO_WINDOW_CONTROLLER_IMPL_H_

#include <map>
#include <set>

#include "base/macros.h"
#include "base/memory/singleton.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "base/unguessable_token.h"
#include "gpu/ipc/common/surface_handle.h"
#include "ipc/ipc_message.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "ozone/media/video_window_provider.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/platform_window/neva/mojom/video_window_controller.mojom.h"
#include "ui/platform_window/neva/video_window_controller.h"

namespace ui {
/* VideoWindowControllerImpl lives in gpu process and it request
 * creating/destroying/geomtry-update VideoWindow to VideoWindowProvider */
class VideoWindowControllerImpl : public ui::mojom::VideoWindowController,
                                  public VideoWindowController {
 public:
  VideoWindowControllerImpl();
  ~VideoWindowControllerImpl() override;

  // Implements ui::mojom::VideoWindowController
  void CreateVideoWindow(
      gfx::AcceleratedWidget w,
      mojo::PendingRemote<ui::mojom::VideoWindowClient> client,
      mojo::PendingReceiver<ui::mojom::VideoWindow> receiver,
      const VideoWindowParams& param) override;

  // Implements VideoWindowController
  void Initialize(
      const scoped_refptr<base::SingleThreadTaskRunner>& task_runner) override;
  bool IsInitialized() override;
  void NotifyVideoWindowGeometryChanged(gpu::SurfaceHandle h,
                                        const base::UnguessableToken& window_id,
                                        const gfx::Rect& rect) override;
  void BeginOverlayProcessor(gpu::SurfaceHandle h) override;
  void EndOverlayProcessor(gpu::SurfaceHandle h) override;
  void AcceleratedWidgetDeleted(gfx::AcceleratedWidget w);
  void OwnerWidgetStateChanged(gfx::AcceleratedWidget w, ui::WidgetState state);

  void Bind(mojo::PendingReceiver<ui::mojom::VideoWindowController> receiver);

 private:
  class VideoWindowInfo;
  VideoWindowInfo* FindVideoWindowInfo(const base::UnguessableToken& window_id);
  void RemoveVideoWindowInfo(const base::UnguessableToken& window_id);
  void SetVideoWindowVisibility(const base::UnguessableToken& window_id,
                                bool visibility);
  void OnWindowEvent(gfx::AcceleratedWidget w,
                     const base::UnguessableToken& window_id,
                     VideoWindowProvider::Event event);
  void OnVideoWindowCreated(gfx::AcceleratedWidget w,
                            const base::UnguessableToken& window_id);
  void OnVideoWindowDestroyed(gfx::AcceleratedWidget w,
                              const base::UnguessableToken& window_id);

  using VideoWindowInfoList = std::vector<std::unique_ptr<VideoWindowInfo>>;

  std::unique_ptr<VideoWindowProvider> provider_;
  std::map<base::UnguessableToken, gfx::AcceleratedWidget> id_to_widget_map_;
  std::map<gfx::AcceleratedWidget, VideoWindowInfoList> video_windows_;
  std::map<gfx::AcceleratedWidget, std::set<base::UnguessableToken>>
      hidden_candidate_;

  mojo::Receiver<ui::mojom::VideoWindowController> receiver_{this};
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
  bool initialized_ = false;
  DISALLOW_COPY_AND_ASSIGN(VideoWindowControllerImpl);
};

}  // namespace ui

#endif  // OZONE_MEDIA_VIDEO_WINDOW_CONTROLLER_IMPL_H_
