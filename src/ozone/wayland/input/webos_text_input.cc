// Copyright 2017-2019 LG Electronics, Inc.
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

#include "ozone/wayland/input/webos_text_input.h"

#include <curses.h>
#include <linux/input.h>

#include <string>

#include "base/logging.h"
#include "ozone/platform/webos_constants.h"
#include "ozone/wayland/display.h"
#include "ozone/wayland/input/keyboard.h"
#include "ozone/wayland/input/text_input_utils.h"
#include "ozone/wayland/seat.h"
#include "ozone/wayland/shell/shell_surface.h"
#include "ozone/wayland/window.h"
#include "ui/base/ime/neva/input_method_common.h"
#include "ui/base/ime/text_input_flags.h"
#include "ui/events/keycodes/keyboard_code_conversion.h"
#include "ui/events/keycodes/xkb_keysym.h"
#include "ui/base/ime/text_input_flags.h"

namespace ozonewayland {

uint32_t ContentHintFromInputContentType(ui::InputContentType content_type,
                                         int input_flags) {
  uint32_t wl_hint = (TEXT_MODEL_CONTENT_HINT_AUTO_COMPLETION |
                      TEXT_MODEL_CONTENT_HINT_AUTO_CAPITALIZATION);
  if (content_type == ui::InputContentType::kPassword)
    wl_hint |= TEXT_MODEL_CONTENT_HINT_PASSWORD;

  // hint from flags
  // TODO TEXT_INPUT_FLAG_SPELLCHECK_ON remains.
  //      The wayland-text-client doesn't offer the spellcheck yet.
  if (input_flags & ui::TEXT_INPUT_FLAG_SENSITIVE_ON)
    wl_hint |= TEXT_MODEL_CONTENT_HINT_SENSITIVE_DATA;
  if (input_flags & ui::TEXT_INPUT_FLAG_AUTOCOMPLETE_ON)
    wl_hint |= TEXT_MODEL_CONTENT_HINT_AUTO_COMPLETION;
  if (input_flags & ui::TEXT_INPUT_FLAG_AUTOCORRECT_ON)
    wl_hint |= TEXT_MODEL_CONTENT_HINT_AUTO_CORRECTION;

  return wl_hint;
}

uint32_t ContentPurposeFromInputContentType(ui::InputContentType content_type) {
  switch (content_type) {
    case ui::InputContentType::kPassword:
      return TEXT_MODEL_CONTENT_PURPOSE_PASSWORD;
    case ui::InputContentType::kEmail:
      return TEXT_MODEL_CONTENT_PURPOSE_EMAIL;
    case ui::InputContentType::kNumber:
      return TEXT_MODEL_CONTENT_PURPOSE_NUMBER;
    case ui::InputContentType::kTelephone:
      return TEXT_MODEL_CONTENT_PURPOSE_PHONE;
    case ui::InputContentType::kUrl:
      return TEXT_MODEL_CONTENT_PURPOSE_URL;
    case ui::InputContentType::kDate:
      return TEXT_MODEL_CONTENT_PURPOSE_DATE;
    case ui::InputContentType::kDateTime:
    case ui::InputContentType::kDateTimeLocal:
      return TEXT_MODEL_CONTENT_PURPOSE_DATETIME;
    case ui::InputContentType::kTime:
      return TEXT_MODEL_CONTENT_PURPOSE_TIME;
    default:
      return TEXT_MODEL_CONTENT_PURPOSE_NORMAL;
  }
}

static uint32_t serial = 0;

WaylandTextInput::WaylandTextInput(WaylandSeat* seat) : seat_(seat) {}

WaylandTextInput::~WaylandTextInput() {
  for (auto& input_panel_item : input_panel_map_)
    input_panel_item.second->Deactivate();
}

void WaylandTextInput::SetActiveWindow(WaylandWindow* window) {}

void WaylandTextInput::ResetIme(unsigned handle) {
  WaylandTextInput::InputPanel* panel = GetInputPanel(handle);
  if (panel && panel->model)
    text_model_reset(panel->model, serial);
  else
    CreateInputPanel(handle);
}

text_model* WaylandTextInput::CreateTextModel() {
  text_model* model = nullptr;
  text_model_factory* factory =
      WaylandDisplay::GetInstance()->GetTextModelFactory();
  if (factory)
    model = text_model_factory_create_text_model(factory);
  return model;
}

WaylandTextInput::InputPanel* WaylandTextInput::CreateInputPanel(
    unsigned handle) {
  WaylandTextInput::InputPanel* panel =
      new InputPanel(CreateTextModel(), seat_, handle);
  input_panel_map_[handle].reset(panel);
  return panel;
}

WaylandTextInput::InputPanel* WaylandTextInput::GetInputPanel(unsigned handle) {
  if (input_panel_map_.find(handle) != input_panel_map_.end()) {
    return input_panel_map_[handle].get();
  }
  return nullptr;
}

void WaylandTextInput::ShowInputPanel(wl_seat* input_seat, unsigned handle) {
  // Check that window is valid.
  WaylandWindow* window = nullptr;
  if (handle) {
    window = WaylandDisplay::GetInstance()->GetWindow(handle);
    if (!window)
      return;
  }
  // Get input panel by valid window handle.
  WaylandTextInput::InputPanel* panel = GetInputPanel(handle);

  if (!panel || !panel->model)
    panel = CreateInputPanel(handle);

  if (panel && panel->model) {
    panel->activated ? panel->Show() : panel->Activate();
  }
}

void WaylandTextInput::HideInputPanel(wl_seat* input_seat,
                                      unsigned handle,
                                      ui::ImeHiddenType hidden_type) {
  WaylandTextInput::InputPanel* panel = GetInputPanel(handle);

  if (!panel || !panel->model)
    return;

  hidden_type == ui::ImeHiddenType::kDeactivate ? panel->Deactivate()
                                                : panel->Hide();
  panel->SetHiddenState();
}

void WaylandTextInput::SetTextInputInfo(
    const ui::TextInputInfo& text_input_info,
    unsigned handle) {
  WaylandTextInput::InputPanel* panel = GetInputPanel(handle);

  if (panel) {
    panel->input_content_type = text_input_info.type;
    panel->text_input_flags = text_input_info.flags;
    panel->max_text_length = text_input_info.max_length;
    panel->input_panel_rect = text_input_info.input_panel_rectangle;

    if (panel->model) {
      // Set the input panel rectangle
      if (panel->input_panel_rect.IsEmpty())
        text_model_reset_input_panel_rect(panel->model);
      else
        text_model_set_input_panel_rect(
            panel->model, panel->input_panel_rect.x(),
            panel->input_panel_rect.y(), panel->input_panel_rect.width(),
            panel->input_panel_rect.height());

      // Set the content type
      text_model_set_content_type(
          panel->model,
          ContentHintFromInputContentType(panel->input_content_type,
                                          panel->text_input_flags),
          ContentPurposeFromInputContentType(panel->input_content_type));

      // Set maximum text length (if it was previously set only)
      if (panel->max_text_length >= 0)
        text_model_set_max_text_length(panel->model, panel->max_text_length);
    }
  }
}

void WaylandTextInput::SetSurroundingText(unsigned handle,
                                          const std::string& text,
                                          size_t cursor_position,
                                          size_t anchor_position) {
  WaylandTextInput::InputPanel* panel = GetInputPanel(handle);
  if (panel && panel->model)
    text_model_set_surrounding_text(panel->model, text.c_str(), cursor_position,
                                    anchor_position);
}

WaylandTextInput::InputPanel::InputPanel() = default;

WaylandTextInput::InputPanel::InputPanel(text_model* t_model,
                                         WaylandSeat* w_seat,
                                         unsigned window_handle)
    : model(t_model), seat(w_seat), associative_window_handle(window_handle) {
  static const text_model_listener text_model_listener = {
      WaylandTextInput::OnCommitString,
      WaylandTextInput::OnPreeditString,
      WaylandTextInput::OnDeleteSurroundingText,
      WaylandTextInput::OnCursorPosition,
      WaylandTextInput::OnPreeditStyling,
      WaylandTextInput::OnPreeditCursor,
      WaylandTextInput::OnModifiersMap,
      WaylandTextInput::OnKeysym,
      WaylandTextInput::OnEnter,
      WaylandTextInput::OnLeave,
      WaylandTextInput::OnInputPanelState,
      WaylandTextInput::OnTextModelInputPanelRect};

  if (model)
    text_model_add_listener(model, &text_model_listener, this);
}

WaylandTextInput::InputPanel::~InputPanel() = default;

void WaylandTextInput::InputPanel::SetHiddenState() {
  input_panel_rect.SetRect(0, 0, 0, 0);
  if (associative_window_handle) {
    WaylandDisplay::GetInstance()->InputPanelRectChanged(
        associative_window_handle, 0, 0, 0, 0);
    WaylandDisplay::GetInstance()->InputPanelStateChanged(
        associative_window_handle, webos::InputPanelState::INPUT_PANEL_HIDDEN);
  }
}

void WaylandTextInput::InputPanel::Activate() {
  WaylandWindow* window = nullptr;
  if (associative_window_handle)
    window =
        WaylandDisplay::GetInstance()->GetWindow(associative_window_handle);

  if (!window)
    return;

  if (model)
    text_model_activate(model, serial, seat->GetWLSeat(),
                        window->ShellSurface()->GetWLSurface());
}

void WaylandTextInput::InputPanel::Deactivate() {
  if (model && seat) {
    text_model_reset(model, serial);
    text_model_deactivate(model, seat->GetWLSeat());
    text_model_destroy(model);
    model = nullptr;
    activated = false;
  }
}

void WaylandTextInput::InputPanel::Show() {
  if (model)
    text_model_show_input_panel(model);
}

void WaylandTextInput::InputPanel::Hide() {
  if (model)
    text_model_hide_input_panel(model);
}

void WaylandTextInput::OnWindowAboutToDestroy(unsigned windowhandle) {
  WaylandTextInput::InputPanel* panel = GetInputPanel(windowhandle);
  if (panel) {
    panel->Deactivate();
    input_panel_map_[windowhandle].reset(nullptr);
  }
}

void WaylandTextInput::OnCommitString(void* data,
                                      struct text_model* text_input,
                                      uint32_t serial,
                                      const char* text) {
  WaylandDisplay* dispatcher = WaylandDisplay::GetInstance();
  WaylandTextInput::InputPanel* panel =
      static_cast<WaylandTextInput::InputPanel*>(data);
  if (panel->associative_window_handle)
    dispatcher->Commit(panel->associative_window_handle, std::string(text));
}

void WaylandTextInput::OnPreeditString(void* data,
                                       struct text_model* text_input,
                                       uint32_t serial,
                                       const char* text,
                                       const char* commit) {
  WaylandDisplay* dispatcher = WaylandDisplay::GetInstance();
  WaylandTextInput::InputPanel* panel =
      static_cast<WaylandTextInput::InputPanel*>(data);
  if (panel->associative_window_handle)
    dispatcher->PreeditChanged(panel->associative_window_handle,
                               std::string(text), std::string(commit));
}

void WaylandTextInput::OnDeleteSurroundingText(void* data,
                                               struct text_model* text_input,
                                               uint32_t serial,
                                               int32_t index,
                                               uint32_t length) {
  WaylandDisplay* dispatcher = WaylandDisplay::GetInstance();
  WaylandTextInput::InputPanel* panel =
      static_cast<WaylandTextInput::InputPanel*>(data);
  if (panel->associative_window_handle)
    dispatcher->DeleteRange(panel->associative_window_handle, index, length);
}

void WaylandTextInput::OnCursorPosition(void* data,
                                        struct text_model* text_input,
                                        uint32_t serial,
                                        int32_t index,
                                        int32_t anchor) {}

void WaylandTextInput::OnPreeditStyling(void* data,
                                        struct text_model* text_input,
                                        uint32_t serial,
                                        uint32_t index,
                                        uint32_t length,
                                        uint32_t style) {}

void WaylandTextInput::OnPreeditCursor(void* data,
                                       struct text_model* text_input,
                                       uint32_t serial,
                                       int32_t index) {}

void WaylandTextInput::OnModifiersMap(void* data,
                                      struct text_model* text_input,
                                      struct wl_array* map) {}

void WaylandTextInput::OnKeysym(void* data,
                                struct text_model* text_input,
                                uint32_t serial,
                                uint32_t time,
                                uint32_t key,
                                uint32_t state,
                                uint32_t modifiers) {
  uint32_t key_code = KeyNumberFromKeySymCode(key, modifiers);
  if (key_code == KEY_UNKNOWN)
    return;

  WaylandTextInput::InputPanel* panel =
      static_cast<WaylandTextInput::InputPanel*>(data);

  // Copied from WaylandKeyboard::OnKeyNotify().

  ui::EventType type = ui::ET_KEY_PRESSED;
  WaylandDisplay* dispatcher = WaylandDisplay::GetInstance();

  dispatcher->SetSerial(serial);
  if (state == WL_KEYBOARD_KEY_STATE_RELEASED)
    type = ui::ET_KEY_RELEASED;
  const uint32_t device_id =
      wl_proxy_get_id(reinterpret_cast<wl_proxy*>(text_input));

  IMEModifierFlags flag = FLAG_ALT;
  while (flag) {
    dispatcher->TextInputModifier(
        state, GetModifierKey((IMEModifierFlags)(flag & modifiers)));
    flag = (IMEModifierFlags)(flag >> 1);
  }

  dispatcher->KeyNotify(type, key_code, device_id);

  bool hide_ime = false;

  if (key_code == KEY_PREVIOUS || key_code == KEY_UP || key_code == KEY_DOWN)
    if (panel->state == InputPanelHidden)
      hide_ime = true;

  if (state == WL_KEYBOARD_KEY_STATE_RELEASED &&
      (key_code == KEY_ENTER || key_code == KEY_KPENTER) &&
      (panel->input_content_type != ui::InputContentType::kTextArea) &&
      (panel->state == InputPanelShown))
    hide_ime = true;

  if (key_code == KEY_TAB)
    hide_ime = true;

  if (hide_ime && panel->associative_window_handle)
    dispatcher->PrimarySeat()->HideInputPanel(panel->associative_window_handle,
                                              ui::ImeHiddenType::kHide);
}

void WaylandTextInput::OnEnter(void* data,
                               struct text_model* text_input,
                               struct wl_surface* surface) {
  WaylandTextInput::InputPanel* panel =
      static_cast<WaylandTextInput::InputPanel*>(data);
  WaylandDisplay* dispatcher = WaylandDisplay::GetInstance();

