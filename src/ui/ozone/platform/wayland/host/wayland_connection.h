// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PLATFORM_WAYLAND_HOST_WAYLAND_CONNECTION_H_
#define UI_OZONE_PLATFORM_WAYLAND_HOST_WAYLAND_CONNECTION_H_

#include <memory>
#include <vector>

#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/events/event.h"
#include "ui/ozone/platform/wayland/common/wayland_object.h"
#include "ui/ozone/platform/wayland/host/wayland_clipboard.h"
#include "ui/ozone/platform/wayland/host/wayland_data_drag_controller.h"
#include "ui/ozone/platform/wayland/host/wayland_data_source.h"
#include "ui/ozone/platform/wayland/host/wayland_window_manager.h"

#if defined(USE_NEVA_APPRUNTIME)
#include "ui/ozone/platform/wayland/host/wayland_seat.h"
#include "ui/ozone/platform/wayland/host/wayland_seat_manager.h"
#endif  // defined(USE_NEVA_APPRUNTIME)

namespace gfx {
class Point;
}

namespace ui {

class WaylandBufferManagerHost;
class WaylandCursor;
class WaylandDrm;
class WaylandEventSource;
class WaylandKeyboard;
class WaylandOutputManager;
class WaylandPointer;
class WaylandShm;
class WaylandTouch;
class WaylandZwpLinuxDmabuf;
class WaylandDataDeviceManager;
class WaylandCursorPosition;
class WaylandWindowDragController;
class GtkPrimarySelectionDeviceManager;
class XdgForeignWrapper;

///@name USE_NEVA_APPRUNTIME
///@{
class WaylandExtension;
///@}

class WaylandConnection {
 public:
  // Stores the last serial and the event type it is associated with.
  struct EventSerial {
    uint32_t serial = 0;
    EventType event_type = EventType::ET_UNKNOWN;
  };

  WaylandConnection();
  WaylandConnection(const WaylandConnection&) = delete;
  WaylandConnection& operator=(const WaylandConnection&) = delete;
  ~WaylandConnection();

  bool Initialize();

  // Schedules a flush of the Wayland connection.
  void ScheduleFlush();

  // Sets a callback that that shutdowns the browser in case of unrecoverable
  // error. Called by WaylandEventWatcher.
  void SetShutdownCb(base::OnceCallback<void()> shutdown_cb);

  wl_display* display() const { return display_.get(); }
  wl_compositor* compositor() const { return compositor_.get(); }
  uint32_t compositor_version() const { return compositor_version_; }
  wl_subcompositor* subcompositor() const { return subcompositor_.get(); }
  wp_viewporter* viewporter() const { return viewporter_.get(); }
  xdg_wm_base* shell() const { return shell_.get(); }
  zxdg_shell_v6* shell_v6() const { return shell_v6_.get(); }
  zaura_shell* aura_shell() const { return aura_shell_.get(); }
#if !defined(USE_NEVA_APPRUNTIME)
  wl_seat* seat() const { return seat_.get(); }
#endif  // !defined(USE_NEVA_APPRUNTIME)
  wp_presentation* presentation() const { return presentation_.get(); }
  zwp_text_input_manager_v1* text_input_manager_v1() const {
    return text_input_manager_v1_.get();
  }
  zwp_linux_explicit_synchronization_v1* linux_explicit_synchronization_v1()
      const {
    return linux_explicit_synchronization_.get();
  }
  zxdg_decoration_manager_v1* xdg_decoration_manager_v1() const {
    return xdg_decoration_manager_.get();
  }

  void set_serial(uint32_t serial, EventType event_type) {
    serial_ = {serial, event_type};
  }
  uint32_t serial() const { return serial_.serial; }
  EventSerial event_serial() const { return serial_; }

  void SetCursorBitmap(const std::vector<SkBitmap>& bitmaps,
                       const gfx::Point& location);

  WaylandEventSource* event_source() const { return event_source_.get(); }

#if defined(USE_NEVA_APPRUNTIME)
  wl_seat* seat() const {
    if (seat_manager_ && seat_manager_->GetFirstSeat())
      return seat_manager_->GetFirstSeat()->seat();
    return nullptr;
  }

