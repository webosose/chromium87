// Copyright 2014-2020 LG Electronics, Inc.
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

#ifndef MEDIA_NEVA_WEBOS_SYSTEM_MEDIA_MANAGER_H_
#define MEDIA_NEVA_WEBOS_SYSTEM_MEDIA_MANAGER_H_

#include <string>

#include "base/memory/weak_ptr.h"
#include "media/neva/webos/webos_mediaclient.h"

namespace gfx {
class Rect;
}

namespace Json {
class Value;
}

#if UMS_INTERNAL_API_VERSION == 2
namespace ums {
struct audio_info_t;
struct video_info_t;
}  // namespace ums
using AudioInfoType = ums::audio_info_t;
using VideoInfoType = ums::video_info_t;
#else
namespace uMediaServer {
struct audio_info_t;
struct video_info_t;
}  // namespace uMediaServer
using AudioInfoType = uMediaServer::audio_info_t;
using VideoInfoType = uMediaServer::video_info_t;
#endif

namespace media {
class UMediaClientImpl;

// SystemMediaManager provide interface to platform media resource manager such
// as acb.
class SystemMediaManager {
 public:
  using ActiveRegionCB = base::Callback<void(const gfx::Rect&)>;

  enum class PlayState {
    kUnloaded,
    kLoaded,
    kPlaying,
    kPaused,
  };

  enum class AppState {
    kInit,
    kForeground,
    kBackground,
  };

  static std::unique_ptr<SystemMediaManager> Create(
      const base::WeakPtr<UMediaClientImpl>& umedia_client,
      const scoped_refptr<base::SingleThreadTaskRunner>& main_task_runner);

  virtual ~SystemMediaManager() {}

  virtual long Initialize(const bool is_video,
                          const std::string& app_id,
                          const ActiveRegionCB& active_region_cb) {
    return 0;
  }
  // |UMediaClientImpl| will call |UpdateMediaOption| before
  // |UMediaClientImpl| continue to Load
  virtual void UpdateMediaOption(const Json::Value& media_option) {}
  // Set video out position in screen space by using in_rect in video space
  virtual bool SetDisplayWindow(const gfx::Rect& out_rect,
                                const gfx::Rect& in_rect,
                                bool fullscreen) {
    return true;
  }
  // Set visibility of video
  virtual void SetVisibility(bool visible) {}
  // Get current visibility of video
  virtual bool GetVisibility() { return true; }
  // Set the media audio focus
  virtual void SetAudioFocus() {}
  // Test the media has audio focus.
  virtual bool GetAudioFocus() { return true; }
  // Switch to autolayout mode to prepare vtg
  virtual void SwitchToAutoLayout() {}
  // Notify |UMediaClientImpl| has updated audio info
  virtual void AudioInfoUpdated(const AudioInfoType& audio_info) {}
  // Notify |UMediaClientImpl| has updated video info
  virtual void VideoInfoUpdated(const VideoInfoType& videoInfo) {}
  virtual void SourceInfoUpdated(bool has_video, bool has_audio) {}
  // Notify app state is changed
  virtual void AppStateChanged(AppState s) {}
  // Notify play state is changed
  virtual void PlayStateChanged(PlayState s) {}
  // Notify audio mute is changed
  virtual void AudioMuteChanged(bool mute) {}
  // Send custom message to system media manager
  virtual bool SendCustomMessage(const std::string& message) { return true; }
  // Eof recived
  virtual void EofReceived() {}
};

}  // namespace media

#endif  // MEDIA_NEVA_WEBOS_SYSTEM_MEDIA_MANAGER_H_