  const uint32_t device_id =
      wl_proxy_get_id(reinterpret_cast<wl_proxy*>(text_input));

  panel->activated = true;

  WaylandWindow* window =
      static_cast<WaylandWindow*>(wl_surface_get_user_data(surface));
  if (window)
    dispatcher->InputPanelEnter(device_id, window->Handle());
}

void WaylandTextInput::OnLeave(void* data,
                               struct text_model* text_input) {
  WaylandTextInput::InputPanel* panel =
      static_cast<WaylandTextInput::InputPanel*>(data);
  WaylandDisplay* dispatcher = WaylandDisplay::GetInstance();

  const uint32_t device_id =
      wl_proxy_get_id(reinterpret_cast<wl_proxy*>(text_input));

  panel->Deactivate();

  dispatcher->InputPanelLeave(device_id);
}

void WaylandTextInput::OnInputPanelState(void* data,
                                         struct text_model* text_input,
                                         uint32_t state) {
  WaylandTextInput::InputPanel* panel =
      static_cast<WaylandTextInput::InputPanel*>(data);
  WaylandDisplay* dispatcher = WaylandDisplay::GetInstance();

  panel->state = static_cast<InputPanelState>(state);

  switch (state) {
    case InputPanelShown: {
      if (panel->associative_window_handle)
        dispatcher->InputPanelStateChanged(
            panel->associative_window_handle,
            webos::InputPanelState::INPUT_PANEL_SHOWN);
      break;
    }
    case InputPanelHidden:
      panel->SetHiddenState();
      break;
    default:
      break;
  }
}

void WaylandTextInput::OnTextModelInputPanelRect(void* data,
                                                 struct text_model* text_model,
                                                 int32_t x,
                                                 int32_t y,
                                                 uint32_t width,
                                                 uint32_t height) {
  WaylandTextInput::InputPanel* panel =
      static_cast<WaylandTextInput::InputPanel*>(data);
  WaylandDisplay* dispatcher = WaylandDisplay::GetInstance();

  gfx::Rect oldRect(panel->input_panel_rect);
  panel->input_panel_rect.SetRect(x, y, width, height);

  if (panel->input_panel_rect != oldRect && panel->associative_window_handle)
    dispatcher->InputPanelRectChanged(panel->associative_window_handle, x, y,
                                      width, height);
}

}  // namespace ozonewayland
