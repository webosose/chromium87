// Copyright 2013 Intel Corporation. All rights reserved.
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

#ifndef OZONE_WAYLAND_SHELL_SHELL_SURFACE_H_
#define OZONE_WAYLAND_SHELL_SHELL_SURFACE_H_

#include <wayland-client.h>

#include <vector>

#include "ozone/wayland/window.h"
#include "ui/gfx/geometry/rect.h"

#if defined(OS_WEBOS)
#include "ui/views/widget/desktop_aura/neva/ui_constants.h"
#endif

namespace ozonewayland {

class WaylandWindow;

class WaylandShellSurface {
 public:
  WaylandShellSurface();
  virtual ~WaylandShellSurface();

  struct wl_surface* GetWLSurface() const;

  // The implementation should initialize the shell and set up all
  // necessary callbacks.
  virtual void InitializeShellSurface(WaylandWindow* window,
                                      WaylandWindow::ShellType type) = 0;
  virtual void UpdateShellSurface(WaylandWindow::ShellType type,
                                  WaylandShellSurface* shell_parent,
                                  int x,
                                  int y) = 0;
  virtual void SetWindowTitle(const base::string16& title) = 0;
  virtual void Maximize() = 0;
  virtual void Minimize() = 0;
  virtual void Unminimize() = 0;
  virtual bool IsMinimized() const = 0;
#if defined(OS_WEBOS)
  virtual void SetGroupKeyMask(ui::KeyMask key_mask) {}
  virtual void SetKeyMask(ui::KeyMask key_mask, bool set) {}
#endif
  virtual void SetInputRegion(const std::vector<gfx::Rect>& region) {}
  virtual void SetWindowProperty(const std::string& name, const std::string& value);

  // static functions.
  static void PopupDone();
  static void WindowResized(void *data, unsigned width, unsigned height);
  static void WindowActivated(void *data);
  static void WindowDeActivated(void *data);

 protected:
  void FlushDisplay() const;

 private:
  struct wl_surface* surface_;
  DISALLOW_COPY_AND_ASSIGN(WaylandShellSurface);
};

}  // namespace ozonewayland

#endif  // OZONE_WAYLAND_SHELL_SHELL_SURFACE_H_
