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

#ifndef MEDIA_NEVA_MEDIA_PLAYER_NEVA_INTERFACE_H_
#define MEDIA_NEVA_MEDIA_PLAYER_NEVA_INTERFACE_H_

#include <string>

#include "base/time/time.h"
#include "media/base/ranges.h"
#include "media/neva/media_constants.h"
#include "media/neva/media_player_neva_types.h"
#include "media/neva/media_track_info.h"
#include "ui/gfx/geometry/rect_f.h"
#include "url/gurl.h"

namespace media {

static const int kTimeUpdateInterval = 100;

class MediaPlayerNevaClient {
 public:
  virtual void OnMediaMetadataChanged(base::TimeDelta duration,
                                      int width,
                                      int height,
                                      bool success) = 0;
  virtual void OnLoadComplete() = 0;
  virtual void OnPlaybackComplete() = 0;
  virtual void OnSeekComplete(const base::TimeDelta& current_time) = 0;
  virtual void OnMediaError(int error_type) = 0;
  virtual void OnVideoSizeChanged(int width, int height) = 0;
  virtual void OnMediaPlayerPlay() = 0;
  virtual void OnMediaPlayerPause() = 0;

  virtual void OnCustomMessage(const media::MediaEventType,
                               const std::string& detail) = 0;
  virtual void OnBufferingUpdate(int percentage) = 0;
  virtual void OnAudioTracksUpdated(
      const std::vector<MediaTrackInfo>& audio_track_info) = 0;

  // Called to update the current time.
  virtual void OnTimeUpdate(base::TimeDelta current_timestamp,
                            base::TimeTicks current_time_ticks) = 0;

  // Getters of playback state.
  // virtual bool paused() const = 0;

  // True if the loaded media has a playable video track.
  // virtual bool hasVideo() const = 0;

  // For video texture
  virtual void OnActiveRegionChanged(const gfx::Rect&) {}

  virtual void OnAudioFocusChanged() = 0;
};

class MediaPlayerNeva {
 public:
  enum Preload {
    PreloadNone,
    PreloadMetaData,
    PreloadAuto,
  };

  enum MediaError {
    MEDIA_ERROR_NONE,
    MEDIA_ERROR_FORMAT,
    MEDIA_ERROR_DECODE,
    MEDIA_ERROR_NOT_VALID_FOR_PROGRESSIVE_PLAYBACK,
    MEDIA_ERROR_INVALID_CODE,
  };

  virtual void Initialize(const bool is_video,
                          const double current_time,
                          const std::string& url,
                          const std::string& mime,
                          const std::string& referrer,
                          const std::string& user_agent,
                          const std::string& cookies,
                          const std::string& media_option,
                          const std::string& custom_option) = 0;

  // Starts the player.
  virtual void Start() = 0;
  // Pauses the player.
  virtual void Pause() = 0;
  // Performs seek on the player.
  virtual void Seek(const base::TimeDelta& time) = 0;
  // Sets the player volume.
  virtual void SetVolume(double volume) = 0;
  // Sets the poster image.
  virtual void SetPoster(const GURL& poster) = 0;

  virtual void SetRate(double rate) = 0;
  virtual void SetPreload(Preload preload) = 0;
  virtual bool IsPreloadable(const std::string& content_media_option) = 0;
  virtual bool HasVideo() = 0;
  virtual bool HasAudio() = 0;
  virtual bool SelectTrack(const MediaTrackType type,
                           const std::string& id) = 0;
  virtual void SwitchToAutoLayout() {}
  virtual void SetDisplayWindow(const gfx::Rect& out,
                                const gfx::Rect& in,
                                bool fullScreen,
                                bool forced = false) {}
  virtual bool UsesIntrinsicSize() const = 0;
  virtual std::string MediaId() const = 0;
  virtual void Suspend(SuspendReason reason) {}
  virtual void Resume() {}
  virtual bool HasAudioFocus() const = 0;
  virtual void SetAudioFocus(bool focus) = 0;
  virtual bool HasVisibility() const = 0;
  virtual void SetVisibility(bool) = 0;
  // Returns |true| if the player uses media resource(e.g. hardware decoder).
  virtual bool RequireMediaResource() const = 0;
  // Returns |true| if the player is recoverable on resume.
  virtual bool IsRecoverableOnResume() const = 0;
  // Disable audio decoding and out
  virtual void SetDisableAudio(bool) = 0;
  virtual void SetMediaLayerId(const std::string& media_layer_id) {}
  virtual media::Ranges<base::TimeDelta> GetBufferedTimeRanges() const;

  virtual ~MediaPlayerNeva() {}
};

}  // namespace media

#endif  // MEDIA_NEVA_MEDIA_PLAYER_NEVA_INTERFACE_H_
