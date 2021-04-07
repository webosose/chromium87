// Copyright 2013 Intel Corporation. All rights reserved.
// Copyright 2014-2018 LG Electronics, Inc.
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

#include "ozone/wayland/shell/shell_surface.h"

#include "ozone/wayland/display.h"
#include "ozone/wayland/seat.h"

namespace ozonewayland {

WaylandShellSurface::WaylandShellSurface()
    : surface_(NULL) {
  WaylandDisplay* display = WaylandDisplay::GetInstance();
  surface_ = wl_compositor_create_surface(display->GetCompositor());
}

WaylandShellSurface::~WaylandShellSurface() {
  DCHECK(surface_);
  wl_surface_destroy(surface_);
  FlushDisplay();
}

struct wl_surface* WaylandShellSurface::GetWLSurface() const {
    return surface_;
}

void WaylandShellSurface::FlushDisplay() const {
  WaylandDisplay* display = WaylandDisplay::GetInstance();
  DCHECK(display);
  display->FlushDisplay();
}

void WaylandShellSurface::SetWindowProperty(const std::string& name,
                                            const std::string& value) {
  NOTIMPLEMENTED() << " Trying to set the \"" << name
                   << "\" property to \"" << value << "\"";
}

void WaylandShellSurface::PopupDone() {
  WaylandDisplay* display = WaylandDisplay::GetInstance();
  WaylandSeat* seat = display->PrimarySeat();

  if (!seat->GetGrabWindowHandle())
    return;
  display->CloseWidget(seat->GetGrabWindowHandle());
  seat->SetGrabWindowHandle(0, 0);
}

void WaylandShellSurface::WindowResized(void* data,
                                 unsigned width,
                                 unsigned height) {
  WaylandWindow *window = static_cast<WaylandWindow*>(data);
  WaylandDisplay::GetInstance()->WindowResized(window->Handle(), width, height);
}

void WaylandShellSurface::WindowActivated(void *data) {
  WaylandWindow *window = static_cast<WaylandWindow*>(data);
  WaylandShellSurface* shellSurface = window->ShellSurface();

  WaylandDisplay* dispatcher = WaylandDisplay::GetInstance();

  if (shellSurface->IsMinimized()) {
    shellSurface->Unminimize();
    dispatcher->WindowUnminimized(window->Handle());
  } else {
    dispatcher->WindowActivated(window->Handle());
  }
}

void WaylandShellSurface::WindowDeActivated(void *data) {
  WaylandWindow *window = static_cast<WaylandWindow*>(data);
  WaylandDisplay::GetInstance()->WindowDeActivated(window->Handle());
}

}  // namespace ozonewayland
