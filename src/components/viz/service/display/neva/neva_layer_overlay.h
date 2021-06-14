// Copyright 2019-2020 LG Electronics, Inc. All rights reserved.
// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Logic is partially copied from DCLayerOverlayProcessor in dc_layer_overlay.h

#ifndef COMPONENTS_VIZ_SERVICE_DISPLAY_NEVA_NEVA_LAYER_OVERLAY_H_
#define COMPONENTS_VIZ_SERVICE_DISPLAY_NEVA_NEVA_LAYER_OVERLAY_H_

#include "base/containers/flat_map.h"
#include "components/viz/common/quads/aggregated_render_pass.h"
#include "gpu/ipc/common/surface_handle.h"
#include "ui/gfx/geometry/rect_f.h"

#if defined(USE_NEVA_MEDIA)
namespace ui {
class VideoWindowController;
}  // namespace ui
#endif  // defined(USE_NEVA_MEDIA)

namespace viz {
class DisplayResourceProvider;

class NevaLayerOverlayProcessor {
 public:
  NevaLayerOverlayProcessor();
  NevaLayerOverlayProcessor(gpu::SurfaceHandle surface_handle);
  ~NevaLayerOverlayProcessor();

  void Process(DisplayResourceProvider* resource_provider,
               const gfx::RectF& display_rect,
               AggregatedRenderPassList* render_passes,
               gfx::Rect* overlay_damage_rect,
               gfx::Rect* damage_rect);

  void ClearOverlayState() {
    previous_frame_underlay_rect_ = gfx::Rect();
    previous_frame_underlay_occlusion_ = gfx::Rect();
  }

 private:
  bool IsVideoHoleDrawQuad(DisplayResourceProvider* resource_provider,
                           const gfx::RectF& display_rect,
                           QuadList::ConstIterator quad_list_begin,
                           QuadList::ConstIterator quad);
  void AddPunchThroughRectIfNeeded(AggregatedRenderPassId id,
                                   const gfx::Rect& rect);

  // Returns an iterator to the element after |it|.
  QuadList::Iterator ProcessAggregatedRenderPassDrawQuad(
      AggregatedRenderPass* render_pass,
      gfx::Rect* damage_rect,
      QuadList::Iterator it);

  void ProcessAggregatedRenderPass(DisplayResourceProvider* resource_provider,
                                   const gfx::RectF& display_rect,
                                   AggregatedRenderPass* render_pass,
                                   bool is_root,
                                   gfx::Rect* overlay_damage_rect,
                                   gfx::Rect* damage_rect);
  bool ProcessForUnderlay(const gfx::RectF& display_rect,
                          AggregatedRenderPass* render_pass,
                          const gfx::Rect& quad_rectangle,
                          const gfx::RectF& occlusion_bounding_box,
                          const QuadList::Iterator& it,
                          bool is_root,
                          gfx::Rect* damage_rect,
                          gfx::Rect* this_frame_underlay_rect,
                          gfx::Rect* this_frame_underlay_occlusion);

  gpu::SurfaceHandle surface_handle_;
#if defined(USE_NEVA_MEDIA)
  ui::VideoWindowController* video_window_controller_ = nullptr;
#endif
  gfx::Rect previous_frame_underlay_rect_;
  gfx::Rect previous_frame_underlay_occlusion_;
  gfx::RectF previous_display_rect_;
  bool processed_overlay_in_frame_ = false;

  // Store information about clipped punch-through rects in target space for
  // non-root render passes. These rects are used to clear the corresponding
  // areas in parent render passes.
  base::flat_map<AggregatedRenderPassId, std::vector<gfx::Rect>>
      pass_punch_through_rects_;

  DISALLOW_COPY_AND_ASSIGN(NevaLayerOverlayProcessor);
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_SERVICE_DISPLAY_NEVA_NEVA_LAYER_OVERLAY_H_
