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

#include <sstream>

#if defined(OS_WEBOS)
#include "third_party/jsoncpp/source/include/json/json.h"
#endif  // defined(OS_WEBOS)

namespace media {

MediaPrefsInfo::MediaPrefsInfo() = default;

MediaPrefsInfo::MediaPrefsInfo(const MediaPrefsInfo& other) = default;

MediaPrefsInfo::~MediaPrefsInfo() = default;

std::string MediaPrefsInfo::ToString() const {
  std::ostringstream s;
  s << "mse_use_intrinsic_size: " << (mse_use_intrinsic_size ? "true" : "false")
    << " / is_av1_codec_enabled: " << (is_av1_codec_enabled ? "true" : "false")
    << " / is_supported_dolby_hdr: "
    << (is_supported_dolby_hdr ? "true" : "false")
    << " / is_supported_dolby_atmos: "
    << (is_supported_dolby_atmos  ? "true" : "false");
  return s.str();
}

//  static
MediaPreferences* MediaPreferences::Get() {
  return base::Singleton<MediaPreferences>::get();
}

MediaPreferences::MediaPreferences() = default;

MediaPreferences::~MediaPreferences() = default;

std::string MediaPreferences::GetRawMediaPreferences() {
  base::AutoLock auto_lock(lock_);
  return raw_preferences;
}

bool MediaPreferences::UseIntrinsicSizeForMSE() {
  base::AutoLock auto_lock(lock_);
  return media_prefs_info_.mse_use_intrinsic_size;
}

void MediaPreferences::SetUseIntrinsicSizeForMSE(bool use_intrinsic_size) {
  base::AutoLock auto_lock(lock_);
  media_prefs_info_.mse_use_intrinsic_size = use_intrinsic_size;
}

bool MediaPreferences::IsAV1CodecEnabled() {
  base::AutoLock auto_lock(lock_);
  return media_prefs_info_.is_av1_codec_enabled;
}

std::vector<MediaCodecCapability>
MediaPreferences::GetMediaCodecCapabilities() {
  base::AutoLock auto_lock(lock_);
  return capabilities_;
}

base::Optional<MediaCodecCapability>
MediaPreferences::GetMediaCodecCapabilityForType(const std::string& type) {
  base::AutoLock auto_lock(lock_);

  auto it = std::find_if(capabilities_.begin(), capabilities_.end(),
                         [&type](const MediaCodecCapability& capability) {
                           return type == capability.type;
                         });
  if (it != capabilities_.end())
    return *it;
  return base::nullopt;
}

#if defined(USE_NEVA_WEBRTC)
base::Optional<MediaCodecCapability>
MediaPreferences::GetMediaCodecCapabilityForCodec(const std::string& codec) {
  base::AutoLock auto_lock(lock_);

  auto it = std::find_if(capabilities_.begin(), capabilities_.end(),
                         [&codec](const MediaCodecCapability& capability) {
                           return codec == capability.codec;
                         });
  if (it != capabilities_.end())
    return *it;
  return base::nullopt;
}
#endif

bool MediaPreferences::IsSupportedDolbyHdr() {
  base::AutoLock auto_lock(lock_);
  return media_prefs_info_.is_supported_dolby_hdr;
}

bool MediaPreferences::IsSupportedDolbyAtmos() {
  base::AutoLock auto_lock(lock_);
  return media_prefs_info_.is_supported_dolby_atmos;
}

}  // namespace media
