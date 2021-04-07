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

#ifndef UI_PLATFORM_WINDOW_NEVA_MOJOM_VIDEO_WINDOW_INFO_MOJOM_TRAITS_H_
#define UI_PLATFORM_WINDOW_NEVA_MOJOM_VIDEO_WINDOW_INFO_MOJOM_TRAITS_H_

#include <string>

#include "base/unguessable_token.h"
#include "mojo/public/cpp/base/unguessable_token_mojom_traits.h"
#include "ui/platform_window/neva/mojom/video_window_controller.mojom.h"
#include "ui/platform_window/neva/video_window_info.h"

namespace mojo {

template <>
class StructTraits<ui::mojom::VideoWindowInfoDataView, ui::VideoWindowInfo> {
 public:
  static base::UnguessableToken window_id(const ui::VideoWindowInfo& info) {
    return info.window_id;
  }
  static std::string native_window_id(const ui::VideoWindowInfo& info) {
    return info.native_window_id;
  }

  static bool Read(ui::mojom::VideoWindowInfoDataView data,
                   ui::VideoWindowInfo* out_info);
};

template <>
class StructTraits<ui::mojom::VideoWindowParamsDataView,
                   ui::VideoWindowParams> {
 public:
  static bool use_overlay_processor_layout(
      const ui::VideoWindowParams& params) {
    return params.use_overlay_processor_layout;
  }
  static bool use_video_mute_on_invisible(const ui::VideoWindowParams& params) {
    return params.use_video_mute_on_invisible;
  }
  static bool use_video_mute_on_app_minimized(
      const ui::VideoWindowParams& params) {
    return params.use_video_mute_on_app_minimized;
  }

  static bool Read(ui::mojom::VideoWindowParamsDataView data,
                   ui::VideoWindowParams* out_params);
};

}  // namespace mojo

#endif  // UI_PLATFORM_WINDOW_NEVA_MOJOM_VIDEO_WINDOW_INFO_MOJOM_TRAITS_H_
