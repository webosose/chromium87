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

#ifndef UI_OZONE_PLATFORM_WAYLAND_EXTENSIONS_WEBOS_HOST_WEBOS_SHELL_SURFACE_WRAPPER_H_
#define UI_OZONE_PLATFORM_WAYLAND_EXTENSIONS_WEBOS_HOST_WEBOS_SHELL_SURFACE_WRAPPER_H_

#include "ui/ozone/platform/wayland/extensions/webos/common/wayland_webos_object.h"
#include "ui/ozone/platform/wayland/extensions/webos/host/wayland_shell_surface_wrapper.h"

namespace ui {

class WaylandConnection;
class WaylandWindow;

class WebosShellSurfaceWrapper : public WaylandShellSurfaceWrapper {
 public:
  WebosShellSurfaceWrapper(WaylandWindow* wayland_window,
                           WaylandConnection* connection);
  WebosShellSurfaceWrapper(const WebosShellSurfaceWrapper&) = delete;
  WebosShellSurfaceWrapper& operator=(const WebosShellSurfaceWrapper&) = delete;
  ~WebosShellSurfaceWrapper() override;

  bool Initialize(bool with_toplevel) override;
  void SetMaximized() override;
  void UnSetMaximized() override;
  void SetFullscreen() override;
  void UnSetFullscreen() override;
  void SetMinimized() override;
  void SetInputRegion(const std::vector<gfx::Rect>& region) override;
  void SetKeyMask(KeyMask key_mask, bool set) override;
  void SetWindowProperty(const std::string& name,
                         const std::string& value) override;

  // wl_webos_shell_surface listener
  // Called to notify a client that the surface state is changed.
  static void StateChanged(void* data,
                           wl_webos_shell_surface* webos_shell_surface,
                           uint32_t state);
  // Called to notify a client that the surface position is changed.
  static void PositionChanged(void* data,
                              wl_webos_shell_surface* webos_shell_surface,
                              int32_t x,
                              int32_t y);
  // Called by the compositor to request closing of the window.
  static void Close(void* data, wl_webos_shell_surface* webos_shell_surface);
  // Called to notify a client which surface areas are now exposed (visible).
  static void Exposed(void* data,
                      wl_webos_shell_surface* webos_shell_surface,
                      wl_array* rectangles);
  // Called to notify a client that the surface state is about to change.
  static void StateAboutToChange(void* data,
                                 wl_webos_shell_surface* webos_shell_surface,
                                 uint32_t state);

 private:
  std::uint32_t applied_key_masks_ = 0;
  WaylandWindow* const wayland_window_;
  WaylandConnection* const connection_;
  wl::Object<wl_webos_shell_surface> webos_shell_surface_;
};

}  // namespace ui

#endif  // UI_OZONE_PLATFORM_WAYLAND_EXTENSIONS_WEBOS_HOST_WEBOS_SHELL_SURFACE_WRAPPER_H_
