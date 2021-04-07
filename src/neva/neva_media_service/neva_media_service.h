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

#ifndef NEVA_NEVA_MEDIA_SERVICE_NEVA_MEDIA_SERVICE_H_
#define NEVA_NEVA_MEDIA_SERVICE_NEVA_MEDIA_SERVICE_H_

#include "base/component_export.h"
#include "neva/neva_media_service/public/mojom/neva_media_service.mojom.h"

namespace neva_media {

COMPONENT_EXPORT(NEVA_MEDIA_SERVICE)
neva_media::mojom::NevaMediaService& GetNevaMediaService();

}  // namespace neva_media

#endif  // NEVA_NEVA_MEDIA_SERVICE_NEVA_MEDIA_SERVICE_H_
