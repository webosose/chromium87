// Copyright (c) 2018 LG Electronics, Inc.
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

#include "media/base/neva/neva_mime_util_internal.h"

namespace media {
namespace internal {

void NevaMimeUtil::AddSupportedMediaFormats() {
  CodecSet webos_codecs;
  webos_codecs.insert(VALID_CODEC);

  AddContainerWithCodecs("application/vnd.apple.mpegurl", webos_codecs);
  AddContainerWithCodecs("application/mpegurl", webos_codecs);
  AddContainerWithCodecs("application/x-mpegurl", webos_codecs);
  AddContainerWithCodecs("audio/mpegurl", webos_codecs);
  AddContainerWithCodecs("audio/x-mpegurl", webos_codecs);

  // webOS specific media types
  AddContainerWithCodecs("service/webos-camera", webos_codecs);
  AddContainerWithCodecs("service/webos-photo-camera", webos_codecs);
}

void NevaMimeUtil::RemoveUnsupportedMediaFormats() {
  RemoveContainer("audio/webm");
  RemoveContainer("video/webm");
  RemoveContainer("video/ogg");
#if BUILDFLAG(USE_PROPRIETARY_CODECS)
  RemoveContainer("audio/aac");
  RemoveContainer("audio/x-m4a");
#endif  // BUILDFLAG(USE_PROPRIETARY_CODECS)
}

} // namespace internal
} // namespace media