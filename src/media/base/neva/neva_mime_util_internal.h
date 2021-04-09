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

#ifndef MEDIA_BASE_NEVA_NEVA_MIME_UTIL_INTERNAL_H_
#define MEDIA_BASE_NEVA_NEVA_MIME_UTIL_INTERNAL_H_

#include "media/base/mime_util_internal.h"

namespace media {
namespace internal {

class MEDIA_EXPORT NevaMimeUtil : public MimeUtil {
 public:
  NevaMimeUtil();
  ~NevaMimeUtil();

 private:
  typedef base::flat_set<int> CodecSet;
  void InitializeMimeTypeMaps();
  void AddSupportedMediaFormats();
  // Used for removing default supported media formats.
  void RemoveUnsupportedMediaFormats();
  void RemoveSupportedCodecFromContainer(const std::string& mime_type,
                                         const Codec& codec);
  // Removes |mime_type| from |media_format_map_| if not supported by the
  // platform.
  void RemoveContainer(const std::string& mime_type);
};

}  // namespace internal
}  // namespace media

#endif  // MEDIA_BASE_NEVA_NEVA_MIME_UTIL_INTERNAL_H_
