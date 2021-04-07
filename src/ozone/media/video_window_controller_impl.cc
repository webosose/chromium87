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

#include "ozone/media/video_window_controller_impl.h"

#include "base/bind.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "ozone/media/video_window_provider.h"
#include "ozone/platform/messages.h"
#include "ui/ozone/public/ozone_platform.h"

namespace ui {

class VideoWindowControllerImpl::VideoWindowInfo {
 public:
  enum class State { kCreating, kCreated, kDestroyed };

  explicit VideoWindowInfo(gfx::AcceleratedWidget w,
                           const base::UnguessableToken& id,
                           const VideoWindowParams& params)
      : owner_widget_(w), id_(id), params_(params) {}
  ~VideoWindowInfo() = default;

  gfx::AcceleratedWidget owner_widget_;
  base::UnguessableToken id_;
  base::Optional<bool> visibility_ = base::nullopt;
  State state_ = State::kCreating;
  VideoWindowParams params_;
};

VideoWindowControllerImpl::VideoWindowControllerImpl() = default;

VideoWindowControllerImpl::~VideoWindowControllerImpl() = default;

void VideoWindowControllerImpl::CreateVideoWindow(
    gfx::AcceleratedWidget w,
    mojo::PendingRemote<ui::mojom::VideoWindowClient> client,
    mojo::PendingReceiver<ui::mojom::VideoWindow> receiver,
    const ui::VideoWindowParams& params) {
  if (!provider_) {
    LOG(ERROR) << "Not initialized.";
    return;
  }
  base::UnguessableToken window_id = provider_->CreateNativeVideoWindow(
      w, std::move(client), std::move(receiver), params,
      base::BindRepeating(&VideoWindowControllerImpl::OnWindowEvent,
                          base::Unretained(this)));
  if (window_id == base::UnguessableToken::Null()) {
    LOG(ERROR) << "Not created.";
    return;
  }

  VideoWindowInfo* info = new VideoWindowInfo(w, window_id, params);
  id_to_widget_map_[window_id] = w;
  VideoWindowInfoList& list = video_windows_[w];
  list.emplace_back(info);
}

VideoWindowControllerImpl::VideoWindowInfo*
VideoWindowControllerImpl::FindVideoWindowInfo(
    const base::UnguessableToken& window_id) {
  auto widget_it = id_to_widget_map_.find(window_id);
  if (widget_it == id_to_widget_map_.end())
    return nullptr;
  auto wl_it = video_windows_.find(widget_it->second);
  if (wl_it == video_windows_.end())
    return nullptr;
  for (auto& window : wl_it->second)
    if (window->id_ == window_id)
      return window.get();
  return nullptr;
}

void VideoWindowControllerImpl::RemoveVideoWindowInfo(
    const base::UnguessableToken& window_id) {
  auto widget_it = id_to_widget_map_.find(window_id);
  if (widget_it == id_to_widget_map_.end()) {
    LOG(INFO) << __func__ << " failed to find widget";
    return;
  }
  gfx::AcceleratedWidget w = widget_it->second;
  id_to_widget_map_.erase(window_id);

  auto wl_it = video_windows_.find(w);
  if (wl_it == video_windows_.end()) {
    LOG(INFO) << __func__ << " failed to find info for widget";
    return;
  }

  int count = wl_it->second.size();

  for (auto it = wl_it->second.cbegin(); it != wl_it->second.cend(); it++) {
    if ((*it)->id_ == window_id) {
      wl_it->second.erase(it);
      count--;
      break;
    }
  }

  LOG(INFO) << __func__ << " total # of windows:" << id_to_widget_map_.size()
            << " / # of windows of widget(" << w << "):" << count;
}

void VideoWindowControllerImpl::Initialize(
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner) {
  task_runner_ = task_runner;
  initialized_ = true;
}

bool VideoWindowControllerImpl::IsInitialized() {
  return initialized_;
}

// NotifyVideoWindowGeometryChanged is called from viz/Display overlay processor
// Now this will work only with --in-process-gpu command-line flag.
// But chromium will move viz/Display to GPU process then it will work without
// --in-process-gpu as well.
void VideoWindowControllerImpl::NotifyVideoWindowGeometryChanged(
    const gpu::SurfaceHandle h,
    const base::UnguessableToken& window_id,
    const gfx::Rect& rect) {
  if (!provider_) {
    LOG(ERROR) << "Not initialized.";
    return;
  }

  if (task_runner_ && !task_runner_->BelongsToCurrentThread()) {
    task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(
            &VideoWindowControllerImpl::NotifyVideoWindowGeometryChanged,
            base::Unretained(this), h, window_id, rect));
    return;
  }

  gfx::AcceleratedWidget w = static_cast<gfx::AcceleratedWidget>(h);

  auto it = hidden_candidate_.find(w);
  if (it != hidden_candidate_.end()) {
    auto& hidden_candidate = it->second;
    hidden_candidate.erase(window_id);
  }

  SetVideoWindowVisibility(window_id, true);

  VideoWindowInfo* info = FindVideoWindowInfo(window_id);
  if (info && info->params_.use_overlay_processor_layout) {
    VLOG(2) << __func__ << " window_id=" << window_id
            << " rect=" << rect.ToString();
    provider_->NativeVideoWindowGeometryChanged(window_id, rect);
  }
}

