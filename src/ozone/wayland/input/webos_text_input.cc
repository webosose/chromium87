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
#include "ui/base/ime/text_input_flags.h"
#include "ui/events/keycodes/keyboard_code_conversion.h"
#include "ui/events/keycodes/xkb_keysym.h"
#include "ui/base/ime/text_input_flags.h"

namespace ozonewayland {

uint32_t ContentHintFromInputContentType(ui::InputContentType content_type,
                                         int input_flags) {
  uint32_t wl_hint = (TEXT_MODEL_CONTENT_HINT_AUTO_COMPLETION |
                      TEXT_MODEL_CONTENT_HINT_AUTO_CAPITALIZATION);
  if (content_type == ui::INPUT_CONTENT_TYPE_PASSWORD)
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
    case ui::INPUT_CONTENT_TYPE_PASSWORD:
      return TEXT_MODEL_CONTENT_PURPOSE_PASSWORD;
    case ui::INPUT_CONTENT_TYPE_EMAIL:
      return TEXT_MODEL_CONTENT_PURPOSE_EMAIL;
    case ui::INPUT_CONTENT_TYPE_NUMBER:
      return TEXT_MODEL_CONTENT_PURPOSE_NUMBER;
    case ui::INPUT_CONTENT_TYPE_TELEPHONE:
      return TEXT_MODEL_CONTENT_PURPOSE_PHONE;
    case ui::INPUT_CONTENT_TYPE_URL:
      return TEXT_MODEL_CONTENT_PURPOSE_URL;
    case ui::INPUT_CONTENT_TYPE_DATE:
      return TEXT_MODEL_CONTENT_PURPOSE_DATE;
    case ui::INPUT_CONTENT_TYPE_DATE_TIME:
    case ui::INPUT_CONTENT_TYPE_DATE_TIME_LOCAL:
      return TEXT_MODEL_CONTENT_PURPOSE_DATETIME;
    case ui::INPUT_CONTENT_TYPE_TIME:
      return TEXT_MODEL_CONTENT_PURPOSE_TIME;
    default:
      return TEXT_MODEL_CONTENT_PURPOSE_NORMAL;
  }
}

const struct text_model_listener text_model_listener_ = {
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
  WaylandTextInput::OnTextModelInputPanelRect
};

static uint32_t serial = 0;

WaylandTextInput::WaylandTextInput(WaylandSeat* seat) : seat_(seat) {}

WaylandTextInput::~WaylandTextInput() {
  for (auto& input_panel_item : input_panel_map_) {
    DeactivateInputPanel(input_panel_item.first);
  }
}

void WaylandTextInput::ResetIme(unsigned handle) {
  WaylandWindow* active_window = FindActiveWindow(handle);
  if (active_window) {
    std::string display_id = active_window->GetDisplayId();
    WaylandTextInput::InputPanel* panel = FindInputPanel(display_id);

    if (panel && panel->model) {
      text_model_reset(panel->model, serial);
    } else {
      panel = new InputPanel(CreateTextModel());
      input_panel_map_[display_id].reset(panel);
    }
  }
}

void WaylandTextInput::DeactivateInputPanel(const std::string& display_id) {
  WaylandTextInput::InputPanel* input_panel = FindInputPanel(display_id);
  if (input_panel && input_panel->model && input_panel->activated) {
    SetHiddenState(display_id);
    text_model_reset(input_panel->model, serial);
    text_model_deactivate(input_panel->model, seat_->GetWLSeat());
    text_model_destroy(input_panel->model);
    input_panel->model = nullptr;
    input_panel->activated = false;
  }
}

text_model* WaylandTextInput::CreateTextModel() {
  text_model* model = nullptr;
  text_model_factory* factory =
      WaylandDisplay::GetInstance()->GetTextModelFactory();
  if (factory) {
    model = text_model_factory_create_text_model(factory);
    if (model)
      text_model_add_listener(model, &text_model_listener_, this);
  }
  return model;
}

WaylandWindow* WaylandTextInput::FindActiveWindow(unsigned handle) {
  for (auto& active_window_item : active_window_map_) {
    if (active_window_item.second &&
        active_window_item.second->Handle() == handle) {
      return active_window_item.second;
    }
  }
  return nullptr;
}

WaylandTextInput::InputPanel* WaylandTextInput::FindInputPanel(
    const std::string& display_id) {
  if (input_panel_map_.find(display_id) != input_panel_map_.end()) {
    return input_panel_map_[display_id].get();
  }
  return nullptr;
}

std::string WaylandTextInput::FindDisplay(text_model* model) {
  for (auto& input_panel_item : input_panel_map_) {
    if (input_panel_item.second && input_panel_item.second->model == model) {
      return input_panel_item.first;
    }
  }
  return std::string();
}

