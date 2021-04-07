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

#ifndef OZONE_UI_DESKTOP_AURA_DESKTOP_SCREEN_WAYLAND_H_
#define OZONE_UI_DESKTOP_AURA_DESKTOP_SCREEN_WAYLAND_H_

#include <vector>

#include "ozone/platform/desktop_platform_screen_delegate.h"
#include "ui/display/display.h"
#include "ui/display/display_change_notifier.h"
#include "ui/display/screen.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/native_widget_types.h"

namespace aura {
class Window;
}

namespace ui {
class DesktopPlatformScreen;
}

namespace views {

class DesktopScreenWayland : public display::Screen,
                             public ui::DesktopPlatformScreenDelegate {
 public:
  DesktopScreenWayland();
  ~DesktopScreenWayland() override;

  // DesktopPlatformScreenDelegate overrides.
  void OnScreenChanged(unsigned width, unsigned height, int rotation) override;

 private:
  void SetGeometry(const gfx::Rect& geometry);
  // Overridden from display::Screen:
  gfx::Point GetCursorScreenPoint() override;
  bool IsWindowUnderCursor(gfx::NativeWindow window) override;
  gfx::NativeWindow GetWindowAtScreenPoint(const gfx::Point& point) override;
  gfx::NativeWindow GetLocalProcessWindowAtPoint(
      const gfx::Point& point,
      const std::set<gfx::NativeWindow>& ignore) override;
  int GetNumDisplays() const override;
  const std::vector<display::Display>& GetAllDisplays() const override;
  display::Display GetDisplayNearestWindow(gfx::NativeView window) const override;
  display::Display GetDisplayNearestPoint(const gfx::Point& point) const override;
  display::Display GetDisplayMatching(const gfx::Rect& match_rect) const override;
  display::Display GetPrimaryDisplay() const override;
  void AddObserver(display::DisplayObserver* observer) override;
  void RemoveObserver(display::DisplayObserver* observer) override;

  gfx::Rect rect_;
#if defined(OS_WEBOS)
  int rotation_ = 0;
#endif
  display::DisplayChangeNotifier change_notifier_;

  // The display objects we present to chrome.
  std::vector<display::Display> displays_;
  std::unique_ptr<ui::DesktopPlatformScreen> platform_Screen_;
  // Update global screen variable which is required for upstream logic
  display::Screen* const old_screen_ = display::Screen::SetScreenInstance(this);

  DISALLOW_COPY_AND_ASSIGN(DesktopScreenWayland);
};

}  // namespace views

#endif  // OZONE_UI_DESKTOP_AURA_DESKTOP_SCREEN_WAYLAND_H_
