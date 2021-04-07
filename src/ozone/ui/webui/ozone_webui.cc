// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ozone/ui/webui/ozone_webui.h"

#include <set>

#include "base/command_line.h"
#include "base/debug/leak_annotations.h"
#include "base/environment.h"
#include "base/i18n/rtl.h"
#include "base/logging.h"
#include "base/nix/mime_util_xdg.h"
#include "base/stl_util.h"
#include "base/strings/stringprintf.h"
#include "ozone/platform/ozone_gpu_platform_support_host.h"
#include "ozone/platform/ozone_platform_wayland.h"
#include "ozone/ui/webui/input_method_context_impl_wayland.h"
#include "ozone/ui/webui/input_method_context_manager.h"
#include "ozone/ui/webui/select_file_dialog_impl_webui.h"
#include "ui/views/linux_ui/nav_button_provider.h"

namespace views {

OzoneWebUI::OzoneWebUI() : host_(NULL) {
}

OzoneWebUI::~OzoneWebUI() {
}

void OzoneWebUI::Initialize() {
  // TODO(kalyan): This is a hack, get rid  of this.
  ui::OzonePlatform* platform = ui::OzonePlatform::GetInstance();
  host_ = static_cast<ui::OzoneGpuPlatformSupportHost*>(
      platform->GetGpuPlatformSupportHost());
  input_method_context_manager_.reset(new ui::InputMethodContextManager(host_));
}

ui::SelectFileDialog* OzoneWebUI::CreateSelectFileDialog(
    ui::SelectFileDialog::Listener* listener,
    std::unique_ptr<ui::SelectFilePolicy> policy) const {
#if defined(USE_SELECT_FILE_DIALOG_WEBUI_IMPL)
  return ui::SelectFileDialogImplWebUI::Create(listener, policy.get());
#endif
  NOTIMPLEMENTED();
  return nullptr;
}

std::unique_ptr<ui::LinuxInputMethodContext> OzoneWebUI::CreateInputMethodContext(
      ui::LinuxInputMethodContextDelegate* delegate, bool is_simple) const {
  // Fake handle for a context which will not use handle id.
  unsigned fake_handle = 0;
  return CreateInputMethodContext(delegate, fake_handle, is_simple);
}

std::unique_ptr<ui::LinuxInputMethodContext> OzoneWebUI::CreateInputMethodContext(
      ui::LinuxInputMethodContextDelegate* delegate, unsigned handle, bool is_simple) const {
  DCHECK(input_method_context_manager_);
  return std::unique_ptr<ui::LinuxInputMethodContext>(
           new ui::InputMethodContextImplWayland(delegate,
                                                 handle,
                                                 input_method_context_manager_.get()));
}

gfx::FontRenderParams OzoneWebUI::GetDefaultFontRenderParams() const {
  NOTIMPLEMENTED_LOG_ONCE();
  return params_;
}

void OzoneWebUI::GetDefaultFontDescription(
    std::string* family_out,
    int* size_pixels_out,
    int* style_out,
    gfx::Font::Weight* weight_out,
    gfx::FontRenderParams* params_out) const {
  NOTIMPLEMENTED();
}

bool OzoneWebUI::GetColor(int id, SkColor* color, bool use_custom_frame) const {
  return false;
}

bool OzoneWebUI::GetDisplayProperty(int id, int* result) const {
  return false;
}

SkColor OzoneWebUI::GetFocusRingColor() const {
  return SkColorSetRGB(0x4D, 0x90, 0xFE);
}

SkColor OzoneWebUI::GetActiveSelectionBgColor() const {
  return SkColorSetRGB(0xCB, 0xE4, 0xFA);
}

SkColor OzoneWebUI::GetActiveSelectionFgColor() const {
  return SK_ColorBLACK;
}

SkColor OzoneWebUI::GetInactiveSelectionBgColor() const {
  return SkColorSetRGB(0xEA, 0xEA, 0xEA);
}

SkColor OzoneWebUI::GetInactiveSelectionFgColor() const {
  return SK_ColorBLACK;
}

base::TimeDelta OzoneWebUI::GetCursorBlinkInterval() const {
  static const double kCursorBlinkTime = 1.0;
  return base::TimeDelta::FromSecondsD(kCursorBlinkTime);
}

ui::NativeTheme* OzoneWebUI::GetNativeTheme(aura::Window* window) const {
  return 0;
}

void OzoneWebUI::SetUseSystemThemeCallback(UseSystemThemeCallback callback) {}

bool OzoneWebUI::GetDefaultUsesSystemTheme() const {
  return false;
}

gfx::Image OzoneWebUI::GetIconForContentType(
  const std::string& content_type, int size) const {
  return gfx::Image();
}

std::unique_ptr<Border> OzoneWebUI::CreateNativeBorder(
  views::LabelButton* owning_button,
  std::unique_ptr<views::LabelButtonBorder> border) {
  return std::move(border);
}

void OzoneWebUI::AddWindowButtonOrderObserver(
  WindowButtonOrderObserver* observer) {
}

void OzoneWebUI::RemoveWindowButtonOrderObserver(
  WindowButtonOrderObserver* observer) {
}

LinuxUI::WindowFrameAction OzoneWebUI::GetWindowFrameAction(
    WindowFrameActionSource source) {
  return LinuxUI::WindowFrameAction::kNone;
}

void OzoneWebUI::NotifyWindowManagerStartupComplete() {
}

bool OzoneWebUI::MatchEvent(const ui::Event& event,
  std::vector<TextEditCommandAuraLinux>* commands) {
  return false;
}

void OzoneWebUI::UpdateDeviceScaleFactor() {
}

float OzoneWebUI::GetDeviceScaleFactor() const {
  return 0.0;
}

bool OzoneWebUI::GetTint(int id, color_utils::HSL* tint) const {
  return true;
}

void OzoneWebUI::AddDeviceScaleFactorObserver(DeviceScaleFactorObserver* observer) {
}

void OzoneWebUI::RemoveDeviceScaleFactorObserver(DeviceScaleFactorObserver* observer) {
}

bool OzoneWebUI::PreferDarkTheme() const {
  return false;
}

std::unique_ptr<NavButtonProvider> OzoneWebUI::CreateNavButtonProvider() {
  return nullptr;
}

base::flat_map<std::string, std::string>
OzoneWebUI::GetKeyboardLayoutMap() {
  return {};
}

std::string OzoneWebUI::GetCursorThemeName() {
  return {};
}

int OzoneWebUI::GetCursorThemeSize() {
  return 0;
}

bool OzoneWebUI::AnimationsEnabled() const {
  return true;
}

}  // namespace views

views::LinuxUI* BuildWebUI() {
  return new views::OzoneWebUI();
}
