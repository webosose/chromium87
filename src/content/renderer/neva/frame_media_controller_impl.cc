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

#include "content/renderer/neva/frame_media_controller_impl.h"

#include "content/renderer/render_frame_impl.h"
#include "third_party/blink/public/web/web_local_frame.h"

namespace content {
namespace neva {

FrameMediaControllerImpl::FrameMediaControllerImpl(
    RenderFrameImpl* render_frame_impl)
    : render_frame_impl_(render_frame_impl) {}

FrameMediaControllerImpl::~FrameMediaControllerImpl() {}

void FrameMediaControllerImpl::Bind(
    mojo::PendingAssociatedReceiver<content::mojom::FrameMediaController>
        receiver) {
  frame_media_controller_receiver_.Bind(std::move(receiver));
}

void FrameMediaControllerImpl::PermitMediaActivation(int player_id) {
  for (auto& observer : render_frame_impl_->observers_) {
    observer.OnMediaActivationPermitted(player_id);
  }
}

void FrameMediaControllerImpl::SetSuppressed(bool is_suppressed) {
  blink::WebLocalFrame* web_frame = render_frame_impl_->GetWebFrame();
  if (web_frame) {
    web_frame->SetSuppressMediaPlay(is_suppressed);
  }

  // TODO(neva): Remove this when legacy implementation is deprecated
  for (auto& observer : render_frame_impl_->observers_) {
    observer.OnSuppressedMediaPlay(is_suppressed);
  }
}

void FrameMediaControllerImpl::SuspendMedia(int player_id) {
  for (auto& observer : render_frame_impl_->observers_) {
    observer.OnSuspendMedia(player_id);
  }
}

}  // namespace neva
}  // namespace content
