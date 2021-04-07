// Copyright 2014 Intel Corporation. All rights reserved.
// Copyright 2014-2018 LG Electronics, Inc.
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

#ifndef OZONE_WAYLAND_SHELL_WL_SHELL_SURFACE_H_
#define OZONE_WAYLAND_SHELL_WL_SHELL_SURFACE_H_

#include "ozone/wayland/shell/shell_surface.h"

namespace ozonewayland {

class WaylandSurface;
class WaylandWindow;

class WLShellSurface : public WaylandShellSurface {
 public:
  WLShellSurface();
  ~WLShellSurface() override;

  void InitializeShellSurface(WaylandWindow* window,
                              WaylandWindow::ShellType type) override;
  void UpdateShellSurface(WaylandWindow::ShellType type,
                          WaylandShellSurface* shell_parent,
                          int x,
                          int y) override;
  void SetWindowTitle(const base::string16& title) override;
  void Maximize() override;
  void Minimize() override;
  void Unminimize() override;
  bool IsMinimized() const override;

  static void HandleConfigure(void* data,
                              struct wl_shell_surface* shell_surface,
                              uint32_t edges,
                              int32_t width,
                              int32_t height);
  static void HandlePopupDone(void* data,
                              struct wl_shell_surface* shell_surface);
  static void HandlePing(void* data,
                         struct wl_shell_surface* shell_surface,
                         uint32_t serial);

 private:
  wl_shell_surface* shell_surface_;
  DISALLOW_COPY_AND_ASSIGN(WLShellSurface);
};

}  // namespace ozonewayland

#endif  // OZONE_WAYLAND_SHELL_WL_SHELL_SURFACE_H_