void VideoWindowControllerImpl::SetVideoWindowVisibility(
    const base::UnguessableToken& window_id,
    bool visibility) {
  // provider_ is already checked from
  // VideoWindowControllerImpl::NotifyVideoWindowGeometryChanged
  DCHECK(provider_);
  VideoWindowInfo* w = FindVideoWindowInfo(window_id);
  if (!w) {
    LOG(WARNING) << __func__ << " failed to find video window for "
                 << window_id;
    return;
  }

  bool visibility_changed = false;
  if (w->visibility_.has_value() && w->visibility_.value() != visibility)
    visibility_changed = true;

  w->visibility_ = visibility;

  if (visibility_changed)
    provider_->NativeVideoWindowVisibilityChanged(window_id, visibility);
}

void VideoWindowControllerImpl::OnWindowEvent(
    gfx::AcceleratedWidget w,
    const base::UnguessableToken& window_id,
    VideoWindowProvider::Event event) {
  VLOG(1) << __func__ << " w=" << w << " id=" << window_id;
  switch (event) {
    case VideoWindowProvider::Event::kCreated:
      OnVideoWindowCreated(w, window_id);
      break;
    case VideoWindowProvider::Event::kDestroyed:
      OnVideoWindowDestroyed(w, window_id);
      break;
    default:
      break;
  }
}

void VideoWindowControllerImpl::OnVideoWindowCreated(
    gfx::AcceleratedWidget w,
    const base::UnguessableToken& window_id) {
  VLOG(1) << __func__ << " window_id=" << window_id;
  VideoWindowInfo* info = FindVideoWindowInfo(window_id);
  if (!info) {
    LOG(WARNING) << __func__ << " failed to find video window for "
                 << window_id;
    return;
  }
  info->state_ = VideoWindowInfo::State::kCreated;
}

void VideoWindowControllerImpl::OnVideoWindowDestroyed(
    gfx::AcceleratedWidget w,
    const base::UnguessableToken& window_id) {
  VLOG(1) << __func__ << " window_id=" << window_id;
  VideoWindowInfo* info = FindVideoWindowInfo(window_id);
  if (!info) {
    LOG(WARNING) << __func__ << " failed to find video window for "
                 << window_id;
    return;
  }
  info->state_ = VideoWindowInfo::State::kDestroyed;
  RemoveVideoWindowInfo(window_id);
}

// BeginOverlayProcessor is called from viz/Display overlay processor
// Now this will work only with --in-process-gpu command-line flag.
// But chromium will move viz/Display to GPU process then it will work without
// --in-process-gpu as well.
void VideoWindowControllerImpl::BeginOverlayProcessor(gpu::SurfaceHandle h) {
  if (task_runner_ && !task_runner_->BelongsToCurrentThread()) {
    task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(&VideoWindowControllerImpl::BeginOverlayProcessor,
                       base::Unretained(this), h));
    return;
  }

  gfx::AcceleratedWidget w = static_cast<gfx::AcceleratedWidget>(h);

  auto wl_it = video_windows_.find(w);
  if (wl_it == video_windows_.end())
    return;

  std::set<base::UnguessableToken>& hidden_candidate = hidden_candidate_[w];
  hidden_candidate.clear();

  for (auto const& window : wl_it->second)
    if (window->visibility_.has_value() && window->visibility_.value())
      hidden_candidate.insert(window->id_);
}

// EndOverlayProcessor is called from viz/Display overlay processor
// Now this will work only with --in-process-gpu command-line flag.
// But chromium will move viz/Display to GPU process then it will work without
// --in-process-gpu as well.
void VideoWindowControllerImpl::EndOverlayProcessor(gpu::SurfaceHandle h) {
  if (task_runner_ && !task_runner_->BelongsToCurrentThread()) {
    task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(&VideoWindowControllerImpl::EndOverlayProcessor,
                       base::Unretained(this), h));
    return;
  }

  gfx::AcceleratedWidget w = static_cast<gfx::AcceleratedWidget>(h);

  auto wl_it = video_windows_.find(w);
  if (wl_it == video_windows_.end())
    return;

  std::set<base::UnguessableToken>& hidden = hidden_candidate_[w];
  for (auto id : hidden)
    SetVideoWindowVisibility(id, false);
  hidden_candidate_.erase(w);
}

void VideoWindowControllerImpl::AcceleratedWidgetDeleted(
    gfx::AcceleratedWidget w) {
  if (!provider_) {
    LOG(ERROR) << "Not initialized.";
    return;
  }
  auto it = video_windows_.find(w);
  if (it == video_windows_.end())
    return;
  for (auto const& w_info : it->second)
    provider_->DestroyNativeVideoWindow(w, w_info->id_);
}

void VideoWindowControllerImpl::OwnerWidgetStateChanged(
    gfx::AcceleratedWidget w,
    ui::WidgetState state) {
  if (task_runner_ && !task_runner_->BelongsToCurrentThread()) {
    task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(&VideoWindowControllerImpl::OwnerWidgetStateChanged,
                       base::Unretained(this), w, state));
    return;
  }

  if (!provider_) {
    LOG(ERROR) << "Not initialized.";
    return;
  }
  auto it = video_windows_.find(w);
  if (it == video_windows_.end())
    return;
  for (auto const& w_info : it->second)
    provider_->OwnerWidgetStateChanged(w_info->id_, state);
}

void VideoWindowControllerImpl::Bind(
    mojo::PendingReceiver<ui::mojom::VideoWindowController> receiver) {
  // NOTE: We want to create a VidoeWindowProvider on the same thread with the
  // receiver's binding thread. So that VideoWindowProvider knows the thread
  // what it should live in. This thread is same with the thread of receiving
  // other ozone releated messages.
  if (!provider_)
    provider_ = VideoWindowProvider::Create();
  if (!task_runner_)
    task_runner_ = base::ThreadTaskRunnerHandle::Get();
  receiver_.Bind(std::move(receiver));
}

}  // namespace ui
