// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_EVENTS_EVENTS_SWITCHES_H_
#define UI_EVENTS_EVENTS_SWITCHES_H_

#include "base/compiler_specific.h"
#include "build/build_config.h"
#include "ui/events/events_base_export.h"

namespace switches {

EVENTS_BASE_EXPORT extern const char kCompensateForUnstablePinchZoom[];

#if defined(OS_LINUX) || defined(OS_CHROMEOS)
EVENTS_BASE_EXPORT extern const char kTouchDevices[];
EVENTS_BASE_EXPORT extern const char kPenDevices[];
#endif

#if defined(USE_X11) || defined(USE_OZONE)
EVENTS_BASE_EXPORT extern const char kEdgeTouchFiltering[];
EVENTS_BASE_EXPORT extern const char kDisableCancelAllTouches[];
#endif

#if defined(USE_OZONE) && defined(OZONE_PLATFORM_WAYLAND_EXTERNAL)
EVENTS_BASE_EXPORT extern const char kIgnoreTouchDevices[];
EVENTS_BASE_EXPORT extern const char kForceMaxTouchPoints[];
#endif  // defined(USE_OZONE) && defined(OZONE_PLATFORM_WAYLAND_EXTERNAL)

}  // namespace switches

#endif  // UI_EVENTS_EVENTS_SWITCHES_H_
