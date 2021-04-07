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

#ifndef NEVA_NEVA_MEDIA_SERVICE_CLIENTS_MOJO_MEDIA_PLAYER_H_
#define NEVA_NEVA_MEDIA_SERVICE_CLIENTS_MOJO_MEDIA_PLAYER_H_

#include <map>
#include <string>

#include "base/component_export.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/single_thread_task_runner.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "media/neva/media_player_neva_factory.h"
#include "media/neva/media_player_neva_interface.h"
#include "media/neva/webos/webos_mediaclient.h"
#include "mojo/public/cpp/bindings/associated_receiver.h"
#include "mojo/public/cpp/bindings/pending_associated_receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "neva/neva_media_service/public/mojom/constants.mojom.h"
#include "neva/neva_media_service/public/mojom/media_player.mojom.h"
#include "neva/neva_media_service/public/mojom/neva_media_service.mojom.h"
#include "third_party/blink/public/platform/web_string.h"
#include "url/gurl.h"

namespace gfx {
class RectF;
}

namespace neva_media {

class COMPONENT_EXPORT(NEVA_MEDIA_SERVICE) MojoMediaPlayer
    : public base::SupportsWeakPtr<MojoMediaPlayer>,
      public neva_media::mojom::MediaPlayerListener,
      public media::MediaPlayerNeva {
 public:
  explicit MojoMediaPlayer(
      mojo::PendingRemote<neva_media::mojom::MediaServiceProvider>
          pending_provider,
      media::MediaPlayerNevaClient*,
      media::MediaPlayerType,
      const scoped_refptr<base::SingleThreadTaskRunner>&,
      const std::string& app_id);
  ~MojoMediaPlayer() override;

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
  bool SelectTrack(const media::MediaTrackType type,
                   const std::string& id) override;
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
  void Suspend(media::SuspendReason reason) override;
  void Resume() override;
  bool RequireMediaResource() const override;
  bool IsRecoverableOnResume() const override;
  void SetDisableAudio(bool) override;
  void SetMediaLayerId(const std::string& media_layer_id) override;
  media::Ranges<base::TimeDelta> GetBufferedTimeRanges() const override;

  // neva_media::mojom::MediaPlayerListener
  void OnMediaPlayerPlay() override;
  void OnMediaPlayerPause() override;
  void OnPlaybackComplete() override;
  void OnMediaError(int error) override;
  void OnSeekComplete(base::TimeDelta current_time) override;
  void OnMediaMetadataChanged(base::TimeDelta duration,
                              uint32_t width,
                              uint32_t height,
                              bool success) override;
  void OnLoadComplete() override;
  void OnVideoSizeChanged(uint32_t width, uint32_t height) override;
  void OnCustomMessage(const media::MediaEventType,
                       const std::string& detail) override;
  void OnBufferingUpdate(int percentage) override;
  void OnTimeUpdate(base::TimeDelta current_timestamp,
                    base::TimeTicks current_time_ticks) override;
  void OnAudioTracksUpdated(
      const std::vector<media::MediaTrackInfo>& audio_track_info) override;
  void OnAudioFocusChanged() override;
  void OnActiveRegionChanged(const gfx::Rect&) override;
  // end of neva_media::mojom::MediaPlayerListener

 private:
  void OnConnected(
      mojo::PendingAssociatedReceiver<neva_media::mojom::MediaPlayerListener>
          receiver);

  media::MediaPlayerNevaClient* client_;
  base::TimeDelta paused_time_;

  gfx::Rect display_window_out_rect_;
  gfx::Rect display_window_in_rect_;
  gfx::Rect active_video_region_;

  base::RepeatingTimer time_update_timer_;

  scoped_refptr<base::SingleThreadTaskRunner> main_task_runner_;

  mojo::AssociatedReceiver<neva_media::mojom::MediaPlayerListener>
      client_receiver_;
  mojo::Remote<neva_media::mojom::MediaPlayer> media_player_;

  base::Optional<bool> has_video_;
  base::Optional<bool> has_audio_;
  base::Optional<bool> uses_intrinsic_size_;
  base::Optional<bool> is_recoverable_on_resume_;
  base::Optional<bool> has_audio_focus_;
  base::Optional<bool> has_visibility_;

  MojoMediaPlayer(const MojoMediaPlayer&) = delete;
  MojoMediaPlayer& operator=(const MojoMediaPlayer&) = delete;
};

}  // namespace neva_media

#endif  // NEVA_NEVA_MEDIA_SERVICE_CLIENTS_MOJO_MEDIA_PLAYER_H_
