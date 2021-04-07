// Copyright 2019-2020 LG Electronics, Inc. All rights reserved.
// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Logic is partially copied from DCLayerOverlayProcessor in dc_layer_overlay.cc

#include "components/viz/service/display/neva/neva_layer_overlay.h"

#include "cc/base/math_util.h"
#include "components/viz/common/quads/aggregated_render_pass_draw_quad.h"
#include "components/viz/common/quads/solid_color_draw_quad.h"
#include "components/viz/common/quads/video_hole_draw_quad.h"
#include "components/viz/service/display/display_resource_provider.h"
#include "ui/gfx/geometry/rect_conversions.h"
#if defined(USE_NEVA_MEDIA)
#include "ui/ozone/public/ozone_platform.h"
#include "ui/platform_window/neva/video_window_controller.h"
#endif  // defined(USE_NEVA_MEDIA)

namespace viz {

namespace {

// This returns the smallest rectangle in target space that contains the quad.
gfx::RectF ClippedQuadRectangle(const DrawQuad* quad) {
  gfx::RectF quad_rect = cc::MathUtil::MapClippedRect(
      quad->shared_quad_state->quad_to_target_transform,
      gfx::RectF(quad->rect));
  if (quad->shared_quad_state->is_clipped)
    quad_rect.Intersect(gfx::RectF(quad->shared_quad_state->clip_rect));
  return quad_rect;
}

// Find a rectangle containing all the quads in a list that occlude the area
// in target_quad.
gfx::RectF GetOcclusionBounds(const gfx::RectF& target_quad,
                              QuadList::ConstIterator quad_list_begin,
                              QuadList::ConstIterator quad_list_end) {
  gfx::RectF occlusion_bounding_box;
  for (auto overlap_iter = quad_list_begin; overlap_iter != quad_list_end;
       ++overlap_iter) {
    float opacity = overlap_iter->shared_quad_state->opacity;
    if (opacity < std::numeric_limits<float>::epsilon())
      continue;
    const DrawQuad* quad = *overlap_iter;
    gfx::RectF overlap_rect = ClippedQuadRectangle(quad);
    if (quad->material == DrawQuad::Material::kSolidColor) {
      SkColor color = SolidColorDrawQuad::MaterialCast(quad)->color;
      float alpha = (SkColorGetA(color) * (1.0f / 255.0f)) * opacity;
      if (quad->ShouldDrawWithBlending() &&
          alpha < std::numeric_limits<float>::epsilon())
        continue;
    }
    overlap_rect.Intersect(target_quad);
    if (!overlap_rect.IsEmpty()) {
      occlusion_bounding_box.Union(overlap_rect);
    }
  }
  return occlusion_bounding_box;
}

}  // namespace

NevaLayerOverlayProcessor::NevaLayerOverlayProcessor() {
  LOG(ERROR) << "This constructor is not intended to used.";
}

NevaLayerOverlayProcessor::NevaLayerOverlayProcessor(
    gpu::SurfaceHandle surface_handle)
    : surface_handle_(surface_handle) {
#if defined(USE_NEVA_MEDIA)
  video_window_controller_ =
      ui::OzonePlatform::GetInstance()->GetVideoWindowController();
#endif  // defined(USE_NEVA_MEDIA)
}

NevaLayerOverlayProcessor::~NevaLayerOverlayProcessor() = default;

bool NevaLayerOverlayProcessor::IsVideoHoleDrawQuad(
    DisplayResourceProvider* resource_provider,
    const gfx::RectF& display_rect,
    QuadList::ConstIterator quad_list_begin,
    QuadList::ConstIterator quad) {
  if (!quad->shared_quad_state ||
      quad->shared_quad_state->blend_mode != SkBlendMode::kSrcOver)
    return false;

  switch (quad->material) {
    case DrawQuad::Material::kVideoHole:
      return true;
    default:
      return false;
  }

  return false;
}

void NevaLayerOverlayProcessor::AddPunchThroughRectIfNeeded(
    AggregatedRenderPassId id,
    const gfx::Rect& rect) {
  bool add_punch_through_rect = true;
  const auto& punch_through_rects = pass_punch_through_rects_[id];
  for (const gfx::Rect& punch_through_rect : punch_through_rects) {
    if (punch_through_rect == rect) {
      add_punch_through_rect = false;
      break;
    }
  }

  if (add_punch_through_rect)
    pass_punch_through_rects_[id].push_back(rect);
}

void NevaLayerOverlayProcessor::Process(
    DisplayResourceProvider* resource_provider,
    const gfx::RectF& display_rect,
    AggregatedRenderPassList* render_passes,
    gfx::Rect* overlay_damage_rect,
    gfx::Rect* damage_rect) {
#if defined(USE_NEVA_MEDIA)
  if (video_window_controller_ && video_window_controller_->IsInitialized())
    video_window_controller_->BeginOverlayProcessor(surface_handle_);
#endif  // defined(USE_NEVA_MEDIA)
  processed_overlay_in_frame_ = false;
  pass_punch_through_rects_.clear();
  for (auto& pass : *render_passes) {
    bool is_root = (pass == render_passes->back());
    ProcessAggregatedRenderPass(resource_provider, display_rect, pass.get(),
                                is_root, overlay_damage_rect,
                                is_root ? damage_rect : &pass->damage_rect);
  }
#if defined(USE_NEVA_MEDIA)
  if (video_window_controller_ && video_window_controller_->IsInitialized())
    video_window_controller_->EndOverlayProcessor(surface_handle_);
#endif  // defined(USE_NEVA_MEDIA)
}

QuadList::Iterator
NevaLayerOverlayProcessor::ProcessAggregatedRenderPassDrawQuad(
    AggregatedRenderPass* render_pass,
    gfx::Rect* damage_rect,
    QuadList::Iterator it) {
  DCHECK_EQ(DrawQuad::Material::kAggregatedRenderPass, it->material);
  const AggregatedRenderPassDrawQuad* arpdq =
      AggregatedRenderPassDrawQuad::MaterialCast(*it);

  ++it;
  // Check if this quad is broken to avoid corrupting pass_info.
  if (arpdq->render_pass_id == render_pass->id)
    return it;
  // |pass_punch_through_rects_| will be empty unless non-root overlays are
  // enabled.
  if (!pass_punch_through_rects_.count(arpdq->render_pass_id))
    return it;

  // Punch holes through for all child video quads that will be displayed in
  // underlays. This doesn't work perfectly in all cases - it breaks with
  // complex overlap or filters - but it's needed to be able to display these
  // videos at all. The EME spec allows that some HTML rendering capabilities
  // may be unavailable for EME videos.
  //
  // The solid color quads are inserted after the RPDQ, so they'll be drawn
  // before it and will only cut out contents behind it. A kDstOut solid color
  // quad is used with an accumulated opacity to do the hole punching, because
  // with premultiplied alpha that reduces the opacity of the current content
  // by the opacity of the layer.
  const auto& punch_through_rects =
      pass_punch_through_rects_[arpdq->render_pass_id];
  const SharedQuadState* original_shared_quad_state = arpdq->shared_quad_state;

  // Copy shared state from RPDQ to get the same clip rect.
  SharedQuadState* new_shared_quad_state =
      render_pass->shared_quad_state_list.AllocateAndCopyFrom<SharedQuadState>(
          original_shared_quad_state);

  bool should_blend =
      arpdq->ShouldDrawWithBlending() &&
      original_shared_quad_state->blend_mode == SkBlendMode::kSrcOver &&
      original_shared_quad_state->opacity < 1.f;

  gfx::Rect visible_rect = arpdq->visible_rect;

  // The iterator was advanced above so InsertBefore inserts after the RPDQ.
  it = render_pass->quad_list
           .InsertBeforeAndInvalidateAllPointers<SolidColorDrawQuad>(
               it, punch_through_rects.size());
  arpdq = nullptr;
  for (const gfx::Rect& punch_through_rect : punch_through_rects) {
    SkBlendMode new_blend_mode = should_blend
                                     ? SkBlendMode::kDstOut
                                     : original_shared_quad_state->blend_mode;
    new_shared_quad_state->blend_mode = new_blend_mode;

    gfx::Rect clipped_punch_through_rect = punch_through_rect;
    clipped_punch_through_rect.Intersect(visible_rect);

    auto* solid_quad = static_cast<SolidColorDrawQuad*>(*it++);
    solid_quad->SetAll(new_shared_quad_state, clipped_punch_through_rect,
                       clipped_punch_through_rect, false,
                       should_blend ? SK_ColorBLACK : SK_ColorTRANSPARENT,
                       true);

    gfx::Rect clipped_quad_rect =
        gfx::ToEnclosingRect(ClippedQuadRectangle(solid_quad));
    // Propagate punch through rect as damage up the stack of render passes.
    // TODO(sunnyps): We should avoid this extra damage if we knew that the
    // video (in child render surface) was the only thing damaging this
    // render surface.
    damage_rect->Union(clipped_quad_rect);

    // Add transformed info to list in case this renderpass is included in
    // another pass.
    AddPunchThroughRectIfNeeded(render_pass->id, clipped_quad_rect);
  }
  return it;
}

void NevaLayerOverlayProcessor::ProcessAggregatedRenderPass(
    DisplayResourceProvider* resource_provider,
    const gfx::RectF& display_rect,
    AggregatedRenderPass* render_pass,
    bool is_root,
    gfx::Rect* overlay_damage_rect,
    gfx::Rect* damage_rect) {
  gfx::Rect this_frame_underlay_rect;
  gfx::Rect this_frame_underlay_occlusion;

  QuadList* quad_list = &render_pass->quad_list;
  auto next_it = quad_list->begin();
  for (auto it = quad_list->begin(); it != quad_list->end(); it = next_it) {
    next_it = it;
    ++next_it;
    // next_it may be modified inside the loop if methods modify the quad list
    // and invalidate iterators to it.

    if (it->material == DrawQuad::Material::kAggregatedRenderPass) {
      next_it =
          ProcessAggregatedRenderPassDrawQuad(render_pass, damage_rect, it);
      continue;
    }

    if (!IsVideoHoleDrawQuad(resource_provider, display_rect,
                             quad_list->begin(), it)) {
      continue;
    }

    // These rects are in quad target space.
    gfx::Rect quad_rectangle_in_target_space =
        gfx::ToEnclosingRect(ClippedQuadRectangle(*it));
    gfx::RectF occlusion_bounding_box = GetOcclusionBounds(
        gfx::RectF(quad_rectangle_in_target_space), quad_list->begin(), it);
    bool processed_overlay = false;

    if (ProcessForUnderlay(
            display_rect, render_pass, quad_rectangle_in_target_space,
            occlusion_bounding_box, it, is_root, damage_rect,
            &this_frame_underlay_rect, &this_frame_underlay_occlusion)) {
      processed_overlay = true;
    }

    if (processed_overlay) {
      gfx::Rect rect_in_root = cc::MathUtil::MapEnclosingClippedRect(
          render_pass->transform_to_root_target,
          quad_rectangle_in_target_space);
      overlay_damage_rect->Union(rect_in_root);

      // Only allow one overlay unless non-root overlays are enabled.
      processed_overlay_in_frame_ = true;
    }
#if defined(USE_NEVA_MEDIA)
    if ((*it)->material == DrawQuad::Material::kVideoHole) {
      auto& transform = (*it)->shared_quad_state->quad_to_target_transform;
      gfx::RectF quad_rect_in_root_target = gfx::RectF((*it)->rect);
      transform.TransformRect(&quad_rect_in_root_target);
      render_pass->transform_to_root_target.TransformRect(
          &quad_rect_in_root_target);
      base::UnguessableToken overlay_plane_id =
          VideoHoleDrawQuad::MaterialCast(*it)->overlay_plane_id;
      render_pass->quad_list.ReplaceExistingQuadWithOpaqueTransparentSolidColor(
          it);
      if (video_window_controller_ &&
          video_window_controller_->IsInitialized()) {
        video_window_controller_->NotifyVideoWindowGeometryChanged(
            surface_handle_, overlay_plane_id,
            gfx::ToEnclosingRect(quad_rect_in_root_target));
      }
    }
#endif  // defined(USE_NEVA_MEDIA)
  }
  if (is_root) {
    damage_rect->Intersect(gfx::ToEnclosingRect(display_rect));
    previous_display_rect_ = display_rect;
    previous_frame_underlay_rect_ = this_frame_underlay_rect;
    previous_frame_underlay_occlusion_ = this_frame_underlay_occlusion;
  }
}

bool NevaLayerOverlayProcessor::ProcessForUnderlay(
    const gfx::RectF& display_rect,
    AggregatedRenderPass* render_pass,
    const gfx::Rect& quad_rectangle,
    const gfx::RectF& occlusion_bounding_box,
    const QuadList::Iterator& it,
    bool is_root,
    gfx::Rect* damage_rect,
    gfx::Rect* this_frame_underlay_rect,
    gfx::Rect* this_frame_underlay_occlusion) {
  if ((it->shared_quad_state->opacity < 1.0))
    return false;

  if (processed_overlay_in_frame_)
    return false;

  const SharedQuadState* shared_quad_state = it->shared_quad_state;
  bool display_rect_changed = (display_rect != previous_display_rect_);
  bool underlay_rect_changed =
      (quad_rectangle != previous_frame_underlay_rect_);
  bool is_axis_aligned =
      shared_quad_state->quad_to_target_transform.Preserves2dAxisAlignment();

  if (is_root && !processed_overlay_in_frame_ && is_axis_aligned &&
      !underlay_rect_changed && !display_rect_changed) {
    // If this underlay rect is the same as for last frame, subtract its area
    // from the damage of the main surface, as the cleared area was already
    // cleared last frame. Add back the damage from the occluded area for this
    // and last frame, as that may have changed.
    gfx::Rect occluding_damage_rect = *damage_rect;
    damage_rect->Subtract(quad_rectangle);

    gfx::Rect occlusion = gfx::ToEnclosingRect(occlusion_bounding_box);
    occlusion.Union(previous_frame_underlay_occlusion_);

    occluding_damage_rect.Intersect(quad_rectangle);
    occluding_damage_rect.Intersect(occlusion);

    damage_rect->Union(occluding_damage_rect);
  } else {
    // Entire replacement quad must be redrawn.
    // TODO(sunnyps): We should avoid this extra damage if we knew that the
    // video was the only thing damaging this render surface.
    damage_rect->Union(quad_rectangle);
  }

  // We only compare current frame's first root pass underlay with the previous
  // frame's first root pass underlay. Non-opaque regions can have different
  // alpha from one frame to another so this optimization doesn't work.
  if (is_root && !processed_overlay_in_frame_ && is_axis_aligned) {
    *this_frame_underlay_rect = quad_rectangle;
    *this_frame_underlay_occlusion =
        gfx::ToEnclosingRect(occlusion_bounding_box);
  }

  // Propagate the punched holes up the chain of render passes. Punch through
  // rects are in quad target (child render pass) space, and are transformed to
  // RPDQ target (parent render pass) in ProcessAggregatedRenderPassDrawQuad().
  pass_punch_through_rects_[render_pass->id].push_back(
      gfx::ToEnclosingRect(ClippedQuadRectangle(*it)));

  return true;
}

}  // namespace viz
