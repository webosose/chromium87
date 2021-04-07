// Copyright 2019 LG Electronics, Inc.
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

#include "neva/neva_media_service/neva_media_manifest.h"

#include <set>

#include "base/no_destructor.h"
#include "neva/neva_media_service/public/mojom/constants.mojom.h"
#include "services/service_manager/public/cpp/manifest_builder.h"

namespace neva_media {

const service_manager::Manifest& GetNevaMediaManifest() {
  static base::NoDestructor<service_manager::Manifest> manifest{
      service_manager::ManifestBuilder()
          .WithServiceName(neva_media::mojom::kServiceName)
          .WithDisplayName("NevaMedia (NEVA_MEDIA process)")
          .ExposeCapability("neva:media_service",
                            std::set<const char*>{
                                "neva_media.mojom.MediaServiceProvider",
                            })
          .Build()};
  return *manifest;
}

}  // namespace neva_media
