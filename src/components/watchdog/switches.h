// Copyright (c) 2020 LG Electronics, Inc.
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

#ifndef COMPONENTS_WATCHDOG_SWITCHES_H_
#define COMPONENTS_WATCHDOG_SWITCHES_H_

namespace watchdog {
namespace switches {

extern const char kEnableWatchdog[];
extern const char kWatchdogBrowserPeriod[];
extern const char kWatchdogBrowserTimeout[];
extern const char kWatchdogRendererPeriod[];
extern const char kWatchdogRendererTimeout[];

}  // namespace switches
}  // namespace watchdog

#endif  // COMPONENTS_WATCHDOG_SWITCHES_H_
