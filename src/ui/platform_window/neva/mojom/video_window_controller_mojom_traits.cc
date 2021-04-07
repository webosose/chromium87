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

#include "ui/platform_window/neva/mojom/video_window_controller_mojom_traits.h"

namespace mojo {

// static
bool StructTraits<ui::mojom::VideoWindowInfoDataView,
                  ui::VideoWindowInfo>::Read(ui::mojom::VideoWindowInfoDataView
                                                 data,
                                             ui::VideoWindowInfo* out_info) {
  base::UnguessableToken window_id;
  std::string native_window_id;
  if (!data.ReadWindowId(&window_id))
    return false;
  if (!data.ReadNativeWindowId(&native_window_id))
    return false;

  out_info->window_id = window_id;
  out_info->native_window_id = native_window_id;

  return true;
}

// static
bool StructTraits<ui::mojom::VideoWindowParamsDataView, ui::VideoWindowParams>::
    Read(ui::mojom::VideoWindowParamsDataView data,
         ui::VideoWindowParams* out_info) {
  out_info->use_overlay_processor_layout = data.use_overlay_processor_layout();
  out_info->use_video_mute_on_invisible = data.use_video_mute_on_invisible();
  out_info->use_video_mute_on_app_minimized =
      data.use_video_mute_on_app_minimized();

  return true;
}

}  // namespace mojo
