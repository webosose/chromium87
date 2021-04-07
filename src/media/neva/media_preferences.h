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

#ifndef MEDIA_NEVA_MEDIA_PREFERENCES_H_
#define MEDIA_NEVA_MEDIA_PREFERENCES_H_

#include "base/memory/singleton.h"
#include "base/synchronization/lock.h"
#include "media/base/media_export.h"
#include "media/base/supported_types.h"
#include "media/neva/media_codec_capability.h"

namespace media {

struct MEDIA_EXPORT MediaPrefsInfo {
 public:
  MediaPrefsInfo();

  MediaPrefsInfo(const MediaPrefsInfo& other);

  ~MediaPrefsInfo();

  std::string ToString() const;

  // MSE Preferences
  // Intrinsic size of a video is default behavior of chromium.
  bool mse_use_intrinsic_size = true;

  // We set default value of this preference as true. Because if build flag
  // of av1 is enabled, we treat av1 codec as supported codec by default.
  // And if the relevant build flag is turned off, this preference is
  // meaningless.
  // Note that this preference changes behavior of
  //  - MediaSource.isTypeSupported().
  //  - navigator.mediaCapabilities.decodingInfo().
  //  - MSE video.
  //  - URL video with content type attribute.
  // Not changes behavior of
  //  - URL video without content type attribute.
  //    Because HTMLMediaElement tries to load anyway if no content mime type.
  bool is_av1_codec_enabled = true;
};

class MEDIA_EXPORT MediaPreferences {
 public:
  // static
  static MediaPreferences* Get();

  std::string GetRawMediaPreferences();

  // Platform dependent method.
  void Update(const std::string& media_preferences);

  bool UseIntrinsicSizeForMSE();

  void SetUseIntrinsicSizeForMSE(bool use_intrinsic_size);

  bool IsAV1CodecEnabled();

  void SetIsAV1CodecEnabled(bool is_av1_decoder_enabled);

  // Platform dependent method.
  void SetMediaCodecCapabilities(const std::string& capabilities);

  std::vector<MediaCodecCapability> GetMediaCodecCapabilities();

  // Find a capability for a given |type|. Note that this differs with
  // MediaPlatformAPI::GetMediaCodecCapabilityFor'Codec'().
  base::Optional<MediaCodecCapability> GetMediaCodecCapabilityForType(
      const std::string& type);

  // Platform dependent method.
  bool IsSupportedAudioType(const media::AudioType& type);

  // Platform dependent method.
  bool IsSupportedVideoType(const media::VideoType& type);

#if defined(USE_NEVA_WEBRTC)
  base::Optional<MediaCodecCapability> GetMediaCodecCapabilityForCodec(
      const std::string& codec);
#endif

 private:
  friend struct base::DefaultSingletonTraits<MediaPreferences>;
  MediaPreferences();
  ~MediaPreferences();

  std::string raw_preferences;

  MediaPrefsInfo media_prefs_info_;

  std::vector<MediaCodecCapability> capabilities_;

  // Used to access |media_prefs_info_|.
  base::Lock lock_;
};

}  // namespace media

#endif  // MEDIA_NEVA_MEDIA_PREFERENCES_H_
