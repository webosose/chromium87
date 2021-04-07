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

#include "ui/ozone/platform/wayland/host/wayland_seat_manager.h"

#include <algorithm>

#include "base/logging.h"
#include "ui/ozone/platform/wayland/host/wayland_seat.h"

namespace ui {

WaylandSeatManager::WaylandSeatManager(WaylandConnection* connection)
    : connection_(connection) {
  DCHECK(connection_);
}

WaylandSeatManager::~WaylandSeatManager() = default;

void WaylandSeatManager::AddSeat(std::uint32_t seat_id, wl_seat* seat) {
  auto seat_it = GetSeatIteratorById(seat_id);
  DCHECK(seat_it == seat_list_.end());

  auto wayland_seat = std::make_unique<WaylandSeat>(seat_id, seat);
  WaylandSeat* wayland_seat_ptr = wayland_seat.get();
  seat_list_.push_back(std::move(wayland_seat));

  wayland_seat_ptr->Initialize(this);
}

WaylandSeat* WaylandSeatManager::GetFirstSeat() const {
  if (!seat_list_.empty())
    return seat_list_.front().get();
  return nullptr;
}

WaylandSeatManager::SeatList::const_iterator
WaylandSeatManager::GetSeatIteratorById(std::uint32_t seat_id) const {
  return std::find_if(
      seat_list_.begin(), seat_list_.end(),
      [seat_id](const auto& item) { return item->seat_id() == seat_id; });
}

}  // namespace ui
