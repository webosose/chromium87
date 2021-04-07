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

#include "ozone/media/foreign_video_window_provider.h"

#include <memory>
#include <string>

#include "base/logging.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/unguessable_token.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "ozone/media/video_window_controller_impl.h"
#include "ozone/platform/messages.h"
#include "ozone/wayland/display.h"
#include "ozone/wayland/screen.h"
#include "ozone/wayland/shell/shell_surface.h"
#include "ozone/wayland/window.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/platform_window/neva/mojom/video_window_controller.mojom.h"

namespace ui {

namespace {

const int kMinVideoGeometryUpdateIntervalMs = 200;
const char kMute[] = "mute";
const char kOn[] = "on";
const char kOff[] = "off";

bool IsSupportedType(ui::ForeignWindowType type) {
  return type == ui::ForeignWindowType::kVideo ||
         type == ui::ForeignWindowType::kSubTitle;
}

std::string ToString(ui::ForeignWindowType type) {
  switch (type) {
    case ui::ForeignWindowType::kVideo:
      return "VIDEO";
    case ui::ForeignWindowType::kSubTitle:
      return "SUBTITLE";
    default:
      return "INVALID";
  }
}

ui::ForeignWindowType ConvertToForeignWindowType(uint32_t type) {
  switch (type) {
    case 0:
      return ui::ForeignWindowType::kVideo;
    case 1:
      return ui::ForeignWindowType::kSubTitle;
  }
  return ui::ForeignWindowType::kInvalid;
}

uint32_t ConvertToUInt32(ui::ForeignWindowType type) {
  switch (type) {
    case ui::ForeignWindowType::kVideo:
      return 0;
    case ui::ForeignWindowType::kSubTitle:
      return 1;
    default:
      NOTREACHED() << " Invalid conversion.";
      return 0;
  }
}

const char* WidgetStateToString(ui::WidgetState state) {
#define STRINGIFY_owner_widget_STATE_CASE(state) \
  case ui::WidgetState::state:                   \
    return #state

  switch (state) {
    STRINGIFY_owner_widget_STATE_CASE(UNINITIALIZED);
    STRINGIFY_owner_widget_STATE_CASE(SHOW);
    STRINGIFY_owner_widget_STATE_CASE(HIDE);
    STRINGIFY_owner_widget_STATE_CASE(FULLSCREEN);
    STRINGIFY_owner_widget_STATE_CASE(MAXIMIZED);
    STRINGIFY_owner_widget_STATE_CASE(MINIMIZED);
    STRINGIFY_owner_widget_STATE_CASE(RESTORE);
    STRINGIFY_owner_widget_STATE_CASE(ACTIVE);
    STRINGIFY_owner_widget_STATE_CASE(INACTIVE);
    STRINGIFY_owner_widget_STATE_CASE(RESIZE);
    STRINGIFY_owner_widget_STATE_CASE(DESTROYED);
  }
  return "null";
}

}  // namespace

class ForeignVideoWindow : public ui::mojom::VideoWindow,
                           public ui::VideoWindow {
 public:
  enum class State { kNone, kCreating, kCreated, kDestroying, kDestroyed };

  ForeignVideoWindow(ForeignVideoWindowProvider* provider,
                     gfx::AcceleratedWidget w,
                     const base::UnguessableToken& window_id,
                     const ui::VideoWindowParams& params,
                     ui::ForeignWindowType type,
                     struct wl_webos_exported* webos_exported);
  ~ForeignVideoWindow();

  // Implements ui::mojom::VideoWindow
  void SetNaturalVideoSize(const gfx::Size& natural_video_size) override;
  void SetProperty(const std::string& name, const std::string& value) override;
  void UpdateCurrentVideoWindowGeometry() override;
  void UpdateVideoWindowGeometry(const gfx::Rect& src,
                                 const gfx::Rect& dst) override;
  void UpdateVideoWindowGeometryWithCrop(const gfx::Rect& ori,
                                         const gfx::Rect& src,
                                         const gfx::Rect& dst) override;

  ForeignVideoWindowProvider* provider_ = nullptr;
  gfx::AcceleratedWidget owner_widget_ = gfx::kNullAcceleratedWidget;
  ui::VideoWindowParams params_;
  ui::ForeignWindowType type_ = ui::ForeignWindowType::kInvalid;
  struct wl_webos_exported* webos_exported_ = nullptr;
  VideoWindowProvider::WindowEventCb window_event_cb_;
  base::CancelableOnceCallback<void()> notify_geometry_cb_;
  base::Optional<gfx::Rect> ori_rect_ = base::nullopt;
  base::Optional<gfx::Size> natural_video_size_;
  gfx::Rect src_rect_;
  gfx::Rect dst_rect_;
  base::Time last_updated_ = base::Time::Now();
  State state_ = State::kNone;
  bool owner_widget_shown_ = true;
  bool visible_in_screen_ = true;

  mojo::Remote<ui::mojom::VideoWindowClient> client_;
  mojo::Receiver<ui::mojom::VideoWindow> receiver_{this};
};

ForeignVideoWindow::ForeignVideoWindow(ForeignVideoWindowProvider* provider,
                                       gfx::AcceleratedWidget w,
                                       const base::UnguessableToken& window_id,
                                       const ui::VideoWindowParams& params,
                                       ui::ForeignWindowType type,
                                       struct wl_webos_exported* webos_exported)
    : ui::VideoWindow{window_id, ""},
      provider_(provider),
      owner_widget_(w),
      params_(params),
      type_(type),
      webos_exported_(webos_exported) {
  VLOG(1) << __func__ << " window_id=" << window_id_ << " type=" << (int)type
          << " webos_exported=" << webos_exported_;
}
ForeignVideoWindow::~ForeignVideoWindow() {
  VLOG(1) << __func__;
  if (!webos_exported_)
    return;

  wl_webos_exported_destroy(webos_exported_);
}

void ForeignVideoWindow::SetNaturalVideoSize(
    const gfx::Size& natural_video_size) {
  natural_video_size_ = natural_video_size;
}

void ForeignVideoWindow::SetProperty(const std::string& name,
                                     const std::string& value) {
  provider_->NativeVideoWindowSetProperty(window_id_, name, value);
}

void ForeignVideoWindow::UpdateCurrentVideoWindowGeometry() {
  provider_->UpdateNativeVideoWindowGeometry(window_id_);
}

void ForeignVideoWindow::UpdateVideoWindowGeometry(const gfx::Rect& src,
                                                   const gfx::Rect& dst) {
  VLOG(1) << __func__ << " src=" << src.ToString() << " dst=" << dst.ToString();
  provider_->NativeVideoWindowGeometryChanged(window_id_, dst, src);
}

void ForeignVideoWindow::UpdateVideoWindowGeometryWithCrop(
    const gfx::Rect& ori,
    const gfx::Rect& src,
    const gfx::Rect& dst) {
  VLOG(1) << __func__ << " ori=" << ori.ToString() << " src=" << src.ToString()
          << " dst=" << dst.ToString();
  provider_->NativeVideoWindowGeometryChanged(window_id_, dst, src, ori);
}

std::unique_ptr<VideoWindowProvider> VideoWindowProvider::Create() {
  return std::make_unique<ForeignVideoWindowProvider>();
}

ForeignVideoWindowProvider::ForeignVideoWindowProvider()
    : task_runner_(base::ThreadTaskRunnerHandle::Get()) {}

ForeignVideoWindowProvider::~ForeignVideoWindowProvider() = default;

// static
void ForeignVideoWindowProvider::HandleExportedWindowAssigned(
    void* data,
    struct wl_webos_exported* webos_exported,
    const char* native_window_id,
    uint32_t exported_type) {
  ForeignVideoWindowProvider* window_provider =
      static_cast<ForeignVideoWindowProvider*>(data);
  if (!window_provider)
    return;

  // ForeignVideoWindowProvider exists as long as ozone is alive.
  // |native_window_id| should be converted to string before posting.
  window_provider->task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&ForeignVideoWindowProvider::OnCreatedForeignWindow,
                     base::Unretained(window_provider), webos_exported,
                     std::string(native_window_id),
                     ConvertToForeignWindowType(exported_type)));
}

