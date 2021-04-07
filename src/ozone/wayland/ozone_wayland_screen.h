// Copyright 2014 Intel Corporation. All rights reserved.
// Copyright 2016-2018 LG Electronics, Inc.
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

#ifndef OZONE_WAYLAND_OZONE_WAYLAND_SCREEN_H_
#define OZONE_WAYLAND_OZONE_WAYLAND_SCREEN_H_

#include <wayland-client.h>

#include "base/macros.h"
#include "ozone/platform/desktop_platform_screen.h"

namespace ui {
class DesktopPlatformScreenDelegate;
class WindowManagerWayland;
}

namespace ozonewayland {

class WaylandScreen;

class OzoneWaylandScreen : public ui::DesktopPlatformScreen {
 public:
  OzoneWaylandScreen(ui::DesktopPlatformScreenDelegate* observer,
                     ui::WindowManagerWayland* window_manager);
  ~OzoneWaylandScreen() override;

  // PlatformScreen:
  gfx::Point GetCursorScreenPoint() override;
  ui::DesktopPlatformScreenDelegate* GetDelegate() const { return observer_; }

 private:
  void LookAheadOutputGeometry();
  // This handler resolves only screen registration. In general you don't want
  // to use this but the one below.
  static void DisplayHandleOutputOnly(
      void *data,
      struct wl_registry *registry,
      uint32_t name,
      const char *interface,
      uint32_t version);
  WaylandScreen* look_ahead_screen_;
  ui::DesktopPlatformScreenDelegate* observer_;
  DISALLOW_COPY_AND_ASSIGN(OzoneWaylandScreen);
};

}  // namespace ozonewayland

#endif  // OZONE_WAYLAND_OZONE_WAYLAND_SCREEN_H_
