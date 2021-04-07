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

struct MEDIA_EXPORT MediaCodecCapability {
  std::string type;
  std::string codec;
  int width = 0;
  int height = 0;
  int frame_rate = 0;
  int64_t bit_rate = 0;
  int channels = 0;

  MediaCodecCapability();

  MediaCodecCapability(const MediaCodecCapability& other);

  MediaCodecCapability(int width,
                       int height,
                       int frame_rate,
                       int bit_rate,
                       int channels);

  ~MediaCodecCapability();

  // Checks to see if the specified |capability| is supported.
  bool IsSatisfied(const MediaCodecCapability& capability) const;
};

}  // namespace media

#endif  // MEDIA_NEVA_MEDIA_CODEC_CAPABILITY_H_
