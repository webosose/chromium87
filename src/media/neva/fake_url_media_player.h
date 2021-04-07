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

#ifndef MEDIA_NEVA_FAKE_URL_MEDIA_PLAYER_H_
#define MEDIA_NEVA_FAKE_URL_MEDIA_PLAYER_H_

#include <map>
#include <string>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/single_thread_task_runner.h"
#include "base/time/default_tick_clock.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "media/base/pipeline_status.h"
#include "media/base/time_delta_interpolator.h"
#include "media/neva/media_player_neva_interface.h"
#include "url/gurl.h"

namespace gfx {
class RectF;
}

namespace media {

class FakeURLMediaPlayer : public base::SupportsWeakPtr<FakeURLMediaPlayer>,
                           public media::MediaPlayerNeva {
 public:
  explicit FakeURLMediaPlayer(
      MediaPlayerNevaClient*,
      const scoped_refptr<base::SingleThreadTaskRunner>&,
      const std::string& app_id);

  FakeURLMediaPlayer(MediaPlayerNevaClient*);

  ~FakeURLMediaPlayer() override;

  void Initialize(const bool is_video,
                  const double current_time,
                  const std::string& url,
                  const std::string& mime,
                  const std::string& referrer,
                  const std::string& user_agent,
                  const std::string& cookies,
                  const std::string& media_option,
                  const std::string& custom_option) override;

  // Starts the player.
  void Start() override;

  // Pauses the player.
  void Pause() override;

  // Performs seek on the player.
  void Seek(const base::TimeDelta& time) override;

  void SetRate(double rate) override;

  // Sets the player volume.
  void SetVolume(double volume) override;

  // Sets the poster image.
  void SetPoster(const GURL& poster) override;

  void SetPreload(Preload preload) override;
  bool IsPreloadable(const std::string& content_media_option) override;
  bool HasVideo() override;
  bool HasAudio() override;
  bool SelectTrack(const MediaTrackType type, const std::string& id) override;
  void SwitchToAutoLayout() override;
  void SetDisplayWindow(const gfx::Rect&,
                        const gfx::Rect&,
                        bool fullScreen,
                        bool forced = false) override;
  bool UsesIntrinsicSize() const override;
  std::string MediaId() const override;
  bool HasAudioFocus() const override;
  void SetAudioFocus(bool focus) override;
  bool HasVisibility() const override;
  void SetVisibility(bool) override;
  void Suspend(SuspendReason reason) override;
  void Resume() override;
  bool RequireMediaResource() const override;
  bool IsRecoverableOnResume() const override;
  void SetDisableAudio(bool) override;
  // end of media::RendererMediaBuiltinPlayerManagerInterface
  //-----------------------------------------------------------------

 private:
  enum MediaState {
    Error,
    Loading,
    Ready,
    Paused,
    Seeking,
    Playing,
  };

  enum BufferingState {
    kHaveMetadata,
    kLoadCompleted,
    kPreloadCompleted,
    kPrerollCompleted,
    kBufferingStart,
    kBufferingEnd,
    kNetworkStateLoading,
    kNetworkStateLoaded
  };

  // Dummy callbacks for testing
  void OnPlaybackStateChanged(bool playing);
  void OnStreamEnded();
  void OnSeekDone(PipelineStatus status);
  void OnError(PipelineStatus error);
  void OnBufferingState(BufferingState buffering_state);
  void OnTimeUpdateTimerFired();

  base::TimeDelta GetCurrentTime();

  bool audio_focus_ = false;
  bool end_of_stream_ = false;
  bool is_suspended_ = false;
  bool pending_seek_ = false;
  bool visibility_ = true;

  double volume_ = 0;
  double playback_rate_ = 1.0f;
  double duration_ = 0;

  MediaPlayerNeva::Preload preload_ = PreloadNone;

  MediaPlayerNevaClient* client_ = nullptr;

  MediaState media_state_ = Loading;
  MediaState media_state_to_be_restored_;

  base::DefaultTickClock default_tick_clock_;
  base::RepeatingTimer time_update_timer_;

  TimeDeltaInterpolator interpolator_{&default_tick_clock_};

  scoped_refptr<base::SingleThreadTaskRunner> main_task_runner_;
  std::string app_id_;

  DISALLOW_COPY_AND_ASSIGN(FakeURLMediaPlayer);
};

}  // namespace media

#endif  // MEDIA_NEVA_FAKE_URL_MEDIA_PLAYER_H_
