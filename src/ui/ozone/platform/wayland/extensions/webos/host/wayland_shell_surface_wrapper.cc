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

#include "ui/ozone/platform/wayland/extensions/webos/host/wayland_shell_surface_wrapper.h"

#include "base/strings/utf_string_conversions.h"
#include "ui/ozone/platform/wayland/extensions/webos/host/wayland_webos_extension.h"
#include "ui/ozone/platform/wayland/host/wayland_connection.h"
#include "ui/ozone/platform/wayland/host/wayland_window.h"

namespace ui {

WaylandShellSurfaceWrapper::WaylandShellSurfaceWrapper(
    WaylandWindow* wayland_window,
    WaylandConnection* connection)
    : wayland_window_(wayland_window), connection_(connection) {}

WaylandShellSurfaceWrapper::~WaylandShellSurfaceWrapper() = default;

bool WaylandShellSurfaceWrapper::Initialize(bool with_toplevel) {
  DCHECK(connection_ && connection_->extension());
  WaylandWebosExtension* webos_extension =
      static_cast<WaylandWebosExtension*>(connection_->extension());
  shell_surface_.reset(wl_shell_get_shell_surface(webos_extension->shell(),
                                                  wayland_window_->surface()));
  if (!shell_surface_) {
    LOG(ERROR) << "Failed to create wl_shell_surface";
    return false;
  }

  static const wl_shell_surface_listener shell_surface_listener = {
      WaylandShellSurfaceWrapper::Ping,
      WaylandShellSurfaceWrapper::Configure,
      WaylandShellSurfaceWrapper::PopupDone,
  };

  wl_shell_surface_add_listener(shell_surface_.get(), &shell_surface_listener,
                                this);
  if (with_toplevel)
    wl_shell_surface_set_toplevel(shell_surface_.get());
  return true;
}

void WaylandShellSurfaceWrapper::SetMaximized() {
  wl_shell_surface_set_maximized(shell_surface_.get(), nullptr);
}

void WaylandShellSurfaceWrapper::UnSetMaximized() {
  NOTIMPLEMENTED_LOG_ONCE();
}

void WaylandShellSurfaceWrapper::SetFullscreen() {
  wl_shell_surface_set_fullscreen(shell_surface_.get(),
                                  WL_SHELL_SURFACE_FULLSCREEN_METHOD_DEFAULT, 0,
                                  nullptr);
}

void WaylandShellSurfaceWrapper::UnSetFullscreen() {
  NOTIMPLEMENTED_LOG_ONCE();
}

void WaylandShellSurfaceWrapper::SetMinimized() {
  NOTIMPLEMENTED_LOG_ONCE();
}

void WaylandShellSurfaceWrapper::SurfaceMove(WaylandConnection* connection) {
  NOTIMPLEMENTED_LOG_ONCE();
}

void WaylandShellSurfaceWrapper::SurfaceResize(WaylandConnection* connection,
                                               uint32_t hittest) {
  NOTIMPLEMENTED_LOG_ONCE();
}

void WaylandShellSurfaceWrapper::SetTitle(const base::string16& title) {
  wl_shell_surface_set_title(shell_surface_.get(),
                             base::UTF16ToUTF8(title).c_str());
}

void WaylandShellSurfaceWrapper::AckConfigure() {
  NOTIMPLEMENTED_LOG_ONCE();
}

void WaylandShellSurfaceWrapper::SetWindowGeometry(const gfx::Rect& bounds) {
  NOTIMPLEMENTED_LOG_ONCE();
}

void WaylandShellSurfaceWrapper::SetMinSize(int32_t width, int32_t height) {
  NOTIMPLEMENTED_LOG_ONCE();
}

void WaylandShellSurfaceWrapper::SetMaxSize(int32_t width, int32_t height) {
  NOTIMPLEMENTED_LOG_ONCE();
}

void WaylandShellSurfaceWrapper::SetAppId(const std::string& app_id) {
  NOTIMPLEMENTED_LOG_ONCE();
}

void WaylandShellSurfaceWrapper::SetKeyMask(KeyMask key_mask, bool set) {
  NOTIMPLEMENTED_LOG_ONCE();
}

void WaylandShellSurfaceWrapper::SetInputRegion(
    const std::vector<gfx::Rect>& region) {
  NOTIMPLEMENTED_LOG_ONCE();
}

void WaylandShellSurfaceWrapper::SetWindowProperty(const std::string& name,
                                                   const std::string& value) {
  NOTIMPLEMENTED_LOG_ONCE();
}

// static
void WaylandShellSurfaceWrapper::Configure(void* data,
                                           wl_shell_surface* shell_surface,
                                           uint32_t edges,
                                           int32_t width,
                                           int32_t height) {
  NOTIMPLEMENTED_LOG_ONCE();
}

void WaylandShellSurfaceWrapper::PopupDone(void* data,
                                           wl_shell_surface* shell_surface) {
  NOTIMPLEMENTED_LOG_ONCE();
}

void WaylandShellSurfaceWrapper::Ping(void* data,
                                      wl_shell_surface* shell_surface,
                                      uint32_t serial) {
  wl_shell_surface_pong(shell_surface, serial);
}

}  // namespace ui
