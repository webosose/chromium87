// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/widget/desktop_aura/desktop_screen.h"

#include "base/notreached.h"

#if defined(USE_X11)
#include "ui/views/widget/desktop_aura/desktop_screen_x11.h"
#endif

#if defined(USE_OZONE)
///@name USE_NEVA_APPRUNTIME
///@{
#include "ui/ozone/public/ozone_platform.h"
#include "ui/views/widget/desktop_aura/desktop_factory_ozone.h"
///@}
#include "ui/base/ui_base_features.h"
#include "ui/views/widget/desktop_aura/desktop_screen_ozone.h"
#endif

namespace views {

display::Screen* CreateDesktopScreen() {
#if defined(USE_OZONE)
  if (features::IsUsingOzonePlatform()) {
    ///@name USE_NEVA_APPRUNTIME
    ///@{
    if (ui::OzonePlatform::IsWaylandExternal())
      return DesktopFactoryOzone::GetInstance()->CreateDesktopScreen();
    ///@}
    return new DesktopScreenOzone();
  }
#endif
#if defined(USE_X11)
  auto* screen = new DesktopScreenX11();
  screen->Init();
  return screen;
#else
  NOTREACHED();
  return nullptr;
#endif
}

}  // namespace views
