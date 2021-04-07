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
#include "ozone/media/pseudo_video_window_provider.h"

#include <memory>
#include <string>

#include "base/threading/thread_task_runner_handle.h"
#include "base/unguessable_token.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "ozone/media/video_window_controller_impl.h"
#include "ozone/platform/messages.h"
#include "ui/gfx/geometry/rect.h"

namespace ui {

namespace {
const int kMinVideoGeometryUpdateInterval = 200;  // milliseconds
}  // namespace

class PseudoVideoWindow : public ui::mojom::VideoWindow,
                          public ui::VideoWindow {
 public:
  PseudoVideoWindow() = default;
  ~PseudoVideoWindow() override = default;

  // pseudo video window doesn't need this path.
  void SetNaturalVideoSize(const gfx::Size& natural_video_size) override {}
  void SetProperty(const std::string& name, const std::string& value) override {
  }
  void UpdateCurrentVideoWindowGeometry() override {}
  void UpdateVideoWindowGeometry(const gfx::Rect& src,
                                 const gfx::Rect& dst) override {}
  void UpdateVideoWindowGeometryWithCrop(const gfx::Rect& ori,
                                         const gfx::Rect& src,
                                         const gfx::Rect& dst) override {}
  VideoWindowProvider::WindowEventCb window_event_cb_;
  base::CancelableOnceCallback<void()> notify_geometry_cb_;
  gfx::Rect rect_;
  base::Time last_updated_ = base::Time::Now();
  mojo::Remote<ui::mojom::VideoWindowClient> client_;
  mojo::Receiver<ui::mojom::VideoWindow> receiver_{this};
};

std::unique_ptr<VideoWindowProvider> VideoWindowProvider::Create() {
  return std::make_unique<PseudoVideoWindowProvider>();
}

PseudoVideoWindowProvider::PseudoVideoWindowProvider() = default;

PseudoVideoWindowProvider::~PseudoVideoWindowProvider() = default;

base::UnguessableToken PseudoVideoWindowProvider::CreateNativeVideoWindow(
    gfx::AcceleratedWidget w,
    mojo::PendingRemote<ui::mojom::VideoWindowClient> client,
    mojo::PendingReceiver<ui::mojom::VideoWindow> receiver,
    const ui::VideoWindowParams& params,
    WindowEventCb cb) {
  base::UnguessableToken id = base::UnguessableToken::Create();
  auto result =
      pseudo_windows_.emplace(id, std::make_unique<PseudoVideoWindow>());
  if (!result.second) {
    LOG(ERROR) << __func__ << " failed to insert PseudoVideoWindow for " << id;
    return base::UnguessableToken::Null();
  }
  auto& window = result.first->second;
  window->window_id_ = id;
  window->native_window_name_ = "window_id_dummy";
  window->window_event_cb_ = cb;

  window->client_.Bind(std::move(client));
  window->receiver_.Bind(std::move(receiver));

  // To detect when a user stop using the window.
  window->client_.set_disconnect_handler(
      base::BindOnce(&PseudoVideoWindowProvider::DestroyNativeVideoWindow,
                     base::Unretained(this), w, id));

  window->client_->OnVideoWindowCreated(
      {window->window_id_, window->native_window_name_});

  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::BindOnce(cb, w, id, ui::VideoWindowProvider::Event::kCreated));
  return id;
}

void PseudoVideoWindowProvider::UpdateNativeVideoWindowGeometry(
    const base::UnguessableToken& window_id) {
  PseudoVideoWindow* win = FindWindow(window_id);
  if (!win) {
    LOG(ERROR) << __func__ << " failed to find info for " << window_id;
    return;
  }
  if (!win->notify_geometry_cb_.IsCancelled())
    win->notify_geometry_cb_.Cancel();

  VLOG(1) << __func__ << " window_id=" << window_id
          << " rect=" << win->rect_.ToString();
  win->client_->OnVideoWindowGeometryChanged(win->rect_);
  win->last_updated_ = base::Time::Now();
}

void PseudoVideoWindowProvider::NativeVideoWindowGeometryChanged(
    const base::UnguessableToken& window_id,
    const gfx::Rect& dst,
    const gfx::Rect& src,
    const base::Optional<gfx::Rect>& ori) {
  PseudoVideoWindow* win = FindWindow(window_id);
  if (!win) {
    LOG(ERROR) << __func__ << " failed to find info for " << window_id;
    return;
  }
  if (win->rect_ == dst)
    return;
  win->rect_ = dst;

  // callback is already scheduled!
  if (!win->notify_geometry_cb_.IsCancelled()) {
    return;
  }

  const base::TimeDelta elapsed = base::Time::Now() - win->last_updated_;
  const base::TimeDelta interval =
      base::TimeDelta::FromMilliseconds(kMinVideoGeometryUpdateInterval);
  if (elapsed < interval) {
    const base::TimeDelta next_update = interval - elapsed;
    win->notify_geometry_cb_.Reset(base::BindOnce(
        &PseudoVideoWindowProvider::UpdateNativeVideoWindowGeometry,
        base::Unretained(this), window_id));
    base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
        FROM_HERE, win->notify_geometry_cb_.callback(), next_update);
    return;
  }
  UpdateNativeVideoWindowGeometry(window_id);
}

void PseudoVideoWindowProvider::NativeVideoWindowVisibilityChanged(
    const base::UnguessableToken& window_id,
    bool visibility) {
  VLOG(1) << __func__ << " window_id=" << window_id
          << " visibility=" << visibility;

  PseudoVideoWindow* win = FindWindow(window_id);
  if (!win) {
    LOG(ERROR) << __func__ << " failed to find info for " << window_id;
    return;
  }
  if (!win->notify_geometry_cb_.IsCancelled()) {
    UpdateNativeVideoWindowGeometry(window_id);
  }

  win->client_->OnVideoWindowVisibilityChanged(visibility);
}

void PseudoVideoWindowProvider::OwnerWidgetStateChanged(
    const base::UnguessableToken& window_id,
    ui::WidgetState state) {}

void PseudoVideoWindowProvider::DestroyNativeVideoWindow(
    gfx::AcceleratedWidget w,
    const base::UnguessableToken& id) {
  PseudoVideoWindow* win = FindWindow(id);
  if (!win) {
    LOG(ERROR) << __func__ << " failed to find info for " << id;
    return;
  }
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(win->window_event_cb_, w, id,
                                ui::VideoWindowProvider::Event::kDestroyed));
  pseudo_windows_.erase(id);
}

PseudoVideoWindow* PseudoVideoWindowProvider::FindWindow(
    const base::UnguessableToken& window_id) {
  auto it = pseudo_windows_.find(window_id);
  if (it == pseudo_windows_.end())
    return nullptr;
  return it->second.get();
}
}  // namespace ui