void WaylandTextInput::ShowInputPanel(wl_seat* input_seat, unsigned handle) {
  WaylandWindow* active_window = FindActiveWindow(handle);

  if (active_window) {
    std::string display_id = active_window->GetDisplayId();
    WaylandTextInput::InputPanel* panel = FindInputPanel(display_id);

    if (!panel || !panel->model) {
      panel = new InputPanel(CreateTextModel());
      input_panel_map_[display_id].reset(panel);
    }

    if (panel && panel->model) {
      if (panel->activated) {
        if (panel->state != InputPanelShown)
          text_model_show_input_panel(panel->model);
      } else
        text_model_activate(panel->model, serial, seat_->GetWLSeat(),
                            active_window->ShellSurface()->GetWLSurface());
      text_model_set_content_type(
          panel->model,
          ContentHintFromInputContentType(panel->input_content_type,
                                          panel->text_input_flags),
          ContentPurposeFromInputContentType(panel->input_content_type));
    }
  }
}

void WaylandTextInput::HideInputPanel(wl_seat* input_seat,
                                      const std::string& display_id,
                                      ui::ImeHiddenType hidden_type) {
  WaylandTextInput::InputPanel* panel = FindInputPanel(display_id);

  if (!panel || !panel->model)
    return;

  if (hidden_type == ui::ImeHiddenType::kDeactivate) {
    DeactivateInputPanel(display_id);
  } else {
    SetHiddenState(display_id);
    text_model_hide_input_panel(panel->model);
  }
}

void WaylandTextInput::SetActiveWindow(const std::string& display_id,
                                       WaylandWindow* window) {
  active_window_map_[display_id] = window;
}

WaylandWindow* WaylandTextInput::GetActiveWindow(
    const std::string& display_id) const {
  if (active_window_map_.find(display_id) != active_window_map_.end()) {
    return active_window_map_.at(display_id);
  }
  return nullptr;
}

void WaylandTextInput::SetHiddenState(const std::string& display_id) {
  WaylandTextInput::InputPanel* panel = FindInputPanel(display_id);
  if (panel)
    panel->input_panel_rect.SetRect(0, 0, 0, 0);
  WaylandWindow* active_window = GetActiveWindow(display_id);
  if (active_window) {
    WaylandDisplay::GetInstance()->InputPanelRectChanged(
        active_window->Handle(), 0, 0, 0, 0);
    WaylandDisplay::GetInstance()->InputPanelStateChanged(
        active_window->Handle(), webos::InputPanelState::INPUT_PANEL_HIDDEN);
  }
}

void WaylandTextInput::SetInputContentType(ui::InputContentType content_type,
                                           int text_input_flags,
                                           unsigned handle) {
  WaylandWindow* active_window = FindActiveWindow(handle);
  if (active_window) {
    std::string display_id = active_window->GetDisplayId();
    WaylandTextInput::InputPanel* panel = FindInputPanel(display_id);

    if (panel) {
      panel->input_content_type = content_type;
      panel->text_input_flags = text_input_flags;
      if (panel->model)
        text_model_set_content_type(
            panel->model,
            ContentHintFromInputContentType(panel->input_content_type,
                                            panel->text_input_flags),
            ContentPurposeFromInputContentType(panel->input_content_type));
    }
  }
}

void WaylandTextInput::SetSurroundingText(unsigned handle,
                                          const std::string& text,
                                          size_t cursor_position,
                                          size_t anchor_position) {
  WaylandWindow* active_window = FindActiveWindow(handle);

  if (active_window) {
    std::string display_id = active_window->GetDisplayId();
    WaylandTextInput::InputPanel* panel = FindInputPanel(display_id);
    if (panel && panel->model)
      text_model_set_surrounding_text(panel->model, text.c_str(),
                                      cursor_position, anchor_position);
  }
}

WaylandTextInput::InputPanel::InputPanel() = default;

WaylandTextInput::InputPanel::InputPanel(text_model* t_model)
    : model(t_model) {}

WaylandTextInput::InputPanel::~InputPanel() = default;

void WaylandTextInput::OnWindowAboutToDestroy(unsigned windowhandle) {
  WaylandWindow* active_window = FindActiveWindow(windowhandle);

  if (active_window)
    active_window_map_[active_window->GetDisplayId()] = nullptr;
}

void WaylandTextInput::OnCommitString(void* data,
                                      struct text_model* text_input,
                                      uint32_t serial,
                                      const char* text) {
  WaylandDisplay* dispatcher = WaylandDisplay::GetInstance();
  WaylandTextInput* instance = static_cast<WaylandTextInput*>(data);
  std::string display = instance->FindDisplay(text_input);
  if (!display.empty()) {
    WaylandWindow* active_window = instance->GetActiveWindow(display);
    if (active_window)
      dispatcher->Commit(active_window->Handle(), std::string(text));
  }
}

void WaylandTextInput::OnPreeditString(void* data,
                                       struct text_model* text_input,
                                       uint32_t serial,
                                       const char* text,
                                       const char* commit) {
  WaylandDisplay* dispatcher = WaylandDisplay::GetInstance();
  WaylandTextInput* instance = static_cast<WaylandTextInput*>(data);
  std::string display = instance->FindDisplay(text_input);
  if (!display.empty()) {
    WaylandWindow* active_window = instance->GetActiveWindow(display);
    if (active_window)
      dispatcher->PreeditChanged(active_window->Handle(), std::string(text),
                                 std::string(commit));
  }
}

