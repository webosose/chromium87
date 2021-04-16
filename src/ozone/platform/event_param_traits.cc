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

#include "ozone/platform/event_param_traits.h"

#include <stddef.h>
#include <stdint.h>

namespace IPC {

void ParamTraits<ui::TouchEventInfo>::Write(base::Pickle* m,
                                            const ui::TouchEventInfo& p) {
  WriteParam(m, p.x_);
  WriteParam(m, p.y_);
  WriteParam(m, p.touch_id_);
  WriteParam(m, p.time_stamp_);
}

bool ParamTraits<ui::TouchEventInfo>::Read(const base::Pickle* m,
                                           base::PickleIterator* iter,
                                           ui::TouchEventInfo* r) {
  float x;
  float y;
  int32_t touch_id;
  uint32_t time_stamp;
  if (!ReadParam(m, iter, &x) || !ReadParam(m, iter, &y) ||
      !ReadParam(m, iter, &touch_id) || !ReadParam(m, iter, &time_stamp))
    return false;
  r->x_ = x;
  r->y_ = y;
  r->touch_id_ = touch_id;
  r->time_stamp_ = time_stamp;
  return true;
}

void ParamTraits<ui::TouchEventInfo>::Log(const ui::TouchEventInfo& p,
                                          std::string* l) {
  l->append(base::StringPrintf("(%i, %i, %i, %i)", p.x_, p.y_, p.touch_id_,
                               p.time_stamp_));
}

}  // namespace IPC
