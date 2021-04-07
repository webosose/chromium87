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

#ifndef NEVA_PAL_SERVICE_WEBOS_LUNA_LUNA_NAMES_H_
#define NEVA_PAL_SERVICE_WEBOS_LUNA_LUNA_NAMES_H_

#include <string>

namespace pal {
namespace luna {

namespace service_uri {

extern const char kAudio[];
extern const char kSettings[];
extern const char kMemoryManager[];
extern const char kPalmBus[];
extern const char kApplicationManager[];

}  // namespace service_uri

namespace service_name {

extern const char kChromiumMedia[];
extern const char kChromiumMemory[];
extern const char kChromiumPlatformSystem[];

}  // namespace service_name

std::string GetServiceURI(const char* uri, const char* action);

std::string GetServiceNameWithRandSuffix(const char* name);

std::string GetServiceNameWithPID(const char* name);

}  // namespace luna
}  // namespace pal

#endif // NEVA_PAL_SERVICE_WEBOS_LUNA_LUNA_NAMES_H_