void ForeignVideoWindowProvider::OnCreatedForeignWindow(
    struct wl_webos_exported* webos_exported,
    const std::string& native_window_id,
    ui::ForeignWindowType type) {
  VLOG(1) << __func__ << " native_window_id=" << native_window_id;
  ForeignVideoWindow* window = FindWindow(webos_exported);
  if (!window) {
    LOG(ERROR) << __func__
               << " failed to find window for exported=" << webos_exported
               << " native_id=" << native_window_id;
    return;
  }
  window->state_ = ForeignVideoWindow::State::kCreated;
  window->native_window_name_ = native_window_id;
  window->type_ = type;
  if (!window->client_) {
    LOG(ERROR) << __func__ << " client_ is disconnected!";
    return;
  }

  window->client_->OnVideoWindowCreated({window->window_id_, native_window_id});
  window->window_event_cb_.Run(window->owner_widget_, window->window_id_,
                               ui::VideoWindowProvider::Event::kCreated);
}

base::UnguessableToken ForeignVideoWindowProvider::CreateNativeVideoWindow(
    gfx::AcceleratedWidget w,
    mojo::PendingRemote<ui::mojom::VideoWindowClient> client,
    mojo::PendingReceiver<ui::mojom::VideoWindow> receiver,
    const VideoWindowParams& params,
    WindowEventCb cb) {
  VLOG(1) << __func__;

  mojo::Remote<ui::mojom::VideoWindowClient> window_client(std::move(client));

  ozonewayland::WaylandDisplay* display =
      ozonewayland::WaylandDisplay::GetInstance();
  ozonewayland::WaylandWindow* wayland_window = nullptr;
  ozonewayland::WaylandShellSurface* shell_surface = nullptr;
  struct wl_surface* surface = nullptr;

  if (display)
    wayland_window = display->GetWindow(static_cast<unsigned>(w));
  if (wayland_window)
    shell_surface = wayland_window->ShellSurface();
  if (shell_surface)
    surface = shell_surface->GetWLSurface();

  if (!surface) {
    LOG(ERROR) << __func__ << " Failed to get surface window_handle=" << w
               << " display=" << display << " wayland_window=" << wayland_window
               << " shell_surface=" << shell_surface << " surface=" << surface;
    window_client->OnVideoWindowDestroyed();
    return base::UnguessableToken::Null();
  }

  base::UnguessableToken id = base::UnguessableToken::Create();
  ui::ForeignWindowType type = ForeignWindowType::kVideo;
  static const wl_webos_exported_listener exported_listener{
      ForeignVideoWindowProvider::HandleExportedWindowAssigned};

  wl_webos_exported* webos_exported = wl_webos_foreign_export_element(
      display->GetWebosForeign(), surface, ConvertToUInt32(type));
  if (!webos_exported) {
    LOG(ERROR) << __func__ << " failed to create webos_exported";
    return base::UnguessableToken::Null();
  }
  wl_webos_exported_add_listener(webos_exported, &exported_listener, this);

  std::unique_ptr<ForeignVideoWindow> window =
      std::make_unique<ForeignVideoWindow>(this, w, id, params, type,
                                           webos_exported);
  if (!window) {
    LOG(ERROR) << __func__
               << " failed to create foreign video window for id=" << id;
    window_client->OnVideoWindowDestroyed();
    return base::UnguessableToken::Null();
  }

  native_id_to_window_id_[id.ToString()] = id;
  window->state_ = ForeignVideoWindow::State::kCreating;
  window->window_event_cb_ = cb;
  window->client_ = std::move(window_client);
  window->receiver_.Bind(std::move(receiver));
  // To detect when the user stop using the window.
  window->client_.set_disconnect_handler(
      base::BindOnce(&ForeignVideoWindowProvider::DestroyNativeVideoWindow,
                     base::Unretained(this), w, id));

  foreign_windows_.emplace(id, std::move(window));
  return id;
}

