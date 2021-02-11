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

#include "ozone/wayland/ozone_wayland_screen.h"

#include "ozone/platform/desktop_platform_screen_delegate.h"
#include "ozone/platform/window_manager_wayland.h"
#include "ozone/wayland/screen.h"

namespace ozonewayland {

OzoneWaylandScreen::OzoneWaylandScreen(
    ui::DesktopPlatformScreenDelegate* observer,
    ui::WindowManagerWayland* window_manager)
    : look_ahead_screen_list_(), observer_(observer) {
  LookAheadOutputGeometry();
  window_manager->OnPlatformScreenCreated(this);
}

OzoneWaylandScreen::~OzoneWaylandScreen() {
}

gfx::Point OzoneWaylandScreen::GetCursorScreenPoint() {
  return gfx::Point();
}

void OzoneWaylandScreen::LookAheadOutputGeometry() {
  wl_display* display = wl_display_connect(nullptr);
  if (!display)
    return;

  static const struct wl_registry_listener registry_output = {
    OzoneWaylandScreen::DisplayHandleOutputOnly
  };

  wl_registry* registry = wl_display_get_registry(display);
  wl_registry_add_listener(registry, &registry_output, this);

  if (wl_display_roundtrip(display) > 0) {
    for (WaylandScreen* look_ahead_screen : look_ahead_screen_list_) {
#if defined(OS_WEBOS)
      while (look_ahead_screen->Geometry().IsEmpty() ||
          !look_ahead_screen->GetOutputTransform().has_value())
#else
      while (look_ahead_screen->Geometry().IsEmpty())
#endif
        wl_display_roundtrip(display);

      observer_->OnScreenChanged(
          look_ahead_screen->GetDisplayId(),
          look_ahead_screen->GetDisplayName(),
          look_ahead_screen->Geometry().width(),
          look_ahead_screen->Geometry().height(),
          look_ahead_screen->GetOutputTransformDegrees());
    }
  }

  for (WaylandScreen* look_ahead_screen : look_ahead_screen_list_)
    delete look_ahead_screen;
  look_ahead_screen_list_.clear();

  wl_registry_destroy(registry);
  wl_display_flush(display);
  wl_display_disconnect(display);
}

void OzoneWaylandScreen::DisplayHandleOutputOnly(void *data,
                                                 struct wl_registry *registry,
                                                 uint32_t name,
                                                 const char *interface,
                                                 uint32_t version) {
  OzoneWaylandScreen* disp = static_cast<OzoneWaylandScreen*>(data);

  if (strcmp(interface, "wl_output") == 0) {
    WaylandScreen* screen = new WaylandScreen(registry, name);
    disp->look_ahead_screen_list_.push_back(screen);
  }
}

}  // namespace ozonewayland
