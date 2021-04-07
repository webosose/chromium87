// Copyright 2020 LG Electronics, Inc.
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

#ifndef OZONE_WAYLAND_INPUT_TEXT_INPUT_UTILS_H_
#define OZONE_WAYLAND_INPUT_TEXT_INPUT_UTILS_H_

#include <stdint.h>

namespace ozonewayland {

#if defined(OS_WEBOS)
enum IMEModifierFlags : uint32_t { FLAG_SHFT = 1, FLAG_CTRL = 2, FLAG_ALT = 4 };
#endif

uint32_t KeyNumberFromKeySymCode(uint32_t key_sym, uint32_t modifiers);

#if defined(OS_WEBOS)
uint32_t GetModifierKey(IMEModifierFlags key_sym);
#endif

}  // namespace ozonewayland

#endif  // OZONE_WAYLAND_INPUT_TEXT_INPUT_UTILS_H_