void ForeignVideoWindowProvider::UpdateNativeVideoWindowGeometry(
    const base::UnguessableToken& window_id) {
  ForeignVideoWindow* w = FindWindow(window_id);
  if (!w) {
    LOG(ERROR) << __func__ << " failed to find foreign window for "
               << window_id;
    return;
  }

  auto display = ozonewayland::WaylandDisplay::GetInstance();
  auto window = display->GetWindow(static_cast<unsigned>(w->owner_widget_));
  if (!window) {
    LOG(ERROR) << __func__ << " window is nullptr";
    return;
  }

  if (!w->notify_geometry_cb_.IsCancelled())
    w->notify_geometry_cb_.Cancel();

  gfx::Rect source = w->src_rect_;
  gfx::Rect dest = w->dst_rect_;
  base::Optional<gfx::Rect> ori = w->ori_rect_;

  // set_exported_window is not work correctly with punch-through in below cases
  // 1. only part of dst video is located in the window
  // 2. the ratio of video width/height is different from the ratio of dst rect
  //    width/height
  // So we will use set_crop_region basically for the general cases.
#if defined(USE_GST_MEDIA)
  // When we are using texture mode, we should use set_exported_window only.
  bool use_set_crop_region = false;
#else
  bool use_set_crop_region = true;
#endif

  // Always use set_exported_window to keep the original video w/h ratio in
  // fullscreen.
  // set_exported_window always keeps the ratio even though dst is not match
  // with the ratio.
  // In webOS, for application window resolution is less than the screen
  // resolution, we have to consider window bounds to decide full-screen mode
  // of video.
  bool fullscreen = (window->GetBounds() == dest);
  if (!fullscreen && use_set_crop_region) {
    // TODO(neva): Currently it considers only single screen for
    // use_set_crop_region. If moving on supporting multi-screen we need to
    // check how to use set_crop_region with multi-screen and revisit this
    // clipping implementaion.
    auto screen = display->PrimaryScreen();
    if (!screen) {
      LOG(ERROR) << __func__ << " primary screen is nullptr";
      return;
    }
    gfx::Rect screen_rect = screen->Geometry();

    // Adjust for original_rect/source/dest rect
    const gfx::Rect& original_rect =
        w->natural_video_size_ ? gfx::Rect(w->natural_video_size_.value())
                               : screen_rect;

    gfx::Rect visible_rect = gfx::IntersectRects(dest, screen_rect);

    DCHECK(visible_rect.width() != 0 && visible_rect.height() != 0);

    int source_x = visible_rect.x() - dest.x();
    int source_y = visible_rect.y() - dest.y();

    float scale_width =
        static_cast<float>(original_rect.width()) / dest.width();
    float scale_height =
        static_cast<float>(original_rect.height()) / dest.height();

    gfx::Rect source_rect(source_x, source_y, visible_rect.width(),
                          visible_rect.height());
    source_rect =
        gfx::ScaleToEnclosingRectSafe(source_rect, scale_width, scale_height);
    // source_rect must be inside of original_rect
    if (!original_rect.Contains(source_rect)) {
      LOG(ERROR) << __func__
                 << " some part of source rect are outside of original rect."
                 << "  original rect: " << original_rect.ToString()
                 << "  / source rect: " << source_rect.ToString();
      source_rect.Intersect(original_rect);
    }
    ori = original_rect;
    source = source_rect;
    dest = visible_rect;
  }

  wl_compositor* wlcompositor = display->GetCompositor();

  wl_region* source_region = wl_compositor_create_region(wlcompositor);
  wl_region_add(source_region, source.x(), source.y(), source.width(),
                source.height());

  wl_region* dest_region = wl_compositor_create_region(wlcompositor);
  wl_region_add(dest_region, dest.x(), dest.y(), dest.width(), dest.height());

  wl_region* ori_region = nullptr;
  if (ori) {
    ori_region = wl_compositor_create_region(wlcompositor);
    wl_region_add(ori_region, ori->x(), ori->y(), ori->width(), ori->height());
    wl_webos_exported_set_crop_region(w->webos_exported_, ori_region,
                                      source_region, dest_region);
    VLOG(1) << __func__ << " called set_crop_region ori=" << ori->ToString()
            << " src=" << source.ToString() << " dst=" << dest.ToString();
  } else {
    wl_webos_exported_set_exported_window(w->webos_exported_, source_region,
                                          dest_region);
    VLOG(1) << __func__ << " called exported_window ori="
            << " src=" << source.ToString() << " dst=" << dest.ToString();
  }

  wl_region_destroy(dest_region);
  wl_region_destroy(source_region);
  if (ori_region)
    wl_region_destroy(ori_region);

  w->last_updated_ = base::Time::Now();
}

