// Copyright 2017-2020 LG Electronics, Inc.
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

#include "media/neva/media_player_neva_factory.h"

namespace media {

media::MediaPlayerType MediaPlayerNevaFactory::GetMediaPlayerType(
    const std::string& mime_type) {
  return media::MediaPlayerType::kMediaPlayerTypeNone;
}

MediaPlayerNeva* MediaPlayerNevaFactory::CreateMediaPlayerNeva(
    MediaPlayerNevaClient* client,
    const media::MediaPlayerType media_player_type,
    const scoped_refptr<base::SingleThreadTaskRunner>& main_task_runner,
    const std::string& app_id) {
  return nullptr;
}

}  // namespace media
