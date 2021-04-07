// Copyright 2019 LG Electronics, Inc.
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

#ifndef UI_OZONE_PLATFORM_WAYLAND_HOST_WAYLAND_EXTENSION_H_
#define UI_OZONE_PLATFORM_WAYLAND_HOST_WAYLAND_EXTENSION_H_

#include <memory>

#include "ui/ozone/platform/wayland/common/wayland_object.h"

namespace ui {

class ExtendedInputWrapper;
class InputManagerWrapper;
class InputPanelManager;
class ShellSurfaceWrapper;
class ShellPopupWrapper;
class SurfaceGroupCompositorWrapper;
class WaylandConnection;
class WaylandWindow;

// Wayland extension abstract interface to support extending of the Wayland
// protocol. Inherit it to provide your own Wayland extension implementation.
class WaylandExtension {
 public:
  WaylandExtension() = default;
  virtual ~WaylandExtension() = default;

  // Binds to the extension interface(s). Can encapsulate binding of several
  // interfaces, defined by |interface|.
  virtual bool Bind(wl_registry* registry,
                    uint32_t name,
                    const char* interface,
                    uint32_t version) = 0;

  // Checks whether the extension has bound shell object(s).
  virtual bool HasShellObject() const = 0;

  // Creates and returns shell surface wrapper object.
  virtual std::unique_ptr<ShellSurfaceWrapper> CreateShellSurface(
      WaylandWindow* window) = 0;

  // Creates and returns shell popup wrapper object.
  virtual std::unique_ptr<ShellPopupWrapper> CreateShellPopup(
      WaylandWindow* window) = 0;

  // Returns extended input wrapper object.
  virtual ExtendedInputWrapper* GetExtendedInput() = 0;

  // Returns input manager wrapper object.
  virtual InputManagerWrapper* GetInputManager() = 0;

  // Returns input panel manager object.
  virtual InputPanelManager* GetInputPanelManager() = 0;

  // Returns surface group compositor wrapper object.
  virtual SurfaceGroupCompositorWrapper* GetSurfaceGroupCompositor() = 0;

  WaylandExtension(const WaylandExtension&) = delete;
  WaylandExtension& operator=(const WaylandExtension&) = delete;
};

// Creates Wayland extension.
std::unique_ptr<WaylandExtension> CreateWaylandExtension(
    WaylandConnection* connection);

}  // namespace ui

#endif  // UI_OZONE_PLATFORM_WAYLAND_HOST_WAYLAND_EXTENSION_H_
