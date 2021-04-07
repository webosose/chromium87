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

#ifndef MEDIA_MOJO_MOJOM_NEVA_MEDIA_CODEC_CAPABILITY_MOJOM_TRAITS_H_
#define MEDIA_MOJO_MOJOM_NEVA_MEDIA_CODEC_CAPABILITY_MOJOM_TRAITS_H_

#include <stdint.h>

#include <string>

#include "media/base/ipc/media_param_traits.h"
#include "media/mojo/mojom/neva/media_codec_capability.mojom.h"
#include "media/neva/media_codec_capability.h"

namespace mojo {

template <>
struct StructTraits<media::mojom::MediaCodecCapabilityDataView,
                    media::MediaCodecCapability> {
  static std::string type(const media::MediaCodecCapability& input) {
    return input.type;
  }

  static std::string codec(const media::MediaCodecCapability& input) {
    return input.codec;
  }

  static int width(const media::MediaCodecCapability& input) {
    return input.width;
  }

  static int height(const media::MediaCodecCapability& input) {
    return input.height;
  }

  static int frame_rate(const media::MediaCodecCapability& input) {
    return input.frame_rate;
  }

  static int64_t bit_rate(const media::MediaCodecCapability& input) {
    return input.bit_rate;
  }

  static int channels(const media::MediaCodecCapability& input) {
    return input.channels;
  }

  static bool Read(media::mojom::MediaCodecCapabilityDataView input,
                   media::MediaCodecCapability* output);
};

}  // namespace mojo

#endif  // MEDIA_MOJO_MOJOM_NEVA_MEDIA_CODEC_CAPABILITY_MOJOM_TRAITS_H_
