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

#include "ozone/wayland/shell/ivi_shell_surface.h"

#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"

#include "ozone/wayland/display.h"
#include "ozone/wayland/protocol/ivi-application-client-protocol.h"
#include "ozone/wayland/shell/shell.h"
#define IVI_SURFACE_ID 7000

namespace ozonewayland {

int IVIShellSurface::last_ivi_surface_id_ = IVI_SURFACE_ID;

IVIShellSurface::IVIShellSurface()
    : WaylandShellSurface(),
      ivi_surface_(NULL),
      ivi_surface_id_(IVI_SURFACE_ID) {
}

IVIShellSurface::~IVIShellSurface() {
  ivi_surface_destroy(ivi_surface_);
}

void IVIShellSurface::InitializeShellSurface(WaylandWindow* window,
                                             WaylandWindow::ShellType type) {
  DCHECK(!ivi_surface_);
  WaylandDisplay* display = WaylandDisplay::GetInstance();
  DCHECK(display);
  WaylandShell* shell = WaylandDisplay::GetInstance()->GetShell();
  DCHECK(shell && shell->GetIVIShell());
  char *env;
  if ((env = getenv("OZONE_WAYLAND_IVI_SURFACE_ID")))
    ivi_surface_id_ = atoi(env);
  else
    ivi_surface_id_ = last_ivi_surface_id_ + 1;
  ivi_surface_ = ivi_application_surface_create(
                     shell->GetIVIShell(), ivi_surface_id_, GetWLSurface());
  last_ivi_surface_id_ = ivi_surface_id_;

  DCHECK(ivi_surface_);
}

void IVIShellSurface::UpdateShellSurface(WaylandWindow::ShellType type,
                                         WaylandShellSurface* shell_parent,
                                         int x,
                                         int y) {
  WaylandShellSurface::FlushDisplay();
}

void IVIShellSurface::SetWindowTitle(const base::string16& title) {
}

void IVIShellSurface::Maximize() {
}

void IVIShellSurface::Minimize() {
}

void IVIShellSurface::Unminimize() {
}

bool IVIShellSurface::IsMinimized() const {
  return false;
}


}  // namespace ozonewayland
