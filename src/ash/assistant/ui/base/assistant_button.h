// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_ASSISTANT_UI_BASE_ASSISTANT_BUTTON_H_
#define ASH_ASSISTANT_UI_BASE_ASSISTANT_BUTTON_H_

#include <memory>

#include "base/component_export.h"
#include "base/macros.h"
#include "base/optional.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/color_palette.h"
#include "ui/views/controls/button/image_button.h"

namespace gfx {
struct VectorIcon;
}  // namespace gfx

namespace views {
class ImageButton;
}  // namespace views

namespace ash {

class AssistantButtonListener;
enum class AssistantButtonId;

class COMPONENT_EXPORT(ASSISTANT_UI) AssistantButton
    : public views::ImageButton,
      public views::ButtonListener {
 public:
  // Initialization parameters for customizing the Assistant button.
  struct InitParams {
    InitParams();

    InitParams(InitParams&&);
    InitParams& operator=(InitParams&&) = default;

    ~InitParams();

    // Size of the Assistant button.
    int size_in_dip = 0;

    // Params for the icon.
    int icon_size_in_dip = 0;
    SkColor icon_color = gfx::kGoogleGrey700;

    // ID of the localization string for the button's accessible name.
    base::Optional<int> accessible_name_id;

    // ID of the localization string for the button's tooltip text.
    base::Optional<int> tooltip_id;
  };

  AssistantButton(AssistantButtonListener* listener,
                  AssistantButtonId button_id);
  ~AssistantButton() override;

  // Creates a button with the default Assistant styles.
  static std::unique_ptr<AssistantButton> Create(
      AssistantButtonListener* listener,
      const gfx::VectorIcon& icon,
      AssistantButtonId button_id,
      InitParams params);

  AssistantButtonId GetAssistantButtonId() const { return id_; }

  // views::Button:
  const char* GetClassName() const override;
  void OnBoundsChanged(const gfx::Rect& previous_bounds) override;
  std::unique_ptr<views::InkDrop> CreateInkDrop() override;
  std::unique_ptr<views::InkDropHighlight> CreateInkDropHighlight()
      const override;
  std::unique_ptr<views::InkDropRipple> CreateInkDropRipple() const override;

  // views::ButtonListener:
  void ButtonPressed(views::Button* sender, const ui::Event& event) override;

 private:
  AssistantButtonListener* listener_;
  const AssistantButtonId id_;

  DISALLOW_COPY_AND_ASSIGN(AssistantButton);
};

}  // namespace ash

#endif  // ASH_ASSISTANT_UI_BASE_ASSISTANT_BUTTON_H_