void WaylandTextInput::OnDeleteSurroundingText(void* data,
                                               struct text_model* text_input,
                                               uint32_t serial,
                                               int32_t index,
                                               uint32_t length) {
  WaylandDisplay* dispatcher = WaylandDisplay::GetInstance();
  WaylandTextInput* instance = static_cast<WaylandTextInput*>(data);
  std::string display = instance->FindDisplay(text_input);
  if (!display.empty()) {
    WaylandWindow* active_window = instance->GetActiveWindow(display);
    if (active_window)
      dispatcher->DeleteRange(active_window->Handle(), index, length);
  }
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

  WaylandTextInput* wl_text_input = static_cast<WaylandTextInput*>(data);
  std::string display = wl_text_input->FindDisplay(text_input);

  if (!display.empty()) {
    WaylandTextInput::InputPanel* panel =
        wl_text_input->FindInputPanel(display);

    if (panel) {
      bool hide_ime = false;

      if (key_code == KEY_PREVIOUS || key_code == KEY_UP ||
          key_code == KEY_DOWN)
        if (panel->state == InputPanelHidden)
          hide_ime = true;

      if (state == WL_KEYBOARD_KEY_STATE_RELEASED &&
          (key_code == KEY_ENTER || key_code == KEY_KPENTER) &&
          (panel->input_content_type !=
           ui::InputContentType::INPUT_CONTENT_TYPE_TEXT_AREA) &&
          (panel->state == InputPanelShown))
        hide_ime = true;

      if (key_code == KEY_TAB)
        hide_ime = true;

      if (hide_ime) {
        dispatcher->PrimarySeat()->HideInputPanel(ui::ImeHiddenType::kHide,
                                                  display);
      }
    }
  }
}

void WaylandTextInput::OnEnter(void* data,
                               struct text_model* text_input,
                               struct wl_surface* surface) {
  WaylandTextInput* instance = static_cast<WaylandTextInput*>(data);
  WaylandDisplay* dispatcher = WaylandDisplay::GetInstance();
  std::string display = instance->FindDisplay(text_input);

  const uint32_t device_id =
      wl_proxy_get_id(reinterpret_cast<wl_proxy*>(text_input));

  if (!display.empty()) {
    WaylandTextInput::InputPanel* panel = instance->FindInputPanel(display);
    if (panel)
      panel->activated = true;
  }

  WaylandWindow* window =
      static_cast<WaylandWindow*>(wl_surface_get_user_data(surface));
  if (window)
    dispatcher->InputPanelEnter(device_id, window->Handle());
}

void WaylandTextInput::OnLeave(void* data,
                               struct text_model* text_input) {
  WaylandTextInput* instance = static_cast<WaylandTextInput*>(data);
  WaylandDisplay* dispatcher = WaylandDisplay::GetInstance();
  std::string display = instance->FindDisplay(text_input);

  const uint32_t device_id =
      wl_proxy_get_id(reinterpret_cast<wl_proxy*>(text_input));

  if (!display.empty())
    instance->DeactivateInputPanel(display);

  dispatcher->InputPanelLeave(device_id);
}

void WaylandTextInput::OnInputPanelState(void* data,
                                         struct text_model* text_input,
                                         uint32_t state) {
  WaylandTextInput* instance = static_cast<WaylandTextInput*>(data);
  WaylandDisplay* dispatcher = WaylandDisplay::GetInstance();

  std::string display = instance->FindDisplay(text_input);

  if (!display.empty()) {
    WaylandTextInput::InputPanel* panel = instance->FindInputPanel(display);

    if (panel)
      panel->state = static_cast<InputPanelState>(state);

    switch (state) {
      case InputPanelShown: {
        WaylandWindow* active_window = instance->GetActiveWindow(display);
        if (active_window)
          dispatcher->InputPanelStateChanged(
              active_window->Handle(),
              webos::InputPanelState::INPUT_PANEL_SHOWN);
        break;
      }
      case InputPanelHidden:
        instance->SetHiddenState(display);
        break;
      default:
        break;
    }
  }
}

void WaylandTextInput::OnTextModelInputPanelRect(void* data,
                                                 struct text_model* text_model,
                                                 int32_t x,
                                                 int32_t y,
                                                 uint32_t width,
                                                 uint32_t height) {
  WaylandTextInput* instance = static_cast<WaylandTextInput*>(data);
  WaylandDisplay* dispatcher = WaylandDisplay::GetInstance();
  std::string display = instance->FindDisplay(text_model);
  if (!display.empty()) {
    WaylandTextInput::InputPanel* panel = instance->FindInputPanel(display);

    if (panel) {
      gfx::Rect oldRect(panel->input_panel_rect);
      panel->input_panel_rect.SetRect(x, y, width, height);

      WaylandWindow* active_window = instance->GetActiveWindow(display);

      if (active_window && panel->input_panel_rect != oldRect)
        dispatcher->InputPanelRectChanged(active_window->Handle(), x, y, width,
                                          height);
    }
  }
}

}  // namespace ozonewayland
