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

#include "cc/base/switches_neva.h"

namespace cc {
namespace switches {

// Sets custom mouse wheel gesture scroll delta value. Unit is pixel.
// From this value, we can allow web apps to change scroll distance
// for mouse wheel event.
// This value is refered to scroll delta in gesture scroll update event
// on renderer side.
// Note that this value is valid only if renderer turns on native scroll and
// threaded compositing. Also this value is initialized as default value(180)
// when native scroll is enabled and this value is not provided in command
// line argument.
const char kCustomMouseWheelGestureScrollDeltaOnWebOSNativeScroll[] =
    "custom-mouse-wheel-gesture-scroll-delta-on-webos-native-scroll";

// Enables using aggressive release policy to reclaim allocated resources
// when application goes into hidden state.
const char kEnableAggressiveReleasePolicy[] =
    "enable-aggressive-release-policy";

// Enable native-scroll feature
const char kEnableWebOSNativeScroll[] = "enable-webos-native-scroll";

}  // namespace switches
}  // namespace cc
