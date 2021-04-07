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

namespace media {

MediaCodecCapability::MediaCodecCapability() = default;

MediaCodecCapability::MediaCodecCapability(const MediaCodecCapability& other) =
    default;

MediaCodecCapability::MediaCodecCapability(int width,
                                           int height,
                                           int frame_rate,
                                           int bit_rate,
                                           int channels)
    : width(width),
      height(height),
      frame_rate(frame_rate),
      bit_rate(bit_rate),
      channels(channels) {}

MediaCodecCapability::~MediaCodecCapability() = default;

// Checks to see if the specified |capability| is supported.
bool MediaCodecCapability::IsSatisfied(
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
  return true;
}

}  // namespace media
