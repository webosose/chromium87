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

#include "content/browser/media/neva/frame_video_window_factory_impl.h"

#include "base/logging.h"
#include "content/browser/renderer_host/render_frame_host_impl.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/gpu_service_registry.h"
#include "mojo/public/cpp/bindings/remote.h"

namespace content {

FrameVideoWindowFactoryImpl::FrameVideoWindowFactoryImpl(
    RenderFrameHostImpl* rfhi)
    : render_frame_host_impl_(rfhi) {}

FrameVideoWindowFactoryImpl::~FrameVideoWindowFactoryImpl() = default;

void FrameVideoWindowFactoryImpl::CreateVideoWindow(
    mojo::PendingRemote<ui::mojom::VideoWindowClient> client,
    mojo::PendingReceiver<ui::mojom::VideoWindow> receiver,
    const ui::VideoWindowParams& params) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  gfx::AcceleratedWidget owner =
      render_frame_host_impl_->GetAcceleratedWidget();

  mojo::Remote<ui::mojom::VideoWindowController> controller;
  content::BindInterfaceInGpuProcess(controller.BindNewPipeAndPassReceiver());

  controller->CreateVideoWindow(owner, std::move(client), std::move(receiver),
                                params);
}

}  // namespace content
