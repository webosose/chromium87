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

#ifndef MEDIA_MOJO_MOJOM_NEVA_MEDIA_TRACK_INFO_MOJOM_TRAITS_H_
#define MEDIA_MOJO_MOJOM_NEVA_MEDIA_TRACK_INFO_MOJOM_TRAITS_H_

#include <string>

#include "media/base/ipc/media_param_traits.h"
#include "media/mojo/mojom/neva/media_track_info.mojom.h"
#include "media/neva/media_track_info.h"

namespace mojo {

template <>
struct StructTraits<media::mojom::MediaTrackInfoDataView,
                    media::MediaTrackInfo> {
  static media::MediaTrackType type(const media::MediaTrackInfo& input) {
    return input.type;
  }

  static const std::string& id(const media::MediaTrackInfo& input) {
    return input.id;
  }

  static const std::string& kind(const media::MediaTrackInfo& input) {
    return input.kind;
  }

  static const std::string& language(const media::MediaTrackInfo& input) {
    return input.language;
  }

  static bool enabled(const media::MediaTrackInfo& input) {
    return input.enabled;
  }

  static bool Read(media::mojom::MediaTrackInfoDataView input,
                   media::MediaTrackInfo* output);
};

}  // namespace mojo

#endif  // MEDIA_MOJO_MOJOM_NEVA_MEDIA_TRACK_INFO_MOJOM_TRAITS_H_
