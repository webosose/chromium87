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

#include "media/mojo/mojom/neva/media_types_neva.mojom.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "mojo/public/cpp/bindings/interface_ptr_set.h"
#include "neva/neva_media_service/public/mojom/neva_media_service.mojom.h"

#ifndef NEVA_NEVA_MEDIA_SERVICE_MEDIA_SERVICE_PROVIDER_IMPL_H_
#define NEVA_NEVA_MEDIA_SERVICE_MEDIA_SERVICE_PROVIDER_IMPL_H_

namespace neva_media {

class MediaServiceProvider : public mojom::MediaServiceProvider {
 public:
  MediaServiceProvider();
  ~MediaServiceProvider() override;

  void AddBinding(mojom::MediaServiceProviderRequest request) {
    bindings_.AddBinding(this, std::move(request));
  }

  void CreateMediaPlayer(
      media::MediaPlayerType media_player_type,
      const std::string& app_id,
      mojo::PendingReceiver<mojom::MediaPlayer> request) override;

  void CreateMediaPlatformAPI(
      mojo::PendingRemote<mojom::MediaPlatformAPIListener>
          media_platform_api_listener,
      bool is_video,
      const std::string& app_id,
      mojo::PendingReceiver<mojom::MediaPlatformAPI> request) override;

 private:
  mojo::BindingSet<mojom::MediaServiceProvider> bindings_;
  MediaServiceProvider(const MediaServiceProvider&) = delete;
  MediaServiceProvider& operator=(const MediaServiceProvider&) = delete;
};

}  // namespace neva_media

#endif  // NEVA_NEVA_MEDIA_SERVICE_MEDIA_SERVICE_PROVIDER_IMPL_H_