void ForeignVideoWindowProvider::NativeVideoWindowGeometryChanged(
    const base::UnguessableToken& window_id,
    const gfx::Rect& dst_rect,
    const gfx::Rect& src_rect,
    const base::Optional<gfx::Rect>& ori_rect) {
  ForeignVideoWindow* win = FindWindow(window_id);
  if (!win) {
    LOG(ERROR) << __func__ << " failed to find windows for id=" << window_id;
    return;
  }

  bool changed = false;

  if (win->ori_rect_ != ori_rect) {
    win->ori_rect_ = ori_rect;
    changed = true;
  }

  if (win->src_rect_ != src_rect) {
    win->src_rect_ = src_rect;
    changed = true;
  }

  if (win->dst_rect_ != dst_rect) {
    win->dst_rect_ = dst_rect;
    changed = true;
  }

  // If any geometry is not changed there is no reason to update.
  // Also if the callback is already scheduled, just wait for callback
  if (!changed || !win->notify_geometry_cb_.IsCancelled())
    return;

  const base::TimeDelta elapsed = base::Time::Now() - win->last_updated_;
  const base::TimeDelta interval =
      base::TimeDelta::FromMilliseconds(kMinVideoGeometryUpdateIntervalMs);
  if (elapsed < interval) {
    const base::TimeDelta next_update = interval - elapsed;
    win->notify_geometry_cb_.Reset(base::BindOnce(
        &ForeignVideoWindowProvider::UpdateNativeVideoWindowGeometry,
        base::Unretained(this), window_id));
    base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
        FROM_HERE, win->notify_geometry_cb_.callback(), next_update);
    return;
  }
  UpdateNativeVideoWindowGeometry(window_id);
}

