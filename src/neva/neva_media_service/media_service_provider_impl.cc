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

#include "neva/neva_media_service/media_service_provider_impl.h"

#include "mojo/public/cpp/bindings/self_owned_receiver.h"
#include "neva/neva_media_service/services/mojo_media_platform_api_service.h"
#include "neva/neva_media_service/services/mojo_media_player_service.h"

namespace neva_media {

MediaServiceProvider::MediaServiceProvider() {}
MediaServiceProvider::~MediaServiceProvider() {}

void MediaServiceProvider::CreateMediaPlayer(
    media::MediaPlayerType media_player_type,
    const std::string& app_id,
    mojo::PendingReceiver<mojom::MediaPlayer> request) {
  mojo::MakeSelfOwnedReceiver(
      std::make_unique<MojoMediaPlayerService>(media_player_type, app_id),
      std::move(request));
}

void MediaServiceProvider::CreateMediaPlatformAPI(
    mojo::PendingRemote<mojom::MediaPlatformAPIListener>
        media_platform_api_listener,
    bool is_video,
    const std::string& app_id,
    mojo::PendingReceiver<mojom::MediaPlatformAPI> request) {
  mojo::MakeSelfOwnedReceiver(
      std::make_unique<MojoMediaPlatformAPIService>(
          std::move(media_platform_api_listener), is_video, app_id),
      std::move(request));
}

}  // namespace neva_media
