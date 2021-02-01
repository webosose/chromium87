// Copyright 2013 The Chromium Authors. All rights reserved.
// Copyright 2013 Intel Corporation. All rights reserved.
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

#ifndef OZONE_WAYLAND_DISPLAY_H_
#define OZONE_WAYLAND_DISPLAY_H_

#if !defined(__STDC_FORMAT_MACROS)
#define __STDC_FORMAT_MACROS
#endif

#define wl_array_for_each_type(type, pos, array) \
  for (type *pos = reinterpret_cast<type*>((array)->data); \
       reinterpret_cast<const char*>(pos) < \
         (reinterpret_cast<const char*>((array)->data) + (array)->size); \
       pos++)

#include <wayland-client.h>

#include <list>
#include <map>
#include <memory>
#include <queue>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/single_thread_task_runner.h"
#include "base/task/current_thread.h"
#include "ozone/platform/input_content_type.h"
#include "ozone/wayland/egl/gl_surface_wayland.h"
#include "ui/base/ime/neva/input_method_common.h"
#include "ui/events/event_constants.h"
#include "ui/events/types/event_type.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gl/gl_surface.h"
#include "ui/ozone/public/gpu_platform_support.h"
#include "ui/ozone/public/surface_factory_ozone.h"
#include "ui/platform_window/neva/xinput_types.h"

#if defined(USE_NEVA_MEDIA)
#include "base/unguessable_token.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "ozone/media/video_window_controller_impl.h"
#include "ui/platform_window/neva/mojom/video_window_controller.mojom.h"
#endif  // defined(USE_NEVA_MEDIA)

#include "ui/views/widget/desktop_aura/neva/ui_constants.h"

#if defined(OS_WEBOS)
#include "ozone/platform/webos_constants.h"
#endif

struct gbm_device;
struct wl_egl_window;
#if defined(OS_WEBOS)
struct text_model_factory;
struct wl_webos_input_manager;
struct wl_webos_seat;
struct wl_webos_xinput;
struct wl_webos_xinput_extension;
#if defined(USE_NEVA_MEDIA) && defined(USE_GAV)
struct wl_webos_foreign;
#endif  // defined(USE_NEVA_MEDIA) && defined(USE_GAV)
#else
struct wl_text_input_manager;
#endif

namespace base {
class MessageLoop;
class Thread;
}

namespace IPC {
class Sender;
}

namespace ui {
class WindowGroupConfiguration;
}

