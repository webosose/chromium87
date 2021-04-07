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

#include "media/blink/neva/video_util_neva.h"

namespace media {

// Input:
//  1) video_rect_in_view_space
//   (0, 0)
//   ---------------------------
//   |   -------------         |
//   |   |           |         |
//   |   |           |         |
//   |   -------------         |
//   |   video_rect            |
//   |                         |
//   |                         |
//   --------------------------- (view's width, height)
//
//   Note that video_rect can be different with rect of the corresponding
//   element when object-fit property is provided.
//   For example, let a video's size is (640, 480), and the video's object-fit
//   value is 'cover', and its corresponding element size is (200, 400). Then,
//
//     (0, 0)
//     ---------------------------
//     |   -------------         |
//     |   |    ---    |         |
//     |   |    | |    |         |
//     |   |    | |    |         |
//     |   |    ---element       |
//     |   -------------         |
//     |   video_rect            |
//     |                         |
//     |                         |
//     ---------------------------
//
//   We always try to fit into video_rect. Then exposed played video from
//   element area is expected result.
//
//  2) natural_video_size: visible width and height of a video frame
//
//  3) additional_scale: scale value for accounting into source_rect
//                       and visible_rect
//
//  4) view_rect_in_screen_space
//   (0, 0)
//   -------------------------------------
//   |              widget               |
//   |    ---------------------------    |
//   |    |      (browser UI)       |    |
//   |    ---------------------------    |
//   |    |(view's x, y)            |    |
//   |    |                         |    |
//   |    |                         |    |
//   |    |                         |    |
//   |    |                         |    |
//   |    |                         |    |
//   |    |                         |    |
//   |    |                         |    |
//   |    |                         |    |
//   |    ---------------------------    |
//   |         (view's width, height)    |
//   |                                   |
//   ------------------------------------- (screen width, height)
//
//  5) is_fullscreen_mode: Indicates whether video tag is set as
//     fullscreen, to provide information to screen saver whether
//     screen saver should be work or not.
//
// Output:
//  1) source_rect: source rect in natural video space.
//   (0, 0)
//   ---------------------------
//   |   -------------         |
//   |   |           |         |
//   |   |           |         |
//   |   -------------         |
//   |   source_rect           |
//   |                         |
//   |                         |
//   --------------------------- (natural video's width, height)
//
//  2) visible_rect: visible rect in screen space.
//   (0, 0)
//   -------------------------------------
//   |              widget               |
//   |    ---------------------------    |
//   |    |      (browser UI)       |    |
//   |    ---------------------------    |
//   |    |(view's x, y)            |    |
//   |    |                         |    |
//   |    |                         |    |
//   |    |   -------------         |    |
//   |    |   |           |         |    |
//   |    |   |           |         |    |
//   |    |   -------------         |    |
//   |    |   visible_rect          |    |
//   |    |                         |    |
//   |    |                         |    |
//   |    ---------------------------    |
//   |         (view's width, height)    |
//   |                                   |
//   ------------------------------------- (screen width, height)
//
//  3) is_fullscreen: indicates fullscreen.
//
// Return value:
//  true - if computing results are different with previous results.
//  false - in opposite case
bool ComputeVideoHoleDisplayRect(const gfx::Rect& video_rect_in_view_space,
                                 const gfx::Size& natural_video_size,
                                 const gfx::PointF& additional_scale,
                                 const gfx::Rect& view_rect,
                                 const gfx::Rect& screen_rect,
                                 bool is_fullscreen_mode,
                                 gfx::Rect& source_rect,
                                 gfx::Rect& visible_rect,
                                 bool& is_fullscreen) {
  // Step0: Adjust view offset since overlay processor takes into account view
  // offset already.
  gfx::Rect view_rect_adj = view_rect;
  view_rect_adj.SetByBounds(0, 0, view_rect.right(), view_rect.bottom());

  // Step1: Save previous results. These values are used at last step.
  gfx::Rect prev_visible_rect = visible_rect;
  bool prev_is_fullscreen = is_fullscreen;

  // Step2: Compute source_rect.
  gfx::Rect view_rect_from_origin(0, 0, view_rect_adj.width(),
                                  view_rect_adj.height());
  visible_rect =
      gfx::IntersectRects(video_rect_in_view_space, view_rect_from_origin);

  // We check video_rect is partially visible or not. If video_rect is partially
  // visible, source_rect will be part of natural video's rect. Otherwise, it
  // will be identical with natural video's rect.
  gfx::Rect natural_video_rect(0, 0, natural_video_size.width(),
                               natural_video_size.height());
  if (visible_rect == video_rect_in_view_space || visible_rect.IsEmpty()) {
    source_rect = natural_video_rect;
  } else {
    DCHECK(video_rect_in_view_space.width() != 0 &&
           video_rect_in_view_space.height() != 0);

    int source_x = visible_rect.x() - video_rect_in_view_space.x();
    int source_y = visible_rect.y() - video_rect_in_view_space.y();

    float scale_width = static_cast<float>(natural_video_rect.width()) /
                        video_rect_in_view_space.width();
    float scale_height = static_cast<float>(natural_video_rect.height()) /
                         video_rect_in_view_space.height();

    source_rect = gfx::Rect(source_x, source_y, visible_rect.width(),
                            visible_rect.height());
    source_rect =
        ScaleToEnclosingRectSafe(source_rect, scale_width, scale_height);
  }

  // source_rect must be inside of natural_video_rect
  if (!natural_video_rect.Contains(source_rect)) {
    LOG(ERROR) << __func__
               << " some part of source rect are outside of natural video."
               << "  narual video: " << natural_video_rect.ToString()
               << "  / source rect: " << source_rect.ToString();
    source_rect.Intersect(natural_video_rect);
  }

  // Step3: Adjust visible_rect to view offset.
  visible_rect.Offset(view_rect_adj.x(), view_rect_adj.y());

  // Step4: Adjust visible_rect to screen space.
  visible_rect = ScaleToEnclosingRectSafe(visible_rect, additional_scale.x(),
                                          additional_scale.y());

  // Step5: Determine is_fullscreen.
  is_fullscreen = is_fullscreen_mode || (source_rect == natural_video_rect &&
                                         visible_rect == screen_rect);

  // Step6: Check update.
  bool need_update =
      !visible_rect.IsEmpty() && (prev_visible_rect != visible_rect ||
                                  prev_is_fullscreen != is_fullscreen);

  return need_update;
}

VideoHoleGeometryUpdateHelper::VideoHoleGeometryUpdateHelper(
    blink::WebMediaPlayerClient* client,
    const gfx::PointF additional_contents_scale,
    SetGeometryCallback geometry_cb,
    SetVisibilityCallback visibility_cb)
    : wmp_client_(client),
      additional_contents_scale_(additional_contents_scale),
      geometry_cb_(geometry_cb),
      visibility_cb_(visibility_cb) {}

VideoHoleGeometryUpdateHelper::~VideoHoleGeometryUpdateHelper() = default;

void VideoHoleGeometryUpdateHelper::SetMediaLayerGeometry(
    const gfx::Rect& rect) {
  last_computed_rect_in_view_space_ = rect;
  UpdateVideoHoleBoundary();
}
void VideoHoleGeometryUpdateHelper::SetMediaLayerVisibility(bool visibility) {
  VLOG(1) << __func__ << " visibility=" << visibility;
  has_visibility_ = visibility;
  visibility_cb_.Run(visibility);
}

void VideoHoleGeometryUpdateHelper::SetFullscreenMode(bool mode) {
  if (is_fullscreen_mode_ == mode)
    return;
  is_fullscreen_mode_ = mode;
  UpdateVideoHoleBoundary();
}

void VideoHoleGeometryUpdateHelper::SetNaturalVideoSize(const gfx::Size& size) {
  natural_video_size_ = size;
  UpdateVideoHoleBoundary();
}

void VideoHoleGeometryUpdateHelper::UpdateVideoHoleBoundary() {
  if (!has_visibility_ || !last_computed_rect_in_view_space_.has_value())
    return;
  ComputeVideoHoleDisplayRect(last_computed_rect_in_view_space_.value(),
                              natural_video_size_, additional_contents_scale_,
                              wmp_client_->WebWidgetViewRect(),
                              wmp_client_->ScreenRect(), is_fullscreen_mode_,
                              source_rect_in_video_space_,
                              visible_rect_in_screen_space_, is_fullscreen_);

  if (geometry_cb_) {
    LOG(INFO) << __func__ << " called SetDisplayWindow("
              << "out=[" << visible_rect_in_screen_space_.ToString() << "]"
              << ", in=[" << source_rect_in_video_space_.ToString() << "]"
              << ", is_fullscreen=" << is_fullscreen_ << ")";
    geometry_cb_.Run(visible_rect_in_screen_space_, source_rect_in_video_space_,
                     is_fullscreen_);
  }
}

void VideoHoleGeometryUpdateHelper::SetCallbacks(
    SetGeometryCallback geometry_cb,
    SetVisibilityCallback visibility_cb) {
  geometry_cb_ = geometry_cb;
  visibility_cb_ = visibility_cb;
}

}  // namespace media
