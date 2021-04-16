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

// Multiply-included message file, hence no include guard here.

#ifndef OZONE_PLATFORM_EVENT_PARAM_TRAINTS_H_
#define OZONE_PLATFORM_EVENT_PARAM_TRAINTS_H_

#include <string>

#include "ipc/ipc_message_utils.h"
#include "ipc/param_traits_macros.h"
#include "ui/events/event_constants.h"

namespace ui {
struct TouchEventInfo {
  TouchEventInfo() = default;
  ~TouchEventInfo() = default;
  TouchEventInfo(float x, float y, int32_t touch_id, uint32_t time_stamp)
      : x_(x), y_(y), touch_id_(touch_id), time_stamp_(time_stamp) {}
  float x_ = 0;
  float y_ = 0;
  int32_t touch_id_ = 0;
  uint32_t time_stamp_ = 0;
};
}  // namespace ui

namespace IPC {

template <>
struct ParamTraits<ui::TouchEventInfo> {
  typedef ui::TouchEventInfo param_type;
  static void Write(base::Pickle* m, const param_type& p);
  static bool Read(const base::Pickle* m,
                   base::PickleIterator* iter,
                   param_type* r);
  static void Log(const param_type& p, std::string* l);
};

}  // namespace IPC

#endif  // OZONE_PLATFORM_EVENT_PARAM_TRAINTS_H_
