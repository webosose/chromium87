// Copyright 2017-2018 LG Electronics, Inc.
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

#ifndef OZONE_WAYLAND_INPUT_WEBOS_TEXT_INPUT_H_
#define OZONE_WAYLAND_INPUT_WEBOS_TEXT_INPUT_H_

#include <map>
#include "wayland-text-client-protocol.h"

#include "ozone/platform/input_content_type.h"
#include "ozone/wayland/display.h"

struct text_model;

namespace base {
template <typename Type>
struct DefaultSingletonTraits;
}

namespace ozonewayland {

class WaylandWindow;
class WaylandSeat;

class WaylandTextInput {
 public:
  explicit WaylandTextInput(WaylandSeat* seat);
  ~WaylandTextInput();

  void ResetIme(unsigned handle);
  void ShowInputPanel(wl_seat* input_seat, unsigned handle);
  void HideInputPanel(wl_seat* input_seat,
                      const std::string& display_id,
                      ui::ImeHiddenType);
  void SetActiveWindow(const std::string& display_id, WaylandWindow* window);
  WaylandWindow* GetActiveWindow(const std::string& display_id) const;
  void SetInputContentType(ui::InputContentType content_type,
                           int text_input_flags,
                           unsigned handle);
  void SetSurroundingText(unsigned handle,
                          const std::string& text,
                          size_t cursor_position,
                          size_t anchor_position);

  void OnWindowAboutToDestroy(unsigned windowhandle);

  static void OnCommitString(void* data,
                             struct text_model* text_input,
                             uint32_t serial,
                             const char* text);

  static void OnPreeditString(void* data,
                              struct text_model* text_input,
                              uint32_t serial,
                              const char* text,
                              const char* commit);

  static void OnDeleteSurroundingText(void* data,
                                      struct text_model* text_input,
                                      uint32_t serial,
                                      int32_t index,
                                      uint32_t length);

  static void OnCursorPosition(void* data,
                               struct text_model* text_input,
                               uint32_t serial,
                               int32_t index,
                               int32_t anchor);

  static void OnPreeditStyling(void* data,
                               struct text_model* text_input,
                               uint32_t serial,
                               uint32_t index,
                               uint32_t length,
                               uint32_t style);

  static void OnPreeditCursor(void* data,
                              struct text_model* text_input,
                              uint32_t serial,
                              int32_t index);

  static void OnModifiersMap(void* data,
                             struct text_model* text_input,
                             struct wl_array* map);

  static void OnKeysym(void* data,
                       struct text_model* text_input,
                       uint32_t serial,
                       uint32_t time,
                       uint32_t key,
                       uint32_t state,
                       uint32_t modifiers);

  static void OnEnter(void* data,
                      struct text_model* text_input,
                      struct wl_surface* surface);

  static void OnLeave(void* data,
                      struct text_model* text_input);

  static void OnInputPanelState(void* data,
                      struct text_model* text_input,
                      uint32_t state);

  static void OnTextModelInputPanelRect(void *data,
                      struct text_model *text_input,
                      int32_t x,
                      int32_t y,
                      uint32_t width,
                      uint32_t height);

 private:
  enum InputPanelState {
    InputPanelUnknownState = 0xffffffff,
    InputPanelHidden = 0,
    InputPanelShown = 1,
    InputPanelShowing = 2
  };

  struct InputPanel {
    InputPanel();
    InputPanel(text_model* t_model);
    ~InputPanel();

    text_model* model = nullptr;
    gfx::Rect input_panel_rect = gfx::Rect(0, 0, 0, 0);
    bool activated = false;
    InputPanelState state = InputPanelUnknownState;
    ui::InputContentType input_content_type = ui::INPUT_CONTENT_TYPE_NONE;
    int text_input_flags = 0;
  };

  struct InputPanelDeleter {
    void operator()(InputPanel* panel) {
      if (panel->model)
        text_model_destroy(panel->model);
      delete panel;
    }
  };

  using InputPanelPtr = std::unique_ptr<InputPanel, InputPanelDeleter>;

  text_model* CreateTextModel();
  WaylandWindow* FindActiveWindow(unsigned handle);
  InputPanel* FindInputPanel(const std::string& display_id);
  std::string FindDisplay(text_model* model);
  void DeactivateInputPanel(const std::string& display_id);
  void SetHiddenState(const std::string& display_id);

  friend struct base::DefaultSingletonTraits<WaylandTextInput>;

  std::map<std::string, InputPanelPtr> input_panel_map_;
  std::map<std::string, WaylandWindow*> active_window_map_;
  WaylandSeat* seat_;

  DISALLOW_COPY_AND_ASSIGN(WaylandTextInput);
};

}  // namespace ozonewayland

#endif  // OZONE_WAYLAND_INPUT_WEBOS_TEXT_INPUT_H_
