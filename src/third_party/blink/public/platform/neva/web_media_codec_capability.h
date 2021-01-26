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

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_NEVA_WEB_MEDIA_CODEC_CAPABILITY_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_NEVA_WEB_MEDIA_CODEC_CAPABILITY_H_

#include "third_party/blink/public/platform/web_string.h"

namespace blink {

struct WebMediaCodecCapability {
  int width;
  int height;
  int frame_rate;
  int64_t bit_rate;
  int channels;
  WebString features;

  WebMediaCodecCapability(int width,
                          int height,
                          int frame_rate,
                          int64_t bit_rate,
                          int channels,
                          const WebString& features)
      : width(width),
        height(height),
        frame_rate(frame_rate),
        bit_rate(bit_rate),
        channels(channels),
        features(features) {}
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_PLATFORM_NEVA_WEB_MEDIA_CODEC_CAPABILITY_H_
