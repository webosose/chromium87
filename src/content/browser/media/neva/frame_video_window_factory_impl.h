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

#ifndef CONTENT_BROWSER_MEDIA_NEVA_FRAME_VIDEO_WINDOW_FACTORY_IMPL_
#define CONTENT_BROWSER_MEDIA_NEVA_FRAME_VIDEO_WINDOW_FACTORY_IMPL_

#include "content/public/common/neva/frame_video_window_factory.mojom.h"
#include "mojo/public/cpp/bindings/unique_receiver_set.h"
#include "ui/platform_window/neva/mojom/video_window_controller.mojom.h"

namespace content {

class RenderFrameHostImpl;

class FrameVideoWindowFactoryImpl
    : public content::mojom::FrameVideoWindowFactory {
 public:
  FrameVideoWindowFactoryImpl(RenderFrameHostImpl*);
  ~FrameVideoWindowFactoryImpl() override;
  void CreateVideoWindow(
      mojo::PendingRemote<ui::mojom::VideoWindowClient> client,
      mojo::PendingReceiver<ui::mojom::VideoWindow> receiver,
      const ui::VideoWindowParams& params) override;

 private:
  RenderFrameHostImpl* render_frame_host_impl_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_MEDIA_NEVA_FRAME_VIDEO_WINDOW_FACTORY_IMPL_
