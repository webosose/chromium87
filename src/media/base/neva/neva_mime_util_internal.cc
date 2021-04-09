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

#include "media/base/neva/neva_mime_util_internal.h"

#include "base/strings/string_util.h"

namespace media {
namespace internal {

NevaMimeUtil::NevaMimeUtil() {
  // This internal function must be called after
  // MimeUtil::AddSupportedMediaFormats().
  InitializeMimeTypeMaps();
}

NevaMimeUtil::~NevaMimeUtil() {}

void NevaMimeUtil::InitializeMimeTypeMaps() {
  AddSupportedMediaFormats();
  RemoveUnsupportedMediaFormats();
}

void NevaMimeUtil::RemoveSupportedCodecFromContainer(
    const std::string& mime_type,
    const Codec& codec) {
  auto it_media_format_map =
      media_format_map_.find(base::ToLowerASCII(mime_type));

  if (it_media_format_map == media_format_map_.end())
    return;

  it_media_format_map->second.erase(codec);
}

void NevaMimeUtil::RemoveContainer(const std::string& mime_type) {
  media_format_map_.erase(base::ToLowerASCII(mime_type));
}

}  // namespace internal
}  // namespace media
