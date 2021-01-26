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

#include "media/neva/media_codec_capability.h"

#include <vector>

#include "media/neva/media_preferences.h"

namespace media {

/* The order of the Codec strings should be same as CodecInfo::HistogramTag */
std::vector<std::string> kTagCodecMap = {
    "",      // For CodecInfo::HistogramTag::HISTOGRAM_UNKNOWN
    "",      // For CodecInfo::HistogramTag::HISTOGRAM_VP8
    "VP9",   // For CodecInfo::HistogramTag::HISTOGRAM_VP9
    "",      // For CodecInfo::HistogramTag::HISTOGRAM_VORBIS
    "H264",  // For CodecInfo::HistogramTag::HISTOGRAM_H264
    "AAC",   // For CodecInfo::HistogramTag::HISTOGRAM_MPEG2AAC
    "AAC",   // For CodecInfo::HistogramTag::HISTOGRAM_MPEG4AAC
    "EAC3",  // For CodecInfo::HistogramTag::HISTOGRAM_EAC3
    "",      // For CodecInfo::HistogramTag::HISTOGRAM_MP3
    "OPUS",  // For CodecInfo::HistogramTag::HISTOGRAM_OPUS
    "",      // For CodecInfo::HistogramTag::HISTOGRAM_HEVC
    "AC3",   // For CodecInfo::HistogramTag::HISTOGRAM_AC3
    "",      // For CodecInfo::HistogramTag::HISTOGRAM_DOLBYVISION
    "",      // For CodecInfo::HistogramTag::HISTOGRAM_FLAC
    "AV1",   // For CodecInfo::HistogramTag::HISTOGRAM_AV1
    ""       // For CodecInfo::HistogramTag::HISTOGRAM_MPEG_H_AUDIO
};

MediaCodecCapability::MediaCodecCapability() = default;

MediaCodecCapability::MediaCodecCapability(const MediaCodecCapability& other) =
    default;

MediaCodecCapability::MediaCodecCapability(int width,
                                           int height,
                                           int frame_rate,
                                           int64_t bit_rate,
                                           int channels,
                                           const std::string& features)
    : width(width),
      height(height),
      frame_rate(frame_rate),
      bit_rate(bit_rate),
      channels(channels),
      features(features) {}

MediaCodecCapability::~MediaCodecCapability() = default;

// Checks to see if the specified |capability| is supported.
bool MediaCodecCapability::IsSatisfied(
    CodecInfo::Type type,
    CodecInfo::HistogramTag tag,
    const MediaCodecCapability& capability) const {
  if (width && width < capability.width)
    return false;
  if (height && height < capability.height)
    return false;
  if (frame_rate && frame_rate < capability.frame_rate)
    return false;
  if (bit_rate && bit_rate < capability.bit_rate)
    return false;
  if (channels && channels < capability.channels)
    return false;

  const std::string& codec_string = kTagCodecMap[tag];
  MediaCodecCapability codec_capability = capability;
  codec_capability.codec = codec_string;

  if (type == CodecInfo::Type::AUDIO) {
    if (tag == CodecInfo::HistogramTag::HISTOGRAM_VORBIS)
      return false;

    int channels = capability.channels;
    if (!codec_string.empty() && channels != -1 &&
        !MediaPreferences::Get()->IsSupportedAudioCodec(codec_capability)) {
      return false;
    }
  }

  if (type == CodecInfo::Type::VIDEO) {
    if (!codec_string.empty() &&
        !MediaPreferences::Get()->IsSupportedVideoCodec(codec_capability)) {
      return false;
    }
  }
  return true;
}

}  // namespace media
