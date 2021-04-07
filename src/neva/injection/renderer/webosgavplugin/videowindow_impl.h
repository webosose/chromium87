// Copyright 2020 LG Electronics, Inc.
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

#ifndef NEVA_INJECTION_RENDERER_WEBOSGAVPLUGIN_VIDEOWINDOW_IMPL_H_
#define NEVA_INJECTION_RENDERER_WEBOSGAVPLUGIN_VIDEOWINDOW_IMPL_H_

#include <string>

#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "ui/platform_window/neva/mojom/video_window_controller.mojom.h"

namespace injections {

class VideoWindowClientOwner {
 public:
  virtual void OnVideoWindowCreated(const std::string& id,
                                    const std::string& video_window_id,
                                    int type) = 0;
  virtual void OnVideoWindowDestroyed(const std::string& id) = 0;
};

struct VideoWindowInfo {
  VideoWindowInfo(const base::UnguessableToken& id, const std::string& nid)
      : window_id(id), native_window_id(nid) {}
  base::UnguessableToken window_id;
  std::string native_window_id;
};

class VideoWindowImpl : public ui::mojom::VideoWindowClient {
 public:
  VideoWindowImpl(
      VideoWindowClientOwner* owner,
      const std::string& id,
      int type,
      mojo::PendingRemote<ui::mojom::VideoWindow> window_remote,
      mojo::PendingReceiver<ui::mojom::VideoWindowClient> pending_receiver);

  ~VideoWindowImpl() override;

  void OnVideoWindowCreated(const ui::VideoWindowInfo& info) final;
  void OnVideoWindowDestroyed() final;

  // Not used in gav
  void OnVideoWindowGeometryChanged(const gfx::Rect& rect) final {}
  // Not used in gav
  void OnVideoWindowVisibilityChanged(bool visibility) final {}

  ui::mojom::VideoWindow* GetVideoWindow();

  VideoWindowClientOwner* owner_;
  std::string id_;
  int type_;
  mojo::Remote<ui::mojom::VideoWindow> window_remote_;
  mojo::Receiver<ui::mojom::VideoWindowClient> receiver_{this};
  ui::VideoWindowInfo info_{base::UnguessableToken::Null(), std::string()};
};

}  // namespace injections

#endif  // NEVA_INJECTION_RENDERER_WEBOSGAVPLUGIN_VIDEOWINDOW_IMPL_H_
