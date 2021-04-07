// Copyright 2019-2020 LG Electronics, Inc.
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

#ifndef MEDIA_NEVA_MEDIA_TRACK_INFO_H_
#define MEDIA_NEVA_MEDIA_TRACK_INFO_H_

#include <string>

#include "media/base/media_export.h"

namespace media {

enum class MediaTrackType { kAudio, kVideo, kMaxValue = kVideo };

struct MEDIA_EXPORT MediaTrackInfo {
 public:
  MediaTrackInfo();
  ~MediaTrackInfo();

  MediaTrackType type;
  std::string id;
  std::string kind;
  std::string language;
  bool enabled;
};

}  // namespace media

#endif  // MEDIA_NEVA_MEDIA_TRACK_INFO_H_
