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
#include "third_party/jsoncpp/source/include/json/json.h"

namespace media {

void MediaPreferences::Update(const std::string& media_preferences) {
  base::AutoLock auto_lock(lock_);

  if (raw_preferences == media_preferences)
    return;
  raw_preferences = media_preferences;

  // Format:
  // "mediaExtension":{"mse":{"enableAV1":true,
  //                          "disableVideoIntrinsicSize":true,
  //                          "maxAudioSourceBuffer":15,
  //                          "maxVideoSourceBuffer":100},
  //                   "ums":{"fixedAspectRatio":true }}
  Json::Value preferences;
  Json::Reader reader;

  if (!reader.parse(media_preferences, preferences))
    return;

  // MSE Preferences
  if (preferences.isMember("mse")) {
    if (preferences["mse"].isMember("disableVideoIntrinsicSize")) {
      media_prefs_info_.mse_use_intrinsic_size =
          !preferences["mse"]["disableVideoIntrinsicSize"].asBool();
    }
  }

  if (preferences.isMember("mse") && preferences["mse"].isMember("enableAV1")) {
    media_prefs_info_.is_av1_codec_enabled =
        preferences["mse"]["enableAV1"].asBool();
  }

  if(preferences.isMember("supportDolbyHDR")) {
    media_prefs_info_.is_supported_dolby_hdr =
        preferences["supportDolbyHDR"].asBool();
  }

  if(preferences.isMember("supportDolbyAtmos")) {
    media_prefs_info_.is_supported_dolby_atmos =
        preferences["supportDolbyAtmos"].asBool();
  }

  VLOG(1) << __func__ << " info: " << media_prefs_info_.ToString();
}

void MediaPreferences::SetMediaCodecCapabilities(
    const std::string& capabilities) {
  base::AutoLock auto_lock(lock_);

  Json::Value codec_capability;
  Json::Reader reader;

  if (!reader.parse(capabilities, codec_capability))
    return;

  Json::Value video_codecs = codec_capability["videoCodecs"];
  for (Json::Value::iterator iter = video_codecs.begin();
       iter != video_codecs.end(); iter++) {
    if (!(*iter).isObject())
      continue;

    std::string type, codec;

    if ((*iter)["name"].asString() == "H.264") {
      type = "video/mp4";
      codec = "H264";
    }
    else if ((*iter)["name"].asString() == "H.265") {
      codec = "H265";
    }
    else if ((*iter)["name"].asString() == "VP8") {
      type = "video/vp8";
      codec = "VP8";
    }
    else if ((*iter)["name"].asString() == "VP9") {
      type = "video/vp9";
      codec = "VP9";
    }
    else if ((*iter)["name"].asString() == "AV1") {
      codec = "AV1";
    } else {
      continue;
    }

    MediaCodecCapability capability;
    capability.type = type;
    capability.codec = codec;
    capability.width = (*iter)["maxWidth"].asInt();
    capability.height = (*iter)["maxHeight"].asInt();
    capability.frame_rate = (*iter)["maxFrameRate"].asInt();
    capability.bit_rate = (*iter)["maxBitRate"].asInt() * 1024 * 1024;
    capability.channels = (*iter)["channels"].asInt();

    auto it = std::find_if(
        capabilities_.begin(), capabilities_.end(),
        [&type, &codec](const MediaCodecCapability& capability) {
          return type == capability.type && codec == capability.codec;
        });

    if (it != capabilities_.end()) {
      *it = capability;
      continue;
    }
    capabilities_.push_back(capability);
  }

  Json::Value audio_codecs = codec_capability["audioCodecs"];
  for (Json::Value::iterator iter = audio_codecs.begin();
       iter != audio_codecs.end(); iter++) {
    if (!(*iter).isObject())
      continue;

    std::string type, codec;

    if ((*iter)["name"].asString() == "AAC") {
      type = "audio/mp4";
      codec = "AAC";
    } else {
      continue;
    }

    MediaCodecCapability capability;
    capability.type = type;
    capability.codec = codec;
    capability.width = (*iter)["maxWidth"].asInt();
    capability.height = (*iter)["maxHeight"].asInt();
    capability.frame_rate = (*iter)["maxFrameRate"].asInt();
    capability.bit_rate = (*iter)["maxBitRate"].asInt() * 1024 * 1024;
    capability.channels = (*iter)["channels"].asInt();

    auto it = std::find_if(
        capabilities_.begin(), capabilities_.end(),
        [&type, &codec](const MediaCodecCapability& capability) {
          return type == capability.type && codec == capability.codec;
        });

    if (it != capabilities_.end()) {
      *it = capability;
      continue;
    }
    capabilities_.push_back(capability);
  }
}

bool MediaPreferences::IsSupportedAudioType(const media::AudioType& type) {
  // Defer to media's default support.
  return media::IsDefaultSupportedAudioType(type);
}

bool MediaPreferences::IsSupportedVideoType(const media::VideoType& type) {
  // Defer to media's default support.
  return media::IsDefaultSupportedVideoType(type);
}

bool MediaPreferences::IsSupportedVideoCodec(
    const MediaCodecCapability& capability) {
  return true;
}

bool MediaPreferences::IsSupportedAudioCodec(
    const MediaCodecCapability& capability) {
  return true;
}

bool MediaPreferences::IsSupportedUHD() {
  return is_supported_uhd.has_value() ? is_supported_uhd.value() : false;
}

}  // namespace media
