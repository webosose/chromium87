// Copyright 2013 The Chromium Authors. All rights reserved.
// Copyright 2013 Intel Corporation. All rights reserved.
// Copyright 2016 LG Electronics, Inc.
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

#ifndef OZONE_WAYLAND_WINDOW_H_
#define OZONE_WAYLAND_WINDOW_H_

#include <wayland-client.h>

#include <vector>

#include "base/macros.h"
#include "base/strings/string16.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/location_hint.h"
#include "ui/views/widget/desktop_aura/neva/ui_constants.h"

struct wl_egl_window;

namespace ui {
class WindowGroupConfiguration;
}

namespace ozonewayland {

class WaylandShellSurface;
class EGLWindow;

#if defined(OS_WEBOS)
class WebOSSurfaceGroup;
#endif

typedef unsigned WaylandWindowId;

class WaylandWindow {
 public:
  enum ShellType {
    None,
    TOPLEVEL,
    FULLSCREEN,
    POPUP,
    CUSTOM
  };

  // Creates a window and maps it to handle.
  explicit WaylandWindow(unsigned handle);
  ~WaylandWindow();

  void SetShellAttributes(ShellType type);
  void SetShellAttributes(ShellType type, WaylandShellSurface* shell_parent,
                          int x, int y);
  void SetWindowTitle(const base::string16& title);
  void Maximize();
  void Minimize();
  void Restore();
  void SetFullscreen();
  void Show();
  void Hide();
  void SetInputRegion(const std::vector<gfx::Rect>& region);
  void SetGroupKeyMask(ui::KeyMask key_mask);
  void SetKeyMask(ui::KeyMask key_mask, bool set);
  void SetWindowProperty(const std::string& name, const std::string& value);
  void SetLocationHint(gfx::LocationHint value);
  void CreateGroup(const ui::WindowGroupConfiguration& config);
  void AttachToGroup(const std::string& group, const std::string& layer);
  void FocusGroupOwner();
  void FocusGroupLayer();
  void DetachGroup();

  ShellType Type() const { return type_; }
  unsigned Handle() const { return handle_; }
  WaylandShellSurface* ShellSurface() const { return shell_surface_; }

  void RealizeAcceleratedWidget();
  void DestroyAcceleratedWidget();

  // Returns pointer to egl window associated with the window.
  // The WaylandWindow object owns the pointer.
  wl_egl_window* egl_window() const;

  // Immediately Resizes window and flushes Wayland Display.
  void Resize(unsigned width, unsigned height);
  void Move(ShellType type,
            WaylandShellSurface* shell_parent,
            const gfx::Rect& rect);
  void AddRegion(int left, int top, int right, int bottom);
  void SubRegion(int left, int top, int right, int bottom);
  gfx::Rect GetBounds() const { return allocation_; }

 private:
  WaylandShellSurface* shell_surface_;
  EGLWindow* window_;

  ShellType type_;
  unsigned handle_;
#if defined(OS_WEBOS)
  WebOSSurfaceGroup* surface_group_;
  bool is_surface_group_client_;
  std::string surface_group_client_layer_;
#endif
  gfx::Rect allocation_;
  DISALLOW_COPY_AND_ASSIGN(WaylandWindow);
};

}  // namespace ozonewayland

#endif  // OZONE_WAYLAND_WINDOW_H_