namespace ozonewayland {

class WaylandScreen;
class WaylandSeat;
class WaylandShell;
class WaylandWindow;

#if defined(OS_WEBOS)
class WebOSSurfaceGroupCompositor;
#endif

typedef std::map<unsigned, std::unique_ptr<WaylandWindow>> WindowMap;

// WaylandDisplay is a wrapper around wl_display. Once we get a valid
// wl_display, the Wayland server will send different events to register
// the Wayland compositor, shell, screens, input devices, ...
class WaylandDisplay : public ui::SurfaceFactoryOzone,
                       public ui::GpuPlatformSupport,
                       public base::MessagePumpForIO::FdWatcher {
 public:
  WaylandDisplay();
  ~WaylandDisplay() override;

  // Ownership is not passed to the caller.
  static WaylandDisplay* GetInstance() { return instance_; }

  // Returns a pointer to wl_display.
  wl_display* display() const { return display_; }

  wl_registry* registry() const { return registry_; }

  // Warning: Most uses of this function need to be removed in order to fix
  // multiseat. See: https://github.com/01org/ozone-wayland/issues/386
  WaylandSeat* PrimarySeat() const { return primary_seat_; }

  // Returns a list of the registered screens.
  const std::list<WaylandScreen*>& GetScreenList() const;
  WaylandScreen* PrimaryScreen() const { return primary_screen_ ; }

  WaylandShell* GetShell() const { return shell_; }

  wl_shm* GetShm() const { return shm_; }
  wl_compositor* GetCompositor() const { return compositor_; }
#if defined(OS_WEBOS)
  text_model_factory* GetTextModelFactory() const;
  WebOSSurfaceGroupCompositor* GetGroupCompositor() const;
#if defined(USE_NEVA_MEDIA) && defined(USE_GAV)
  struct wl_webos_foreign* GetWebosForeign() const;
#endif  // defined(USE_NEVA_MEDIA) && defined(USE_GAV)
#else
  struct wl_text_input_manager* GetTextInputManager() const;
#endif

  wl_data_device_manager*
  GetDataDeviceManager() const { return data_device_manager_; }

  int GetDisplayFd() const { return wl_display_get_fd(display_); }
  unsigned GetSerial() const { return serial_; }
  void SetSerial(unsigned serial) { serial_ = serial; }
  // Returns WaylandWindow associated with w. The ownership is not transferred
  // to the caller.
  WaylandWindow* GetWindow(unsigned window_handle) const;
  wl_egl_window* GetEglWindow(unsigned window_handle);

  // Destroys WaylandWindow whose handle is w.
  void DestroyWindow(unsigned w);

  // Does a round trip to Wayland server. This call blocks the current thread
  // until all pending request are processed by the server.
  void FlushDisplay();

  bool InitializeHardware();

  // SurfaceFactoryOzone:
  std::vector<gl::GLImplementation> GetAllowedGLImplementations() override;
  ui::GLOzone* GetGLOzone(gl::GLImplementation implementation) override;
  std::unique_ptr<ui::SurfaceOzoneCanvas> CreateCanvasForWidget(
      gfx::AcceleratedWidget widget) override;
  scoped_refptr<gfx::NativePixmap> CreateNativePixmap(
      gfx::AcceleratedWidget widget,
      VkDevice vk_device,
      gfx::Size size,
      gfx::BufferFormat format,
      gfx::BufferUsage usage,
      base::Optional<gfx::Size> framebuffer_size = base::nullopt) override;
  scoped_refptr<gfx::NativePixmap> CreateNativePixmapFromHandle(
      gfx::AcceleratedWidget widget,
      gfx::Size size,
      gfx::BufferFormat format,
      gfx::NativePixmapHandle handle) override;

  void MotionNotify(float x, float y);
  void ButtonNotify(unsigned handle,
                    ui::EventType type,
                    ui::EventFlags flags,
                    float x,
                    float y);
  void AxisNotify(float x, float y, int xoffset, int yoffset);
  void PointerEnter(unsigned handle, float x, float y);
  void PointerLeave(unsigned handle, float x, float y);
  void KeyNotify(ui::EventType type, unsigned code, int device_id);
  void VirtualKeyNotify(ui::EventType type, uint32_t key, int device_id);
  void TouchNotify(ui::EventType type,
                   float x,
                   float y,
                   int32_t touch_id,
                   uint32_t time_stamp);

  void OutputScreenChanged(unsigned width, unsigned height, int rotation);
  void WindowResized(unsigned handle, unsigned width, unsigned height);
  void WindowUnminimized(unsigned windowhandle);
  void WindowDeActivated(unsigned windowhandle);
  void WindowActivated(unsigned windowhandle);
  void CloseWidget(unsigned handle);

  void Commit(unsigned handle, const std::string& text);
  void PreeditChanged(unsigned handle,
                      const std::string& text,
                      const std::string& commit);
  void DeleteRange(unsigned handle, int32_t index, uint32_t length);
  void PreeditEnd();
  void PreeditStart();
  void InitializeXKB(base::UnsafeSharedMemoryRegion region);

  void DragEnter(unsigned windowhandle,
                 float x,
                 float y,
                 const std::vector<std::string>& mime_types,
                 uint32_t serial);
  void DragData(unsigned windowhandle, base::FileDescriptor pipefd);
  void DragLeave(unsigned windowhandle);
  void DragMotion(unsigned windowhandle, float x, float y, uint32_t time);
  void DragDrop(unsigned windowhandle);

#if defined(ENABLE_DRM_SUPPORT)
  // DRM related.
  void DrmHandleDevice(const char*);
  void SetWLDrmFormat(uint32_t);
  void DrmAuthenticated();
  void SetDrmCapabilities(uint32_t);
#endif

#if defined(OS_WEBOS)
  void InputPanelRectChanged(unsigned handle,
                             int32_t x,
                             int32_t y,
                             uint32_t width,
                             uint32_t height);
  void InputPanelStateChanged(unsigned handle, webos::InputPanelState state);
  void TextInputModifier(uint32_t state, uint32_t modifier);
#endif

  // Additional notification for app-runtime
  void InputPanelVisibilityChanged(unsigned handle, bool visibility);
  void NativeWindowExposed(unsigned handle);
  void NativeWindowStateChanged(unsigned handle, ui::WidgetState new_state);
  void NativeWindowStateAboutToChange(unsigned handle, ui::WidgetState state);
  void WindowClose(unsigned handle);
  void KeyboardEnter(unsigned handle);
  void KeyboardLeave(unsigned handle);

#if defined(USE_NEVA_MEDIA)
  void BindVideoWindowController(
      mojo::PendingReceiver<ui::mojom::VideoWindowController> receiver);
  ui::VideoWindowControllerImpl* GetVideoWindowControllerImpl();
#endif
 private:
  typedef std::queue<IPC::Message*> DeferredMessages;
  void InitializeDisplay();
  // Creates a WaylandWindow backed by EGL Window and maps it to w. This can be
  // useful for callers to track a particular surface. By default the type of
  // surface(i.e. toplevel, menu) is none. One needs to explicitly call
  // WaylandWindow::SetShellAttributes to set this. The ownership of
  // WaylandWindow is not passed to the caller.
  WaylandWindow* CreateAcceleratedSurface(unsigned w);

  // Starts polling on display fd. This should be used when one needs to
  // continuously read pending events coming from Wayland compositor and
  // dispatch them. The polling is done completely on a separate thread and
  // doesn't block the thread from which this is called.
  void StartProcessingEvents();
  // Stops polling on display fd.
  void StopProcessingEvents();

  void StartWatchFileDescriptor();
  void PrepareForReadEvents();
  void ReadEvents();
  void EventsDispatch();
  void EventsDispatchForPreparing();

  void Terminate();
  WaylandWindow* GetWidget(unsigned w) const;
  void SetWidgetState(unsigned widget, ui::WidgetState state);
  void SetWidgetTitle(unsigned w, const base::string16& title);
  void CreateWidget(unsigned widget);
  void InitWindow(unsigned widget,
                  unsigned parent,
                  const gfx::Rect& rect,
                  ui::WidgetType type);
  void MoveWindow(unsigned widget, unsigned parent,
                  ui::WidgetType type, const gfx::Rect& rect);
  void AddRegion(unsigned widget, int left, int top, int right, int bottom);
  void SubRegion(unsigned widget, int left, int top, int right, int bottom);
  void SetCursorBitmap(const std::vector<SkBitmap>& bitmaps,
                       const gfx::Point& location);
  void MoveCursor(const gfx::Point& location);
  void ResetIme();
  void ImeCaretBoundsChanged(gfx::Rect rect);
  void ShowInputPanel(unsigned handle);
  void HideInputPanel(ui::ImeHiddenType);
  void SetInputContentType(ui::InputContentType content_type,
                           int text_input_flags,
                           unsigned handle);
  void RequestDragData(const std::string& mime_type);
  void RequestSelectionData(const std::string& mime_type);
  void DragWillBeAccepted(uint32_t serial, const std::string& mime_type);
  void DragWillBeRejected(uint32_t serial);
  void SetWindowProperty(unsigned w,
                         const std::string& name,
                         const std::string& value);
  void CreateWindowGroup(unsigned w,
                         const ui::WindowGroupConfiguration& config);
  void AttachToWindowGroup(unsigned w,
                           const std::string& group,
                           const std::string& layer);
  void FocusWindowGroupOwner(unsigned w);
  void FocusWindowGroupLayer(unsigned w);
  void DetachWindowGroup(unsigned w);

  // This handler resolves all server events used in initialization. It also
  // handles input device registration, screen registration.
  static void DisplayHandleGlobal(
      void *data,
      struct wl_registry *registry,
      uint32_t name,
      const char *interface,
      uint32_t version);

#if defined(OS_WEBOS)
  static void OnWebosInputPointerListener(
      void* data,
      wl_webos_input_manager*,
      uint32_t visible,
      wl_webos_seat*);

  void PointerVisibilityNotify(bool visible);
#endif

  // GpuPlatformSupport:
  void OnChannelEstablished(IPC::Sender* sender) override;
  bool OnMessageReceived(const IPC::Message& message) override;
  IPC::MessageFilter* GetMessageFilter() override;

  // base::MessagePumpForIO::FdWatcher
  void OnFileCanReadWithoutBlocking(int fd) override;
  void OnFileCanWriteWithoutBlocking(int fd) override;

  // Posts task to main loop of the thread on which Dispatcher was initialized.
  void Dispatch(IPC::Message* message);

  void SetInputRegion(unsigned handle, const std::vector<gfx::Rect>& region);
  void SetGroupKeyMask(unsigned handle, ui::KeyMask key_mask);
  void SetKeyMask(unsigned handle, ui::KeyMask key_mask, bool set);
  void SetSurroundingText(const std::string& text,
                          size_t cursor_position,
                          size_t anchor_position);
  void XInputActivate(const std::string& type);
  void XInputDeactivate();
  void XInputInvokeAction(uint32_t keysym,
                          ui::XInputKeySymbolType sym_type,
                          ui::XInputEventType event_type);

  // WaylandDisplay manages the memory of all these pointers.
  wl_display* display_;
  wl_registry* registry_;
  wl_compositor* compositor_;
  wl_data_device_manager* data_device_manager_;
  WaylandShell* shell_;
  wl_shm* shm_;
#if defined(OS_WEBOS)
  text_model_factory* text_model_factory_;
  wl_webos_xinput_extension* webos_xinput_extension_;
  wl_webos_xinput* webos_xinput_;
  std::unique_ptr<WebOSSurfaceGroupCompositor> group_compositor_;
  wl_webos_input_manager* webos_input_manager_ = nullptr;
  bool pointer_visible_ = false;
#if defined(USE_NEVA_MEDIA) && defined(USE_GAV)
  struct wl_webos_foreign* webos_foreign_ = nullptr;
#endif  // defined(USE_NEVA_MEDIA) && defined(USE_GAV)
#else
  struct wl_text_input_manager* text_input_manager_;
#endif
  WaylandScreen* primary_screen_;
  WaylandSeat* primary_seat_;
#if defined(ENABLE_DRM_SUPPORT)
  gbm_device* device_;
  char* m_deviceName;
#endif
  IPC::Sender* sender_;
  scoped_refptr<base::SingleThreadTaskRunner> gpu_task_runner_;
  std::unique_ptr<base::Thread> io_wayland_thread_;
  bool prepared_ = false;
  bool read_pending_ = false;

  std::list<WaylandScreen*> screen_list_;
  std::list<WaylandSeat*> seat_list_;
  WindowMap widget_map_;
  // Display queues messages till Channel is establised.
  DeferredMessages deferred_messages_;
  unsigned serial_;
  bool watching_ = false;
  base::MessagePumpForIO::FdWatchController controller_;
#if defined(ENABLE_DRM_SUPPORT)
  bool m_authenticated_ :1;
  int m_fd_;
  uint32_t m_capabilities_;
#endif
  std::unique_ptr<ui::GLOzone> egl_implementation_;
#if defined(USE_NEVA_MEDIA)
  std::unique_ptr<ui::VideoWindowControllerImpl> video_window_controller_impl_;
#endif  // defined(USE_NEVA_MEDIA)
  static WaylandDisplay* instance_;
  // Support weak pointers for attach & detach callbacks.
  base::WeakPtrFactory<WaylandDisplay> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(WaylandDisplay);
};

}  // namespace ozonewayland

#endif  // OZONE_WAYLAND_DISPLAY_H_
