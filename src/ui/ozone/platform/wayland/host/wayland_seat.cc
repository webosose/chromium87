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

#include "ui/ozone/platform/wayland/host/wayland_seat.h"

#include "base/logging.h"
#include "ui/events/ozone/layout/keyboard_layout_engine_manager.h"
#include "ui/ozone/platform/wayland/host/wayland_connection.h"
#include "ui/ozone/platform/wayland/host/wayland_cursor.h"
#include "ui/ozone/platform/wayland/host/wayland_cursor_position.h"
#include "ui/ozone/platform/wayland/host/wayland_event_source.h"
#include "ui/ozone/platform/wayland/host/wayland_keyboard.h"
#include "ui/ozone/platform/wayland/host/wayland_pointer.h"
#include "ui/ozone/platform/wayland/host/wayland_seat_manager.h"
#include "ui/ozone/platform/wayland/host/wayland_touch.h"

namespace ui {

WaylandSeat::WaylandSeat(std::uint32_t seat_id, wl_seat* seat)
    : seat_id_(seat_id), seat_(seat) {}

WaylandSeat::~WaylandSeat() = default;

void WaylandSeat::Initialize(WaylandSeatManager* seat_manager) {
  DCHECK(!seat_manager_);
  seat_manager_ = seat_manager;
  static const wl_seat_listener seat_listener = {
      &WaylandSeat::Capabilities,
      &WaylandSeat::Name,
  };
  wl_seat_add_listener(seat_.get(), &seat_listener, this);
}

void WaylandSeat::UpdateInputDevices(wl_seat* seat,
                                     std::uint32_t capabilities) {
  DCHECK(seat);

  auto has_pointer = capabilities & WL_SEAT_CAPABILITY_POINTER;
  auto has_keyboard = capabilities & WL_SEAT_CAPABILITY_KEYBOARD;
  auto has_touch = capabilities & WL_SEAT_CAPABILITY_TOUCH;

  if (!has_pointer) {
    pointer_.reset();
    cursor_.reset();
    cursor_position_.reset();
  } else if (wl_pointer* pointer = wl_seat_get_pointer(seat)) {
    pointer_ = std::make_unique<WaylandPointer>(
        pointer, seat_manager_->connection(),
        seat_manager_->connection()->event_source());
    cursor_ = std::make_unique<WaylandCursor>(pointer_.get(),
                                              seat_manager_->connection());
    cursor_position_ = std::make_unique<WaylandCursorPosition>();
  } else {
    LOG(ERROR) << "Failed to get wl_pointer from seat";
  }

  if (!has_keyboard) {
    keyboard_.reset();
  } else if (wl_keyboard* keyboard = wl_seat_get_keyboard(seat)) {
    auto* layout_engine =
        KeyboardLayoutEngineManager::GetKeyboardLayoutEngine();
    keyboard_ = std::make_unique<WaylandKeyboard>(
        keyboard, seat_manager_->connection(), layout_engine,
        seat_manager_->connection()->event_source());
  } else {
    LOG(ERROR) << "Failed to get wl_keyboard from seat";
  }

  if (!has_touch) {
    touch_.reset();
  } else if (wl_touch* touch = wl_seat_get_touch(seat)) {
    touch_ = std::make_unique<WaylandTouch>(
        touch, seat_manager_->connection(),
        seat_manager_->connection()->event_source());
  } else {
    LOG(ERROR) << "Failed to get wl_touch from seat";
  }
}

// static
void WaylandSeat::Capabilities(void* data,
                               wl_seat* seat,
                               std::uint32_t capabilities) {
  WaylandSeat* wayland_seat = static_cast<WaylandSeat*>(data);

  DCHECK(wayland_seat);
  DCHECK(wayland_seat->seat_manager_);
  DCHECK(wayland_seat->seat_manager_->connection());

  wayland_seat->UpdateInputDevices(seat, capabilities);
  wayland_seat->seat_manager_->connection()->ScheduleFlush();
}

// static
void WaylandSeat::Name(void* data, wl_seat* seat, const char* name) {}

}  // namespace ui
