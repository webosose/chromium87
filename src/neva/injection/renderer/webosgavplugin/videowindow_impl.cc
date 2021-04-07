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

#include "neva/injection/renderer/webosgavplugin/videowindow_impl.h"

namespace injections {

VideoWindowImpl::VideoWindowImpl(
    VideoWindowClientOwner* owner,
    const std::string& id,
    int type,
    mojo::PendingRemote<ui::mojom::VideoWindow> window_remote,
    mojo::PendingReceiver<ui::mojom::VideoWindowClient> pending_receiver)
    : owner_(owner),
      id_(id),
      type_(type),
      window_remote_(std::move(window_remote)) {
  receiver_.Bind(std::move(pending_receiver));
  VLOG(1) << "[" << this << "] " << __func__;
}

VideoWindowImpl::~VideoWindowImpl() {
  VLOG(1) << "[" << this << "] " << __func__;
}

void VideoWindowImpl::OnVideoWindowCreated(const ui::VideoWindowInfo& info) {
  VLOG(1) << __func__ << " id=" << id_;
  info_ = info;
  owner_->OnVideoWindowCreated(id_, info.native_window_id, type_);
}

void VideoWindowImpl::OnVideoWindowDestroyed() {
  VLOG(1) << __func__ << " id=" << id_;
  owner_->OnVideoWindowDestroyed(id_);
}

ui::mojom::VideoWindow* VideoWindowImpl::GetVideoWindow() {
  return window_remote_.get();
}

}  // namespace injections
