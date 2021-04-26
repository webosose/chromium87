// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/ime/text_input_client.h"

///@name USE_NEVA_APPRUNTIME
///@{
#include "ui/gfx/geometry/rect.h"
///@}

namespace ui {

TextInputClient::~TextInputClient() {
}

///@name USE_NEVA_APPRUNTIME
///@{
bool TextInputClient::SystemKeyboardDisabled() const {
  return false;
}

gfx::Rect TextInputClient::GetTextInputBounds() const {
  return gfx::Rect();
}

int TextInputClient::GetTextInputMaxLength() const {
  return -1;
}

gfx::Rect TextInputClient::GetInputPanelRectangle() const {
  return gfx::Rect();
}
///@}

}  // namespace ui
