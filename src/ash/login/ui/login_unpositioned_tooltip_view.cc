// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/login/ui/login_unpositioned_tooltip_view.h"

#include "ash/login/ui/non_accessible_view.h"
#include "ash/login/ui/views_utils.h"
#include "ui/accessibility/ax_enums.mojom.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/vector_icons.h"

namespace ash {

namespace {

// The size of the info icon in the tooltip view.
constexpr int kInfoIconSizeDp = 20;

}  // namespace

LoginUnpositionedTooltipView::LoginUnpositionedTooltipView(
    const base::string16& message,
    views::View* anchor_view)
    : LoginBaseBubbleView(anchor_view) {
  views::ImageView* info_icon = new views::ImageView();
  info_icon->SetPreferredSize(gfx::Size(kInfoIconSizeDp, kInfoIconSizeDp));
  info_icon->SetImage(gfx::CreateVectorIcon(views::kInfoIcon, SK_ColorWHITE));
  AddChildView(info_icon);

  label_ =
      login_views_utils::CreateBubbleLabel(message, gfx::kGoogleGrey200, this);
  AddChildView(label_);
}

LoginUnpositionedTooltipView::~LoginUnpositionedTooltipView() = default;

void LoginUnpositionedTooltipView::SetText(const base::string16& message) {
  label_->SetText(message);
}

void LoginUnpositionedTooltipView::GetAccessibleNodeData(
    ui::AXNodeData* node_data) {
  node_data->role = ax::mojom::Role::kTooltip;
}

}  // namespace ash
