// Copyright 2018-2020 LG Electronics, Inc.
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

#ifndef MEDIA_NEVA_WEBOS_WEBOS_MEDIACLIENT_H_
#define MEDIA_NEVA_WEBOS_WEBOS_MEDIACLIENT_H_

#include "base/callback.h"
#include "base/callback_forward.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/thread.h"
#include "base/time/time.h"
#include "media/base/pipeline.h"
#include "media/neva/media_constants.h"
#include "media/neva/media_track_info.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"

namespace media {

class WebOSMediaClient {
 public:
  enum class BufferingState {
    kHaveMetadata,
    kLoadCompleted,
    kPreloadCompleted,
    kPrerollCompleted,
    kWebOSBufferingStart,
    kWebOSBufferingEnd,
    kWebOSNetworkStateLoading,
    kWebOSNetworkStateLoaded
  };

  enum class Preload {
    kPreloadNone,
    kPreloadMetaData,
    kPreloadAuto,
  };

  class EventListener {
   public:
    virtual void OnPlaybackStateChanged(bool playing) = 0;
    virtual void OnPlaybackEnded() = 0;
    virtual void OnBufferingStatusChanged(BufferingState status) = 0;
    virtual void OnError(PipelineStatus error) = 0;
    virtual void OnDurationChanged() = 0;
    virtual void OnVideoSizeChanged() = 0;
    virtual void OnDisplayWindowChanged() = 0;
    virtual void OnAudioTrackAdded(
        const std::vector<MediaTrackInfo>& audio_track_info) = 0;
    virtual void OnVideoTrackAdded(const std::string& id,
                                   const std::string& kind,
                                   const std::string& language,
                                   bool enabled) = 0;
    virtual void OnUMSInfoUpdated(const std::string& detail) = 0;
    virtual void OnAudioFocusChanged() = 0;
    virtual void OnActiveRegionChanged(const gfx::Rect& active_region) = 0;
    virtual void OnWaitingForDecryptionKey() = 0;
    virtual void OnEncryptedMediaInitData(
        const std::string& init_data_type,
        const std::vector<uint8_t>& init_data) = 0;
    virtual void OnTimeUpdated(base::TimeDelta current_time) = 0;
  };

  virtual ~WebOSMediaClient() {}

  static std::unique_ptr<WebOSMediaClient> Create(
      const scoped_refptr<base::SingleThreadTaskRunner>& main_task_runner,
      base::WeakPtr<EventListener> event_listener,
      const std::string& app_id);

  virtual void Load(bool video,
                    double current_time,
                    bool is_local_source,
                    const std::string& url,
                    const std::string& mime_type,
                    const std::string& referrer,
                    const std::string& user_agent,
                    const std::string& cookies,
                    const std::string& payload) = 0;
  virtual void Seek(base::TimeDelta time,
                    const media::PipelineStatusCB& seek_cb) = 0;
  virtual float GetPlaybackRate() const = 0;
  virtual void SetPlaybackRate(float playback_rate) = 0;
  virtual double GetPlaybackVolume() const = 0;
  virtual void SetPlaybackVolume(double volume, bool forced = false) = 0;
  virtual bool SelectTrack(const MediaTrackType type,
                           const std::string& id) = 0;
  virtual void Suspend(SuspendReason reason) = 0;
  virtual void Resume() = 0;
  virtual bool IsRecoverableOnResume() = 0;
  virtual void SetPreload(Preload preload) = 0;
  virtual bool IsPreloadable(const std::string& content_media_option) = 0;
  virtual std::string MediaId() = 0;

  virtual double GetDuration() const = 0;
  virtual void SetDuration(double duration) = 0;
  virtual double GetCurrentTime() = 0;
  virtual void SetCurrentTime(double time) = 0;

  virtual media::Ranges<base::TimeDelta> GetBufferedTimeRanges() const = 0;
  virtual bool HasAudio() = 0;
  virtual bool HasVideo() = 0;
  virtual gfx::Size GetNaturalVideoSize() = 0;
  virtual void SetNaturalVideoSize(const gfx::Size& size) = 0;
  // outRect and inRect should not empty.
  virtual bool SetDisplayWindow(const gfx::Rect& outRect,
                                const gfx::Rect& inRect,
                                bool fullScreen,
                                bool forced = false) = 0;
  virtual void SetVisibility(bool visible) = 0;
  virtual bool Visibility() = 0;
  virtual void SetFocus() = 0;
  virtual bool Focus() = 0;
  virtual void SwitchToAutoLayout() = 0;
  virtual bool DidLoadingProgress() = 0;
  virtual bool UsesIntrinsicSize() const = 0;
  virtual void Unload() = 0;
  virtual bool IsSupportedBackwardTrickPlay() = 0;
  virtual bool IsSupportedPreload() = 0;
  virtual bool CheckUseMediaPlayerManager(const std::string& media_option) = 0;
  virtual void SetDisableAudio(bool) = 0;
  virtual void SetMediaLayerId(const std::string& media_layer_id) {}
};

}  // namespace media

#endif  // MEDIA_NEVA_WEBOS_WEBOS_MEDIACLIENT_H_
