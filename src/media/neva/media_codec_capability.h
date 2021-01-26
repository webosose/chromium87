// Copyright 2018 LG Electronics, Inc.
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

#ifndef MEDIA_NEVA_MEDIA_CODEC_CAPABILITY_H_
#define MEDIA_NEVA_MEDIA_CODEC_CAPABILITY_H_

#include <stdint.h>

#include <string>

#include "media/base/media_export.h"

namespace media {

class MediaLog;

// Below types, CodecIDValidatorFunction and CodecInfo are upstream types
// moved to here from stream_parser_factory.cc file
typedef bool (*CodecIDValidatorFunction)(const std::string& codecs_id,
                                         MediaLog* media_log);

struct CodecInfo {
  enum Type { UNKNOWN, AUDIO, VIDEO };

  // Update tools/metrics/histograms/histograms.xml if new values are added.
  enum HistogramTag {
    HISTOGRAM_UNKNOWN,
    HISTOGRAM_VP8,
    HISTOGRAM_VP9,
    HISTOGRAM_VORBIS,
    HISTOGRAM_H264,
    HISTOGRAM_MPEG2AAC,
    HISTOGRAM_MPEG4AAC,
    HISTOGRAM_EAC3,
    HISTOGRAM_MP3,
    HISTOGRAM_OPUS,
    HISTOGRAM_HEVC,
    HISTOGRAM_AC3,
    HISTOGRAM_DOLBYVISION,
    HISTOGRAM_FLAC,
    HISTOGRAM_AV1,
    HISTOGRAM_MPEG_H_AUDIO,
    HISTOGRAM_MAX =
        HISTOGRAM_MPEG_H_AUDIO  // Must be equal to largest logged entry.
  };

  const char* pattern;
  Type type;
  CodecIDValidatorFunction validator;
  HistogramTag tag;
};

struct MEDIA_EXPORT MediaCodecCapability {
  std::string type;
  std::string codec;
  int width = 0;
  int height = 0;
  int frame_rate = 0;
  int64_t bit_rate = 0;
  int channels = 0;
  std::string features;

  MediaCodecCapability();

  MediaCodecCapability(const MediaCodecCapability& other);

  MediaCodecCapability(int width,
                       int height,
                       int frame_rate,
                       int64_t bit_rate,
                       int channels,
                       const std::string& features);

  ~MediaCodecCapability();

  // Checks to see if the specified |capability| is supported.
  bool IsSatisfied(CodecInfo::Type type,
                   CodecInfo::HistogramTag tag,
                   const MediaCodecCapability& capability) const;
};

}  // namespace media

#endif  // MEDIA_NEVA_MEDIA_CODEC_CAPABILITY_H_
