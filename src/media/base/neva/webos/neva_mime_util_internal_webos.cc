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

#include "media/base/neva/neva_mime_util_internal.h"

#include "media/neva/media_preferences.h"

namespace media {
namespace internal {

void NevaMimeUtil::AddSupportedMediaFormats() {
  CodecSet webos_codecs;
  webos_codecs.insert(VALID_CODEC);

  // extended support type on tv platform
  AddContainerWithCodecs("audio/x-mpeg", webos_codecs);
  AddContainerWithCodecs("video/x-ms-wmv", webos_codecs);
  AddContainerWithCodecs("video/x-ms-asf", webos_codecs);
  AddContainerWithCodecs("video/x-m2ts", webos_codecs);
  AddContainerWithCodecs("video/m2ts", webos_codecs);
  // MPEG-2 TS.
  AddContainerWithCodecs("video/mp2t", webos_codecs);
  // hls
  AddContainerWithCodecs("application/vnd.apple.mpegurl", webos_codecs);
  AddContainerWithCodecs("application/vnd.apple.mpegurl.audio", webos_codecs);
  AddContainerWithCodecs("application/mpegurl", webos_codecs);
  AddContainerWithCodecs("application/x-mpegurl", webos_codecs);
  AddContainerWithCodecs("audio/mpegurl", webos_codecs);
  AddContainerWithCodecs("audio/x-mpegurl", webos_codecs);
  // mpeg-dash
  AddContainerWithCodecs("application/dash+xml", webos_codecs);
  // msiis
  AddContainerWithCodecs("application/vnd.ms-sstr+xml", webos_codecs);
#if defined(USE_GST_MEDIA)
  AddContainerWithCodecs("service/webos-camera", webos_codecs);
#endif
}

void NevaMimeUtil::RemoveUnsupportedMediaFormats() {
  if (!media::MediaPreferences::Get()->IsAV1CodecEnabled()) {
    RemoveSupportedCodecFromContainer("video/webm", AV1);
    RemoveSupportedCodecFromContainer("video/mp4", AV1);
  }
}

}  // namespace internal
}  // namespace media