void ForeignVideoWindowProvider::NativeVideoWindowVisibilityChanged(
    const base::UnguessableToken& window_id,
    bool visibility) {
  VLOG(1) << __func__ << " window_id=" << window_id
          << " visibility=" << visibility;

  ForeignVideoWindow* win = FindWindow(window_id);
  if (!win) {
    LOG(ERROR) << __func__ << " failed to find windows for id=" << window_id;
    return;
  }

  if (win->visible_in_screen_ == visibility) {
    VLOG(1) << __func__ << " window_id=" << window_id
            << " notified the same visibility";
    return;
  }

  if (win->params_.use_video_mute_on_invisible)
    NativeVideoWindowSetProperty(window_id, kMute, visibility ? kOff : kOn);

  win->visible_in_screen_ = visibility;

  if (!win->notify_geometry_cb_.IsCancelled()) {
    UpdateNativeVideoWindowGeometry(window_id);
  }
}

void ForeignVideoWindowProvider::OwnerWidgetStateChanged(
    const base::UnguessableToken& window_id,
    ui::WidgetState state) {
  VLOG(1) << __func__ << " window_id=" << window_id
          << " widget_state=" << WidgetStateToString(state);

  ForeignVideoWindow* win = FindWindow(window_id);
  if (!win) {
    LOG(ERROR) << __func__ << " failed to find windows for id=" << window_id;
    return;
  }

  bool new_value = win->owner_widget_shown_;
  switch (state) {
    case ui::WidgetState::MINIMIZED:
      new_value = false;
      break;
    case ui::WidgetState::MAXIMIZED:
    case ui::WidgetState::FULLSCREEN:
      new_value = true;
      break;
    default:
      break;
  }

  if (win->owner_widget_shown_ == new_value)
    return;

  win->owner_widget_shown_ = new_value;

  // No need to change video mute state for already muted video.
  if (win->params_.use_video_mute_on_app_minimized && win->visible_in_screen_)
    NativeVideoWindowSetProperty(window_id, kMute,
                                 win->owner_widget_shown_ ? kOff : kOn);
}

void ForeignVideoWindowProvider::DestroyNativeVideoWindow(
    gfx::AcceleratedWidget w,
    const base::UnguessableToken& id) {
  ForeignVideoWindow* video_window = FindWindow(id);
  if (video_window) {
    video_window->state_ = ForeignVideoWindow::State::kDestroying;
    task_runner_->PostTask(
        FROM_HERE, base::BindOnce(video_window->window_event_cb_, w, id,
                                  ui::VideoWindowProvider::Event::kDestroyed));
    if (video_window->client_)
      video_window->client_->OnVideoWindowDestroyed();

    foreign_windows_.erase(id);
  } else {
    LOG(WARNING) << __func__ << " failed to find video window id=" << id;
  }
}

void ForeignVideoWindowProvider::NativeVideoWindowSetProperty(
    const base::UnguessableToken& window_id,
    const std::string& name,
    const std::string& value) {
  ForeignVideoWindow* win = FindWindow(window_id);
  if (!win) {
    LOG(ERROR) << __func__ << " failed to find windows for id=" << window_id;
    return;
  }
  wl_webos_exported_set_property(win->webos_exported_, name.c_str(),
                                 value.c_str());
}

ForeignVideoWindow* ForeignVideoWindowProvider::FindWindow(
    struct wl_webos_exported* webos_exported) {
  for (auto it = foreign_windows_.begin(); it != foreign_windows_.end(); ++it) {
    if (it->second->webos_exported_ == webos_exported) {
      return it->second.get();
    }
  }
  return nullptr;
}

ForeignVideoWindow* ForeignVideoWindowProvider::FindWindow(
    const base::UnguessableToken& id) {
  auto it = foreign_windows_.find(id);
  if (it == foreign_windows_.end())
    return nullptr;
  return it->second.get();
}

}  // namespace ui