  // Returns current cursor, which may be null.
  WaylandCursor* cursor() const {
    if (seat_manager_ && seat_manager_->GetFirstSeat())
      return seat_manager_->GetFirstSeat()->cursor();
    return nullptr;
  }

  // Returns current keyboard, which may be null.
  WaylandKeyboard* keyboard() const {
    if (seat_manager_ && seat_manager_->GetFirstSeat())
      return seat_manager_->GetFirstSeat()->keyboard();
    return nullptr;
  }

  // Returns current pointer, which may be null.
  WaylandPointer* pointer() const {
    if (seat_manager_ && seat_manager_->GetFirstSeat())
      return seat_manager_->GetFirstSeat()->pointer();
    return nullptr;
  }

  // Returns current touch, which may be null.
  WaylandTouch* touch() const {
    if (seat_manager_ && seat_manager_->GetFirstSeat())
      return seat_manager_->GetFirstSeat()->touch();
    return nullptr;
  }

  // Returns current cursor position, which may be null.
  WaylandCursorPosition* wayland_cursor_position() const {
    if (seat_manager_ && seat_manager_->GetFirstSeat())
      return seat_manager_->GetFirstSeat()->cursor_position();
    return nullptr;
  }

  WaylandSeatManager* seat_manager() const { return seat_manager_.get(); }
#else  // defined(USE_NEVA_APPRUNTIME)
  // Returns the current touch, which may be null.
  WaylandTouch* touch() const { return touch_.get(); }

  // Returns the current pointer, which may be null.
  WaylandPointer* pointer() const { return pointer_.get(); }

  // Returns the current keyboard, which may be null.
  WaylandKeyboard* keyboard() const { return keyboard_.get(); }
#endif  // !defined(USE_NEVA_APPRUNTIME)

  WaylandClipboard* clipboard() const { return clipboard_.get(); }

  WaylandOutputManager* wayland_output_manager() const {
    return wayland_output_manager_.get();
  }

#if !defined(USE_NEVA_APPRUNTIME)
  // Returns the cursor position, which may be null.
  WaylandCursorPosition* wayland_cursor_position() const {
    return wayland_cursor_position_.get();
  }
#endif  // !defined(USE_NEVA_APPRUNTIME)

  WaylandBufferManagerHost* buffer_manager_host() const {
    return buffer_manager_host_.get();
  }

  ///@name USE_NEVA_APPRUNTIME
  ///@{
  WaylandExtension* extension() { return extension_.get(); }
  ///@}

  WaylandZwpLinuxDmabuf* zwp_dmabuf() const { return zwp_dmabuf_.get(); }

#if !defined(OS_WEBOS)
  WaylandDrm* drm() const { return drm_.get(); }
#else  // defined(OS_WEBOS)
  WaylandDrm* drm() const { return nullptr; }
#endif  // !defined(OS_WEBOS)

  WaylandShm* shm() const { return shm_.get(); }

  WaylandWindowManager* wayland_window_manager() {
    return &wayland_window_manager_;
  }

  WaylandDataDeviceManager* data_device_manager() const {
    return data_device_manager_.get();
  }

  GtkPrimarySelectionDeviceManager* primary_selection_device_manager() const {
    return primary_selection_device_manager_.get();
  }

  WaylandDataDragController* data_drag_controller() const {
    return data_drag_controller_.get();
  }

  WaylandWindowDragController* window_drag_controller() const {
    return window_drag_controller_.get();
  }

  XdgForeignWrapper* xdg_foreign() const { return xdg_foreign_.get(); }

  // Returns true when dragging is entered or started.
  bool IsDragInProgress() const;

  // Creates a new wl_surface.
  wl::Object<wl_surface> CreateSurface();

 private:
  void Flush();
#if !defined(USE_NEVA_APPRUNTIME)
  void UpdateInputDevices(wl_seat* seat, uint32_t capabilities);
#endif  // !defined(USE_NEVA_APPRUNTIME)

  // Initialize data-related objects if required protocol objects are already
  // in place, i.e: wl_seat and wl_data_device_manager.
  void CreateDataObjectsIfReady();

  // Creates WaylandKeyboard with the currently acquired protocol objects, if
  // possible. Returns true iff WaylandKeyboard was created.
  bool CreateKeyboard();

