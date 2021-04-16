// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_WINDOW_TREE_HOST_PLATFORM_H_
#define UI_AURA_WINDOW_TREE_HOST_PLATFORM_H_

#include <memory>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "ui/aura/aura_export.h"
#include "ui/aura/client/window_types.h"
#include "ui/aura/window.h"
#include "ui/aura/window_tree_host.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/platform_window/platform_window_delegate.h"

///@name USE_NEVA_APPRUNTIME
///@{
#include "ui/base/ime/neva/input_method_neva_observer.h"
///@}

namespace ui {
enum class DomCode;
class PlatformWindow;
class KeyboardHook;
struct PlatformWindowInitProperties;

///@name USE_NEVA_APPRUNTIME
///@{
class LinuxInputMethodContext;
///@}
}  // namespace ui

namespace aura {

// The unified WindowTreeHost implementation for platforms
// that implement PlatformWindow.
class AURA_EXPORT WindowTreeHostPlatform : public WindowTreeHost,
                                           ///@name USE_NEVA_APPRUNTIME
                                           ///@{
                                           public ui::InputMethodNevaObserver,
                                           ///@}
                                           public ui::PlatformWindowDelegate {
 public:
  explicit WindowTreeHostPlatform(ui::PlatformWindowInitProperties properties,
                                  std::unique_ptr<Window> = nullptr);
  ~WindowTreeHostPlatform() override;

  // WindowTreeHost:
  ui::EventSource* GetEventSource() override;
  gfx::AcceleratedWidget GetAcceleratedWidget() override;
  void ShowImpl() override;
  void HideImpl() override;
  gfx::Rect GetBoundsInPixels() const override;
  void SetBoundsInPixels(const gfx::Rect& bounds) override;
  gfx::Point GetLocationOnScreenInPixels() const override;
  void SetCapture() override;
  void ReleaseCapture() override;
  void SetCursorNative(gfx::NativeCursor cursor) override;
  void MoveCursorToScreenLocationInPixels(
      const gfx::Point& location_in_pixels) override;
  void OnCursorVisibilityChangedNative(bool show) override;
  ///@name USE_NEVA_APPRUNTIME
  ///@{
  void SetKeyMask(ui::KeyMask key_mask, bool set) override;
  void SetInputRegion(const std::vector<gfx::Rect>& region) override;
  void SetWindowProperty(const std::string& name,
                         const std::string& value) override;
  void ToggleFullscreen() override;
  void CreateGroup(const ui::WindowGroupConfiguration& config) override;
  void AttachToGroup(const std::string& group_name,
                     const std::string& layer_name) override;
  void FocusGroupOwner() override;
  void FocusGroupLayer() override;
  void DetachGroup() override;
  void XInputActivate(const std::string& type) override;
  void XInputDeactivate() override;
  void XInputInvokeAction(std::uint32_t keysym,
                          ui::XInputKeySymbolType symbol_type,
                          ui::XInputEventType event_type) override;
  ///@}

  ui::PlatformWindow* platform_window() { return platform_window_.get(); }
  const ui::PlatformWindow* platform_window() const {
    return platform_window_.get();
  }

 protected:
  // NOTE: this does not call CreateCompositor(); subclasses must call
  // CreateCompositor() at the appropriate time.
  explicit WindowTreeHostPlatform(std::unique_ptr<Window> window = nullptr);

  // Creates a ui::PlatformWindow appropriate for the current platform and
  // installs it at as the PlatformWindow for this WindowTreeHostPlatform.
  void CreateAndSetPlatformWindow(ui::PlatformWindowInitProperties properties);

  void SetPlatformWindow(std::unique_ptr<ui::PlatformWindow> window);

  // ui::PlatformWindowDelegate:
  void OnBoundsChanged(const gfx::Rect& new_bounds) override;
  void OnDamageRect(const gfx::Rect& damaged_region) override;
  void DispatchEvent(ui::Event* event) override;
  void OnCloseRequest() override;
  void OnClosed() override;
  void OnWindowStateChanged(ui::PlatformWindowState new_state) override;
  void OnLostCapture() override;
  void OnAcceleratedWidgetAvailable(gfx::AcceleratedWidget widget) override;
  void OnWillDestroyAcceleratedWidget() override;
  void OnAcceleratedWidgetDestroyed() override;
  void OnActivationChanged(bool active) override;

  ///@name USE_NEVA_APPRUNTIME
  ///@{
  void OnWindowHostStateChanged(ui::WidgetState new_state) override;
  ui::LinuxInputMethodContext* GetInputMethodContext() override;

#if defined(OS_WEBOS)
  void OnInputPanelVisibilityChanged(bool visibility) override;
  void OnInputPanelRectChanged(int32_t x,
                               int32_t y,
                               uint32_t width,
                               uint32_t height) override;
#endif

  // Overridden from ui::InputMethodNevaObserver:
  void OnShowIme() override;
  void OnHideIme(ui::ImeHiddenType) override;
  void OnTextInputInfoChanged(
      const ui::TextInputInfo& text_input_info) override;
  void SetSurroundingText(const std::string& text,
                          size_t cursor_position,
                          size_t anchor_position) override;
  ///@}

  void OnMouseEnter() override;

  // Overridden from aura::WindowTreeHost:
  bool CaptureSystemKeyEventsImpl(
      base::Optional<base::flat_set<ui::DomCode>> dom_codes) override;
  void ReleaseSystemKeyEventCapture() override;
  bool IsKeyLocked(ui::DomCode dom_code) override;
  base::flat_map<std::string, std::string> GetKeyboardLayoutMap() override;

 private:
  gfx::AcceleratedWidget widget_;
  std::unique_ptr<ui::PlatformWindow> platform_window_;
  gfx::NativeCursor current_cursor_;
  gfx::Rect bounds_in_pixels_;

  std::unique_ptr<ui::KeyboardHook> keyboard_hook_;

  gfx::Size pending_size_;

  // Tracks how nested OnBoundsChanged() is. That is, on entering
  // OnBoundsChanged() this is incremented and on leaving OnBoundsChanged() this
  // is decremented.
  int on_bounds_changed_recursion_depth_ = 0;

  DISALLOW_COPY_AND_ASSIGN(WindowTreeHostPlatform);
};

}  // namespace aura

#endif  // UI_AURA_WINDOW_TREE_HOST_PLATFORM_H_
