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

#include "media/neva/media_preferences.h"

#include "base/logging.h"

namespace media {

void MediaPreferences::Update(const std::string& media_preferences) {
  VLOG(1) << __func__ << " info: " << media_prefs_info_.ToString();
}

void MediaPreferences::SetMediaCodecCapabilities(
    const std::string& capabilities) {}

bool MediaPreferences::IsSupportedAudioType(const media::AudioType& type) {
  // Defer to media's default support.
  return media::IsDefaultSupportedAudioType(type);
}

bool MediaPreferences::IsSupportedVideoType(const media::VideoType& type) {
  // Defer to media's default support.
  return media::IsDefaultSupportedVideoType(type);
}

}  // namespace media
