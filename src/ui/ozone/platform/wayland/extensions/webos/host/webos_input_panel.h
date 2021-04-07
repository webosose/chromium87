// Copyright 2020 LG Electronics, Inc.
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
//

#ifndef UI_OZONE_PLATFORM_WAYLAND_EXTENSIONS_WEBOS_HOST_WEBOS_INPUT_PANEL_H_
#define UI_OZONE_PLATFORM_WAYLAND_EXTENSIONS_WEBOS_HOST_WEBOS_INPUT_PANEL_H_

#include <memory>

#include "ui/ozone/platform/wayland/extensions/webos/host/webos_text_model_wrapper.h"
#include "ui/ozone/platform/wayland/host/input_panel.h"

namespace ui {

class WaylandConnection;
class WaylandWindow;

// Representation of the text input panel associated with an active window.
class WebosInputPanel : public InputPanel {
 public:
  WebosInputPanel(WaylandConnection* connection, WaylandWindow* window);
  WebosInputPanel(const WebosInputPanel&) = delete;
  WebosInputPanel& operator=(const WebosInputPanel&) = delete;
  ~WebosInputPanel() override;

  void HideInputPanel() override;
  void SetInputContentType(TextInputType type, int flags) override;
  void SetSurroundingText(const std::string& text,
                          std::size_t cursor_position,
                          std::size_t anchor_position) override;
  void ShowInputPanel() override;

 private:
  // Creates the text model wrapper (if not exists).
  bool CreateTextModel();
  // Deactivates and destroys the text model wrapper (is exists).
  void Deactivate();

  WaylandConnection* const connection_;
  WaylandWindow* const associated_window_;

  std::unique_ptr<WebosTextModelWrapper> webos_text_model_;

  // Text input attributes affecting the virtual keyboard appearance.
  TextInputType text_input_type_ = TEXT_INPUT_TYPE_NONE;
  int text_input_flags_ = 0;
};

}  // namespace ui

#endif  // UI_OZONE_PLATFORM_WAYLAND_EXTENSIONS_WEBOS_HOST_WEBOS_INPUT_PANEL_H_
