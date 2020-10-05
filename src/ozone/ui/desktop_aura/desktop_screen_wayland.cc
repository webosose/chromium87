// Copyright 2013 Intel Corporation. All rights reserved.
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

#include "ozone/ui/desktop_aura/desktop_screen_wayland.h"

#include "ozone/platform/desktop_platform_screen.h"
#include "ozone/platform/ozone_platform_wayland.h"
#include "ozone/ui/desktop_aura/desktop_window_tree_host_ozone_wayland.h"
#include "ui/aura/window.h"
#include "ui/views/widget/desktop_aura/desktop_screen.h"

#if defined(OS_WEBOS)
#include "base/environment.h"
#endif

namespace views {

DesktopScreenWayland::DesktopScreenWayland()
    : display::Screen(),
      rect_(0, 0, 0, 0),
      displays_() {
  platform_Screen_ = CreatePlatformScreen(this);
}

DesktopScreenWayland::~DesktopScreenWayland() {
  display::Screen::SetScreenInstance(old_screen_);
}

void DesktopScreenWayland::SetGeometry(const gfx::Rect& geometry) {
  rect_ = geometry;
  int max_area = 0;
  const display::Display* matching = nullptr;
  for (std::vector<display::Display>::const_iterator it = displays_.begin();
       it != displays_.end(); ++it) {
    gfx::Rect intersect = gfx::IntersectRects(it->bounds(), rect_);
    int area = intersect.width() * intersect.height();
    if (area > max_area) {
      max_area = area;
      matching = &*it;
    }
  }

  if (!matching) {
    std::vector<display::Display> old_displays = displays_;
    displays_.push_back(display::Display(displays_.size(), rect_));
    change_notifier_.NotifyDisplaysChanged(old_displays, displays_);
  }
}

display::Display DesktopScreenWayland::GetDisplayByDisplayId(
    int display_id) const {
  for (auto& display : displays_)
    if (display.id() == display_id)
      return display;
  return GetPrimaryDisplay();
}

gfx::Point DesktopScreenWayland::GetCursorScreenPoint() {
  return platform_Screen_->GetCursorScreenPoint();
}

bool DesktopScreenWayland::IsWindowUnderCursor(gfx::NativeWindow window) {
  NOTIMPLEMENTED();
  return false;
}

gfx::NativeWindow DesktopScreenWayland::GetWindowAtScreenPoint(
    const gfx::Point& point) {
  const std::vector<aura::Window*>& windows =
      views::DesktopWindowTreeHostOzone::GetAllOpenWindows();
  for (std::vector<aura::Window*>::const_iterator it = windows.begin();
         it != windows.end(); ++it) {
    if ((*it)->GetBoundsInScreen().Contains(point))
      return *it;
  }

  return nullptr;
}

gfx::NativeWindow DesktopScreenWayland::GetLocalProcessWindowAtPoint(
    const gfx::Point& point,
    const std::set<gfx::NativeWindow>& ignore) {
  return nullptr;
}

int DesktopScreenWayland::GetNumDisplays() const {
  return displays_.size();
}

const std::vector<display::Display>& DesktopScreenWayland::GetAllDisplays() const {
  return displays_;
}

display::Display DesktopScreenWayland::GetDisplayNearestWindow(
    gfx::NativeView window) const {
  DCHECK(!rect_.IsEmpty());
  if (displays_.size() == 1)
    return displays_.front();
  // Getting screen bounds here safely is hard.
  //
  // You'd think we'd be able to just call window->GetBoundsInScreen(), but we
  // can't because |window| (and the associated RootWindow*) can be partially
  // initialized at this point; RootWindow initializations call through into
  // GetDisplayNearestWindow(). But the wayland resources are created before we
  // create the aura::RootWindow. So we ask what the DRWH believes the
  // window bounds are instead of going through the aura::Window's screen
  // bounds.
  aura::WindowTreeHost* host = nullptr;
  if (window)
    host = window->GetHost();
  if (host) {
    DesktopWindowTreeHostOzone* rwh =
        DesktopWindowTreeHostOzone::GetHostForAcceleratedWidget(
            host->GetAcceleratedWidget());
    if (rwh)
#if defined(OS_WEBOS)
      return GetDisplayByDisplayId(std::stoi(rwh->GetDisplayId()));
#else
      return GetDisplayMatching(rwh->GetBoundsInScreen());
#endif
  }

  return GetPrimaryDisplay();
}

display::Display DesktopScreenWayland::GetDisplayNearestPoint(
    const gfx::Point& point) const {
  if (displays_.size() == 1)
    return displays_.front();

#if defined(OS_WEBOS)
  return GetPrimaryDisplay();
#endif

  for (std::vector<display::Display>::const_iterator it = displays_.begin();
         it != displays_.end(); ++it) {
    if (it->bounds().Contains(point))
      return *it;
  }

  return GetPrimaryDisplay();
}

display::Display DesktopScreenWayland::GetDisplayMatching(
    const gfx::Rect& match_rect) const {
  if (displays_.size() == 1)
    return displays_.front();

#if defined(OS_WEBOS)
  return GetPrimaryDisplay();
#endif

  DCHECK(!rect_.IsEmpty());
  int max_area = 0;
  const display::Display* matching = nullptr;
  for (std::vector<display::Display>::const_iterator it = displays_.begin();
       it != displays_.end(); ++it) {
    gfx::Rect intersect = gfx::IntersectRects(it->bounds(), match_rect);
    int area = intersect.width() * intersect.height();
    if (area > max_area) {
      max_area = area;
      matching = &*it;
    }
  }

  // Fallback to the primary display if there is no matching display.
  return matching ? *matching : GetPrimaryDisplay();
}

display::Display DesktopScreenWayland::GetPrimaryDisplay() const {
#if defined(OS_WEBOS)
  // webOS specific way to determine the screen to use:
  // DISPLAY_ID is set by the application manager meaning the display ID to use.
  std::unique_ptr<base::Environment> env(base::Environment::Create());
  std::string display_id_env;
  if (env->GetVar("DISPLAY_ID", &display_id_env)) {
    int display_id = std::stoi(display_id_env);
    for (auto& display : displays_)
      if (display.id() == display_id)
        return display;
  }
  return displays_.front();
#else
  DCHECK(!rect_.IsEmpty());
  return displays_.front();
#endif
}

void DesktopScreenWayland::AddObserver(display::DisplayObserver* observer) {
  change_notifier_.AddObserver(observer);
}

void DesktopScreenWayland::RemoveObserver(display::DisplayObserver* observer) {
  change_notifier_.RemoveObserver(observer);
}

void DesktopScreenWayland::OnScreenChanged(const std::string& display_id,
                                           const std::string& display_name,
                                           unsigned width,
                                           unsigned height,
                                           int rotation) {
#if defined(OS_WEBOS)
  std::vector<display::Display> old_displays = displays_;

  bool matching = false;
  bool displays_changed = false;
  int displayid = 0;
  gfx::Rect display_rect = gfx::Rect(0, 0, width, height);

  if (!display_id.empty())
    displayid = std::stoi(display_id);

  for (auto& display : displays_) {
    if (display.id() == displayid) {
      matching = true;
      if (display.bounds() != display_rect ||
          display.RotationAsDegree() != rotation) {
        display.set_bounds(display_rect);
        display.set_work_area(display_rect);
        display.SetRotationAsDegree(rotation);
        displays_changed = true;
      }
    }
  }

  if (!matching) {
    display::Display display(displayid, display_rect);
    display.set_work_area(display_rect);
    display.SetRotationAsDegree(rotation);
    displays_.push_back(display);
    displays_changed = true;
  }

  if (displays_changed)
    change_notifier_.NotifyDisplaysChanged(old_displays, displays_);
#else
  SetGeometry(gfx::Rect(0, 0, width, height));
#endif
}

}  // namespace views
