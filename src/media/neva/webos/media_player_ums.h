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

#ifndef MEDIA_NEVA_WEBOS_MEDIA_PLAYER_UMS_H_
#define MEDIA_NEVA_WEBOS_MEDIA_PLAYER_UMS_H_

#include <map>
#include <string>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/single_thread_task_runner.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "media/neva/media_player_neva_interface.h"
#include "media/neva/webos/webos_mediaclient.h"
#include "url/gurl.h"

namespace blink {
class WebFrame;
}

namespace gfx {
class RectF;
}

namespace media {

class MediaPlayerUMS : public base::SupportsWeakPtr<MediaPlayerUMS>,
                       public WebOSMediaClient::EventListener,
                       public media::MediaPlayerNeva {
 public:
  // Constructs a RendererMediaPlayerManager object for the |render_frame|.
  explicit MediaPlayerUMS(MediaPlayerNevaClient*,
                          const scoped_refptr<base::SingleThreadTaskRunner>&,
                          const std::string& app_id);
  ~MediaPlayerUMS() override;

  // media::RendererMediaBuiltinPlayerManagerInterface implementation
  // Initializes a MediaPlayerAndroid object in browser process.
  void Initialize(const bool is_video,
                  const double current_time,
                  const std::string& url,
                  const std::string& mime,
                  const std::string& referrer,
                  const std::string& user_agent,
                  const std::string& cookies,
                  const std::string& media_option,
                  const std::string& custon_option) override;

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

  // bool IsSupportedBackwardTrickPlay() override;
  void SetPreload(
      Preload preload) override;  // TODO(wanchang): fix the type of preload
  bool IsPreloadable(const std::string& content_media_option) override;
  bool HasVideo() override;
  bool HasAudio() override;
  bool SelectTrack(const MediaTrackType type, const std::string& id) override;
  // gfx::Size NaturalVideoSize() override;
  // double Duration() override;
  // double CurrentTime() override;
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
  void SetMediaLayerId(const std::string& media_layer_id) override;
  media::Ranges<base::TimeDelta> GetBufferedTimeRanges() const override;
  bool Send(const std::string& message) const override;
  // end of media::RendererMediaBuiltinPlayerManagerInterface
  //-----------------------------------------------------------------

  // Implement WebOSMediaClient::EventListener
  void OnPlaybackStateChanged(bool playing) override;
  void OnPlaybackEnded() override;
  void OnBufferingStatusChanged(
      WebOSMediaClient::BufferingState buffering_state) override;
  void OnError(PipelineStatus error) override;
  void OnDurationChanged() override;
  void OnVideoSizeChanged() override;
  void OnDisplayWindowChanged() override;
  void OnAudioTrackAdded(
      const std::vector<MediaTrackInfo>& audio_track_info) override;
  void OnVideoTrackAdded(const std::string& id,
                         const std::string& kind,
                         const std::string& language,
                         bool enabled) override;
  void OnUMSInfoUpdated(const std::string& detail) override;
  void OnAudioFocusChanged() override;
  void OnActiveRegionChanged(const gfx::Rect& active_region) override;
  void OnWaitingForDecryptionKey() override;
  void OnEncryptedMediaInitData(const std::string& init_data_type,
                                const std::vector<uint8_t>& init_data) override;
  void OnTimeUpdated(base::TimeDelta current_time) override;
  // End of implement WebOSMediaClient::EventListener

 private:
  void OnSeekDone(PipelineStatus status);

  std::unique_ptr<WebOSMediaClient> umedia_client_;
  MediaPlayerNevaClient* client_ = nullptr;
  bool paused_ = true;
  base::TimeDelta paused_time_;
  double playback_rate_ = 1.0f;
  bool is_suspended_ = false;

  bool fullscreen_ = false;
  gfx::Rect display_window_out_rect_;
  gfx::Rect display_window_in_rect_;
  gfx::Rect active_video_region_;
  bool active_video_region_changed_ = false;

  bool is_video_offscreen_ = false;

  base::TimeDelta current_time_;

  scoped_refptr<base::SingleThreadTaskRunner> main_task_runner_;
  std::string app_id_;

  DISALLOW_COPY_AND_ASSIGN(MediaPlayerUMS);
};

}  // namespace media

#endif  // MEDIA_NEVA_WEBOS_MEDIA_PLAYER_UMS_H_
