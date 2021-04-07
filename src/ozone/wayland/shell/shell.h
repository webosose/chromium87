// Copyright 2014 Intel Corporation. All rights reserved.
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

#ifndef OZONE_WAYLAND_SHELL_SHELL_H_
#define OZONE_WAYLAND_SHELL_SHELL_H_

#include <wayland-client.h>

#include "ozone/wayland/window.h"

#if defined(OS_WEBOS)
#include "wayland-webos-shell-client-protocol.h"
#endif

struct xdg_shell;
struct ivi_application;
namespace ozonewayland {

class WaylandShellSurface;
class WaylandWindow;

class WaylandShell {
 public:
  WaylandShell();
  ~WaylandShell();
  // Creates shell surface for a given WaylandWindow. This can be either
  // wl_shell, xdg_shell or any shell which supports wayland protocol.
  // Ownership is passed to the caller.
  WaylandShellSurface* CreateShellSurface(WaylandWindow* parent,
                                          WaylandWindow::ShellType type);
  void Initialize(struct wl_registry *registry,
                  uint32_t name,
                  const char *interface,
                  uint32_t version);

  wl_shell* GetWLShell() const { return shell_; }
#if defined(OS_WEBOS)
  wl_webos_shell* GetWebosWLShell() const { return webos_shell_; }
#endif
  xdg_shell* GetXDGShell() const { return xdg_shell_; }
  ivi_application* GetIVIShell() const { return ivi_application_; }

 private:
  static void XDGHandlePing(void* data,
                            struct xdg_shell* xdg_shell,
                            uint32_t serial);
  wl_shell* shell_;
#if defined(OS_WEBOS)
  wl_webos_shell* webos_shell_;
#endif
  xdg_shell* xdg_shell_;
  ivi_application* ivi_application_;
  DISALLOW_COPY_AND_ASSIGN(WaylandShell);
};

}  // namespace ozonewayland

#endif  // OZONE_WAYLAND_SHELL_H_
