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

#include "ui/ozone/platform/wayland/extensions/webos/host/webos_shell_surface_wrapper.h"

#include <wayland-webos-shell-client-protocol.h>

#include "ui/ozone/platform/wayland/extensions/webos/host/wayland_webos_extension.h"
#include "ui/ozone/platform/wayland/host/wayland_connection.h"
#include "ui/ozone/platform/wayland/host/wayland_window.h"

namespace ui {

PlatformWindowState ToPlatformWindowState(uint32_t state) {
  switch (state) {
    case WL_WEBOS_SHELL_SURFACE_STATE_DEFAULT:
      return PlatformWindowState::kNormal;
    case WL_WEBOS_SHELL_SURFACE_STATE_MINIMIZED:
      return PlatformWindowState::kMinimized;
    case WL_WEBOS_SHELL_SURFACE_STATE_MAXIMIZED:
      return PlatformWindowState::kMaximized;
    case WL_WEBOS_SHELL_SURFACE_STATE_FULLSCREEN:
      return PlatformWindowState::kFullScreen;
    default:
      return PlatformWindowState::kUnknown;
  }
}

bool ToActivationState(uint32_t state) {
  switch (state) {
    case WL_WEBOS_SHELL_SURFACE_STATE_DEFAULT:
    case WL_WEBOS_SHELL_SURFACE_STATE_MAXIMIZED:
    case WL_WEBOS_SHELL_SURFACE_STATE_FULLSCREEN:
      return true;
    case WL_WEBOS_SHELL_SURFACE_STATE_MINIMIZED:
      return false;
    default:
      return true;
  }
}

WebosShellSurfaceWrapper::WebosShellSurfaceWrapper(
    WaylandWindow* wayland_window,
    WaylandConnection* connection)
    : WaylandShellSurfaceWrapper(wayland_window, connection),
      wayland_window_(wayland_window),
      connection_(connection) {}

WebosShellSurfaceWrapper::~WebosShellSurfaceWrapper() = default;

bool WebosShellSurfaceWrapper::Initialize(bool with_toplevel) {
  DCHECK(connection_ && connection_->extension());
  WaylandWebosExtension* webos_extension =
      static_cast<WaylandWebosExtension*>(connection_->extension());

  WaylandShellSurfaceWrapper::Initialize(with_toplevel);

  webos_shell_surface_.reset(wl_webos_shell_get_shell_surface(
      webos_extension->webos_shell(), wayland_window_->surface()));
  if (!webos_shell_surface_) {
    LOG(ERROR) << "Failed to create wl_webos_shell_surface";
    return false;
  }

  static const wl_webos_shell_surface_listener webos_shell_surface_listener = {
      WebosShellSurfaceWrapper::StateChanged,
      WebosShellSurfaceWrapper::PositionChanged,
      WebosShellSurfaceWrapper::Close, WebosShellSurfaceWrapper::Exposed,
      WebosShellSurfaceWrapper::StateAboutToChange};

  wl_webos_shell_surface_add_listener(webos_shell_surface_.get(),
                                      &webos_shell_surface_listener, this);

  return true;
}

void WebosShellSurfaceWrapper::SetMaximized() {
  wl_webos_shell_surface_set_state(webos_shell_surface_.get(),
                                   WL_WEBOS_SHELL_SURFACE_STATE_MAXIMIZED);
}

void WebosShellSurfaceWrapper::UnSetMaximized() {
  wl_webos_shell_surface_set_state(webos_shell_surface_.get(),
                                   WL_WEBOS_SHELL_SURFACE_STATE_DEFAULT);
}

void WebosShellSurfaceWrapper::SetFullscreen() {
  wl_webos_shell_surface_set_state(webos_shell_surface_.get(),
                                   WL_WEBOS_SHELL_SURFACE_STATE_FULLSCREEN);
}

void WebosShellSurfaceWrapper::UnSetFullscreen() {
  wl_webos_shell_surface_set_state(webos_shell_surface_.get(),
                                   WL_WEBOS_SHELL_SURFACE_STATE_DEFAULT);
}

void WebosShellSurfaceWrapper::SetMinimized() {
  wl_webos_shell_surface_set_state(webos_shell_surface_.get(),
                                   WL_WEBOS_SHELL_SURFACE_STATE_MINIMIZED);
}

void WebosShellSurfaceWrapper::SetInputRegion(
    const std::vector<gfx::Rect>& region) {
  wl::Object<wl_region> wlregion(
      wl_compositor_create_region(connection_->compositor()));

  for (const auto& reg : region)
    wl_region_add(wlregion.get(), reg.x(), reg.y(), reg.width(), reg.height());

  wl_surface_set_input_region(wayland_window_->surface(), wlregion.get());
  wl_surface_commit(wayland_window_->surface());
}

void WebosShellSurfaceWrapper::SetKeyMask(KeyMask key_mask, bool set) {
  std::uint32_t curr_key_mask = static_cast<std::uint32_t>(key_mask);
  std::uint32_t key_masks = set ? applied_key_masks_ | curr_key_mask
                                : applied_key_masks_ & ~curr_key_mask;
  if (key_masks == applied_key_masks_)
    return;

  applied_key_masks_ = key_masks;
  wl_webos_shell_surface_set_key_mask(webos_shell_surface_.get(), key_masks);
}

void WebosShellSurfaceWrapper::SetWindowProperty(const std::string& name,
                                                 const std::string& value) {
  wl_webos_shell_surface_set_property(webos_shell_surface_.get(), name.c_str(),
                                      value.c_str());
}

// static
void WebosShellSurfaceWrapper::StateChanged(
    void* data,
    wl_webos_shell_surface* webos_shell_surface,
    uint32_t state) {
  WebosShellSurfaceWrapper* shell_surface_wrapper =
      static_cast<WebosShellSurfaceWrapper*>(data);
  DCHECK(shell_surface_wrapper);
  DCHECK(shell_surface_wrapper->wayland_window_);

  if (shell_surface_wrapper->wayland_window_) {
    shell_surface_wrapper->wayland_window_->HandleStateChanged(
        ToPlatformWindowState(state));
    shell_surface_wrapper->wayland_window_->HandleActivationChanged(
        ToActivationState(state));
  }
}

void WebosShellSurfaceWrapper::PositionChanged(
    void* data,
    wl_webos_shell_surface* webos_shell_surface,
    int32_t x,
    int32_t y) {
  NOTIMPLEMENTED_LOG_ONCE();
}

void WebosShellSurfaceWrapper::Close(
    void* data,
    wl_webos_shell_surface* webos_shell_surface) {
  NOTIMPLEMENTED_LOG_ONCE();
}

void WebosShellSurfaceWrapper::Exposed(
    void* data,
    wl_webos_shell_surface* webos_shell_surface,
    wl_array* rectangles) {
  WebosShellSurfaceWrapper* shell_surface_wrapper =
      static_cast<WebosShellSurfaceWrapper*>(data);
  DCHECK(shell_surface_wrapper);
  DCHECK(shell_surface_wrapper->wayland_window_);
  // Activate window.
  if (shell_surface_wrapper->wayland_window_) {
    shell_surface_wrapper->wayland_window_->HandleActivationChanged(true);
    shell_surface_wrapper->wayland_window_->HandleExposed();
  }
}

void WebosShellSurfaceWrapper::StateAboutToChange(
    void* data,
    wl_webos_shell_surface* webos_shell_surface,
    uint32_t state) {
  WebosShellSurfaceWrapper* shell_surface_wrapper =
      static_cast<WebosShellSurfaceWrapper*>(data);
  DCHECK(shell_surface_wrapper);
  DCHECK(shell_surface_wrapper->wayland_window_);

  if (shell_surface_wrapper->wayland_window_)
    shell_surface_wrapper->wayland_window_->HandleStateAboutToChange(
        ToPlatformWindowState(state));
}

}  // namespace ui