  // wl_registry_listener
  static void Global(void* data,
                     wl_registry* registry,
                     uint32_t name,
                     const char* interface,
                     uint32_t version);
  static void GlobalRemove(void* data, wl_registry* registry, uint32_t name);

#if !defined(USE_NEVA_APPRUNTIME)
  // wl_seat_listener
  static void Capabilities(void* data, wl_seat* seat, uint32_t capabilities);
  static void Name(void* data, wl_seat* seat, const char* name);
#endif  // !defined(USE_NEVA_APPRUNTIME)

  // zxdg_shell_v6_listener
  static void PingV6(void* data, zxdg_shell_v6* zxdg_shell_v6, uint32_t serial);

  // xdg_wm_base_listener
  static void Ping(void* data, xdg_wm_base* shell, uint32_t serial);

  uint32_t compositor_version_ = 0;
  wl::Object<wl_display> display_;
  wl::Object<wl_registry> registry_;
  wl::Object<wl_compositor> compositor_;
  wl::Object<wl_subcompositor> subcompositor_;
#if !defined(USE_NEVA_APPRUNTIME)
  wl::Object<wl_seat> seat_;
#endif  // !defined(USE_NEVA_APPRUNTIME)
  wl::Object<xdg_wm_base> shell_;
  wl::Object<zxdg_shell_v6> shell_v6_;
  wl::Object<wp_presentation> presentation_;
  wl::Object<wp_viewporter> viewporter_;
  wl::Object<zcr_keyboard_extension_v1> keyboard_extension_v1_;
  wl::Object<zwp_text_input_manager_v1> text_input_manager_v1_;
  wl::Object<zaura_shell> aura_shell_;
  wl::Object<zwp_linux_explicit_synchronization_v1>
      linux_explicit_synchronization_;
  wl::Object<zxdg_decoration_manager_v1> xdg_decoration_manager_;

  // Event source instance. Must be declared before input objects so it
  // outlives them so thus being able to properly handle their destruction.
  std::unique_ptr<WaylandEventSource> event_source_;

#if defined(USE_NEVA_APPRUNTIME)
  std::unique_ptr<WaylandSeatManager> seat_manager_;
#else  // defined(USE_NEVA_APPRUNTIME)
  // Input device objects.
  std::unique_ptr<WaylandKeyboard> keyboard_;
  std::unique_ptr<WaylandPointer> pointer_;
  std::unique_ptr<WaylandTouch> touch_;

  std::unique_ptr<WaylandCursor> cursor_;
#endif  // !defined(USE_NEVA_APPRUNTIME)
  std::unique_ptr<WaylandDataDeviceManager> data_device_manager_;
  std::unique_ptr<WaylandClipboard> clipboard_;
  std::unique_ptr<WaylandOutputManager> wayland_output_manager_;
#if !defined(USE_NEVA_APPRUNTIME)
  std::unique_ptr<WaylandCursorPosition> wayland_cursor_position_;
#endif  // !defined(USE_NEVA_APPRUNTIME)
  std::unique_ptr<WaylandZwpLinuxDmabuf> zwp_dmabuf_;
#if !defined(OS_WEBOS)
  std::unique_ptr<WaylandDrm> drm_;
#endif  // !defined(OS_WEBOS)
  std::unique_ptr<WaylandShm> shm_;
  std::unique_ptr<WaylandBufferManagerHost> buffer_manager_host_;
  std::unique_ptr<XdgForeignWrapper> xdg_foreign_;

  ///@name USE_NEVA_APPRUNTIME
  ///@{
  std::unique_ptr<WaylandExtension> extension_;
  ///@}

  std::unique_ptr<GtkPrimarySelectionDeviceManager>
      primary_selection_device_manager_;

  std::unique_ptr<WaylandDataDragController> data_drag_controller_;
  std::unique_ptr<WaylandWindowDragController> window_drag_controller_;

  // Manages Wayland windows.
  WaylandWindowManager wayland_window_manager_;

  bool scheduled_flush_ = false;

  EventSerial serial_;
};

}  // namespace ui

#endif  // UI_OZONE_PLATFORM_WAYLAND_HOST_WAYLAND_CONNECTION_H_
