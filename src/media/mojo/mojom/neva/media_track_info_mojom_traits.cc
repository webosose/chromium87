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

#include "media/mojo/mojom/neva/media_track_info_mojom_traits.h"

namespace mojo {

// static
bool StructTraits<media::mojom::MediaTrackInfoDataView, media::MediaTrackInfo>::
    Read(media::mojom::MediaTrackInfoDataView input,
         media::MediaTrackInfo* output) {
  if (!input.ReadType(&output->type))
    return false;

  if (!input.ReadId(&output->id))
    return false;

  if (!input.ReadKind(&output->kind))
    return false;

  if (!input.ReadLanguage(&output->language))
    return false;

  output->enabled = input.enabled();

  return true;
}

}  // namespace mojo
