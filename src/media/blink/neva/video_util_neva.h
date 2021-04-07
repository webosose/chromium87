// Copyright 2018-2020 LG Electronics, Inc.
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

#ifndef MEDIA_BLINK_NEVA_VIDEO_UTIL_NEVA_H_
#define MEDIA_BLINK_NEVA_VIDEO_UTIL_NEVA_H_

#include "base/callback.h"
#include "base/optional.h"
#include "third_party/blink/public/platform/web_media_player_client.h"
#include "ui/gfx/geometry/point_f.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"

namespace media {

class VideoHoleGeometryUpdateHelper {
 public:
  using SetGeometryCallback =
      base::RepeatingCallback<void(const gfx::Rect&, const gfx::Rect&, bool)>;
  using SetVisibilityCallback = base::RepeatingCallback<void(bool)>;
  VideoHoleGeometryUpdateHelper(blink::WebMediaPlayerClient* client,
                                const gfx::PointF additional_contents_scale,
                                SetGeometryCallback geometry_cb,
                                SetVisibilityCallback visibility_cb);
  ~VideoHoleGeometryUpdateHelper();
  void SetMediaLayerGeometry(const gfx::Rect& rect);
  void SetMediaLayerVisibility(bool visibility);
  void SetFullscreenMode(bool fullscreen);
  void SetNaturalVideoSize(const gfx::Size& size);
  void UpdateVideoHoleBoundary();
  void SetCallbacks(SetGeometryCallback geometry_cb,
                    SetVisibilityCallback visibility_cb);

 private:
  blink::WebMediaPlayerClient* const wmp_client_;
  const gfx::PointF additional_contents_scale_;
  SetGeometryCallback geometry_cb_;
  SetVisibilityCallback visibility_cb_;
  bool has_visibility_ = true;

  base::Optional<gfx::Rect> last_computed_rect_in_view_space_ = base::nullopt;
  gfx::Rect source_rect_in_video_space_;
  gfx::Rect visible_rect_in_screen_space_;
  gfx::Size natural_video_size_;
  // Is WebMediaPlayer fullscreen mode
  bool is_fullscreen_mode_ = false;
  // true if WebMediaPlayer is fullscreen mode or calculated geometry covers
  // full screen.
  bool is_fullscreen_ = false;
};

}  // namespace media

#endif  // MEDIA_BLINK_NEVA_VIDEO_UTIL_NEVA_H_
