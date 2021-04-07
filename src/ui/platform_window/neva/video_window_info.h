// Copyright 2020 LG Electronics, Inc.
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

#ifndef UI_PLATFORM_WINDOW_NEVA_VIDEO_WINDOW_INFO_H_
#define UI_PLATFORM_WINDOW_NEVA_VIDEO_WINDOW_INFO_H_

#include <string>

#include "base/component_export.h"
#include "base/unguessable_token.h"

namespace ui {

struct COMPONENT_EXPORT(PLATFORM_WINDOW) VideoWindowInfo {
  base::UnguessableToken window_id;
  std::string native_window_id;
};

struct COMPONENT_EXPORT(PLATFORM_WINDOW) VideoWindowParams {
  // If true, overlay processor will be used for setting video window geometry
  // otherwise the client needs to set geomtery manually.
  bool use_overlay_processor_layout = true;
  // If true, video mute property will be set when video window is out of screen
  bool use_video_mute_on_invisible = true;
  // If true, video mute property will be set when app is minimized
  bool use_video_mute_on_app_minimized = true;
};

}  // namespace ui

#endif  // UI_PLATFORM_WINDOW_NEVA_VIDEO_WINDOW_INFO_H_
