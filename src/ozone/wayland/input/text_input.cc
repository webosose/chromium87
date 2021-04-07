// Copyright 2014 Intel Corporation. All rights reserved.
// Copyright 2016-2019 LG Electronics, Inc.
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

#include "ozone/wayland/input/text_input.h"

#include <string>

#include "ozone/wayland/display.h"
#include "ozone/wayland/input/keyboard.h"
#include "ozone/wayland/input/text_input_utils.h"
#include "ozone/wayland/protocol/text-client-protocol.h"
#include "ozone/wayland/seat.h"
#include "ozone/wayland/shell/shell_surface.h"
#include "ozone/wayland/window.h"
#include "ui/base/ui_base_neva_switches.h"

namespace ozonewayland {

WaylandTextInput::WaylandTextInput(WaylandSeat* seat)
    : text_input_(NULL),
      active_window_(NULL),
      last_active_window_(NULL),
      seat_(seat) {
  enable_vkb_support_ = getenv("USE_OZONE_WAYLAND_VKB") ||
                        base::CommandLine::ForCurrentProcess()->HasSwitch(
                            switches::kUseOzoneWaylandVkb);
}

WaylandTextInput::~WaylandTextInput() {
  if (text_input_)
    wl_text_input_destroy(text_input_);
}

void WaylandTextInput::SetActiveWindow(WaylandWindow* window) {
  active_window_ = window;
  if (active_window_)
    last_active_window_ = active_window_;
}

void WaylandTextInput::OnCommitString(void* data,
                                      struct wl_text_input* text_input,
                                      uint32_t serial,
                                      const char* text) {
  WaylandDisplay* dispatcher = WaylandDisplay::GetInstance();
  DCHECK(static_cast<WaylandTextInput*>(data)->last_active_window_);
  dispatcher->Commit(static_cast<WaylandTextInput*>(data)->
      last_active_window_->Handle(), std::string(text));
}

void WaylandTextInput::OnPreeditString(void* data,
                                       struct wl_text_input* text_input,
                                       uint32_t serial,
                                       const char* text,
                                       const char* commit) {
  WaylandDisplay* dispatcher = WaylandDisplay::GetInstance();
  DCHECK(static_cast<WaylandTextInput*>(data)->last_active_window_);
  dispatcher->PreeditChanged(static_cast<WaylandTextInput*>(data)->
     last_active_window_->Handle(), std::string(text), std::string(commit));
}

void WaylandTextInput::OnDeleteSurroundingText(void* data,
                                       struct wl_text_input* text_input,
                                       int32_t index,
                                       uint32_t length) {
  WaylandDisplay* dispatcher = WaylandDisplay::GetInstance();

  DCHECK(static_cast<WaylandTextInput*>(data)->last_active_window_);
  dispatcher->DeleteRange(
      static_cast<WaylandTextInput*>(data)->last_active_window_->Handle(),
      index, length);
}

void WaylandTextInput::OnCursorPosition(void* data,
                                       struct wl_text_input* text_input,
                                       int32_t index,
                                       int32_t anchor) {
}

void WaylandTextInput::OnPreeditStyling(void* data,
                                       struct wl_text_input* text_input,
                                       uint32_t index,
                                       uint32_t length,
                                       uint32_t style) {
}

void WaylandTextInput::OnPreeditCursor(void* data,
                                       struct wl_text_input* text_input,
                                       int32_t index) {
}

void WaylandTextInput::OnModifiersMap(void* data,
                                      struct wl_text_input* text_input,
                                      struct wl_array* map) {
}

void WaylandTextInput::OnKeysym(void* data,
                                struct wl_text_input* text_input,
                                uint32_t serial,
                                uint32_t time,
                                uint32_t key,
                                uint32_t state,
                                uint32_t modifiers) {
  // Copied from WaylandKeyboard::OnKeyNotify().
  ui::EventType type = ui::ET_KEY_PRESSED;
  WaylandDisplay::GetInstance()->SetSerial(serial);
  if (state == WL_KEYBOARD_KEY_STATE_RELEASED)
    type = ui::ET_KEY_RELEASED;
  WaylandDisplay* dispatcher = WaylandDisplay::GetInstance();
  const uint32_t device_id =
      wl_proxy_get_id(reinterpret_cast<wl_proxy*>(text_input));
  dispatcher->VirtualKeyNotify(type, KeyNumberFromKeySymCode(key, modifiers),
                               device_id);
}

void WaylandTextInput::OnEnter(void* data,
                               struct wl_text_input* text_input,
                               struct wl_surface* surface) {
  WaylandTextInput* instance = static_cast<WaylandTextInput*>(data);
  WaylandDisplay* dispatcher = WaylandDisplay::GetInstance();

  if (instance->last_active_window_)
    dispatcher->InputPanelVisibilityChanged(instance->last_active_window_->Handle(), true);
}

void WaylandTextInput::OnLeave(void* data,
                               struct wl_text_input* text_input) {
  WaylandTextInput* instance = static_cast<WaylandTextInput*>(data);
  WaylandDisplay* dispatcher = WaylandDisplay::GetInstance();

  if (instance->last_active_window_)
    dispatcher->InputPanelVisibilityChanged(instance->last_active_window_->Handle(), false);
}

void WaylandTextInput::OnInputPanelState(void* data,
                               struct wl_text_input* text_input,
                               uint32_t state) {
}

void WaylandTextInput::OnLanguage(void* data,
                               struct wl_text_input* text_input,
                               uint32_t serial,
                               const char* language) {
}

void WaylandTextInput::OnTextDirection(void* data,
                               struct wl_text_input* text_input,
                               uint32_t serial,
                               uint32_t direction) {
}

void WaylandTextInput::SetSurroundingText(const std::string& text,
                                          uint32_t cursor,
                                          uint32_t anchor) {
  if (text_input_)
    wl_text_input_set_surrounding_text(text_input_, text.c_str(), cursor, anchor);
}

void WaylandTextInput::ResetIme() {
  static const struct wl_text_input_listener text_input_listener = {
      WaylandTextInput::OnEnter,
      WaylandTextInput::OnLeave,
      WaylandTextInput::OnModifiersMap,
      WaylandTextInput::OnInputPanelState,
      WaylandTextInput::OnPreeditString,
      WaylandTextInput::OnPreeditStyling,
      WaylandTextInput::OnPreeditCursor,
      WaylandTextInput::OnCommitString,
      WaylandTextInput::OnCursorPosition,
      WaylandTextInput::OnDeleteSurroundingText,
      WaylandTextInput::OnKeysym,
      WaylandTextInput::OnLanguage,
      WaylandTextInput::OnTextDirection
  };

  if (!text_input_ && enable_vkb_support_) {
    text_input_ = wl_text_input_manager_create_text_input(
        WaylandDisplay::GetInstance()->GetTextInputManager());
    wl_text_input_add_listener(text_input_, &text_input_listener, this);
  }
}

void WaylandTextInput::ShowInputPanel(wl_seat* input_seat, unsigned handle) {
  if (!text_input_ && enable_vkb_support_)
    ResetIme();
  if (text_input_ && active_window_ && active_window_->ShellSurface()) {
    wl_text_input_show_input_panel(text_input_);
    wl_text_input_activate(text_input_,
                           input_seat,
                           active_window_->ShellSurface()->GetWLSurface());
  }
}

void WaylandTextInput::HideInputPanel(wl_seat* input_seat) {
  if (text_input_)
    wl_text_input_deactivate(text_input_, input_seat);
}

}  // namespace ozonewayland
