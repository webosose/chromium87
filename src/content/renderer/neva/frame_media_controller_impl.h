// Copyright 2018-2019 LG Electronics, Inc.
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

#ifndef CONTENT_RENDERER_NEVA_FRAME_MEDIA_CONTROLLER_IMPL_H_
#define CONTENT_RENDERER_NEVA_FRAME_MEDIA_CONTROLLER_IMPL_H_

#include "base/observer_list.h"
#include "content/common/media/neva/frame_media_controller.mojom.h"
#include "mojo/public/cpp/bindings/associated_receiver.h"
#include "ui/gfx/geometry/rect.h"

namespace content {

class RenderFrameImpl;

namespace neva {

class FrameMediaControllerImpl : public content::mojom::FrameMediaController {
 public:
  FrameMediaControllerImpl(RenderFrameImpl* render_frame_impl);
  ~FrameMediaControllerImpl() override;

  void PermitMediaActivation(int player_id) override;
  void SetSuppressed(bool is_suppressed) override;
  void SuspendMedia(int player_id) override;

  void Bind(
      mojo::PendingAssociatedReceiver<content::mojom::FrameMediaController>);

 private:
  mojo::AssociatedReceiver<content::mojom::FrameMediaController>
      frame_media_controller_receiver_{this};
  RenderFrameImpl* render_frame_impl_;
};

}  // namespace neva
}  // namespace content

#endif  // CONTENT_RENDERER_NEVA_FRAME_MEDIA_CONTROLLER_IMPL_H_
