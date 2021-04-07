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

#ifndef NEVA_NEVA_MEDIA_SERVICE_SERVICES_MOJO_MEDIA_PLATFORM_API_SERVICE_H_
#define NEVA_NEVA_MEDIA_SERVICE_SERVICES_MOJO_MEDIA_PLATFORM_API_SERVICE_H_

#include <map>
#include <string>
#include <vector>

#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/single_thread_task_runner.h"
#include "base/task/post_task.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "content/public/browser/browser_task_traits.h"
#include "media/base/pipeline_status.h"
#include "media/mojo/common/mojo_decoder_buffer_converter.h"
#include "media/neva/media_codec_capability.h"
#include "media/neva/media_constants.h"
#include "media/neva/media_platform_api.h"
#include "media/neva/media_track_info.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "neva/neva_media_service/public/mojom/media_platform_api.mojom.h"
#include "url/gurl.h"

namespace neva_media {

class MojoMediaPlatformAPIService : public mojom::MediaPlatformAPI {
 public:
  using ActiveRegionCB = base::Callback<void(const gfx::Rect&)>;
  using NaturalVideoSizeChangedCB = base::Callback<void(const gfx::Size&)>;
  using PlayerEventCB = base::Callback<void(media::PlayerEvent)>;

  MojoMediaPlatformAPIService(
      mojo::PendingRemote<neva_media::mojom::MediaPlatformAPIListener>
          media_platform_api_listener_remote,
      bool is_video,
      const std::string& app_id);
  ~MojoMediaPlatformAPIService() override;

  //-----------------------------------------------------------------
  // mojom::MediaPlatformAPI implementations
  void Initialize(
      const base::Optional<media::AudioDecoderConfig>& maybe_audio_config,
      const base::Optional<media::VideoDecoderConfig>& maybe_video_config,
      mojo::ScopedDataPipeConsumerHandle consumer_handle,
      InitializeCallback init_cb) override;

  // rect and in_rect should not be empty.
  void SetDisplayWindow(const gfx::Rect& rect,
                        const gfx::Rect& in_rect,
                        bool fullscreen) override;
  void Feed(media::mojom::DecoderBufferPtr buffer,
            media::FeedType type,
            FeedCallback callback) override;
  void Seek(base::TimeDelta time) override;
  void Suspend(media::SuspendReason reason) override;
  void Resume(base::TimeDelta paused_time,
              media::RestorePlaybackMode restore_playback_mode) override;
  void SetPlaybackRate(float playback_rate) override;
  void SetPlaybackVolume(double volume) override;
  void AllowedFeedVideo(AllowedFeedVideoCallback callback) override;
  void AllowedFeedAudio(AllowedFeedAudioCallback callback) override;
  void Finalize() override;
  void SetKeySystem(const std::string& key_system) override;
  void IsEOSReceived(IsEOSReceivedCallback callback) override;
  void UpdateVideoConfig(
      const media::VideoDecoderConfig& video_config) override;

  void SetVisibility(bool visible) override;

  void SwitchToAutoLayout() override;
  void SetDisableAudio(bool disable) override;
  void HaveEnoughData(HaveEnoughDataCallback callback) override;
  void SetMediaLayerId(const std::string& media_layer_id) override;
  void SetMediaPreferences(const std::string& preferences) override;
  void SetMediaCodecCapabilities(
      const std::vector<media::MediaCodecCapability>& capabilities) override;

  // Caller functions of MediaPlatformAPIListener
  void OnNaturalVideoSizeChanged(const gfx::Size& size);
  void OnResumeDone();
  void OnSuspendDone();
  void OnActiveRegionChanged(const gfx::Rect& rect);
  void OnPipelineStatusChanged(media::PipelineStatus status);
  void OnPlayerEvent(media::PlayerEvent event);
  void OnUpdateCurrentTime(const base::TimeDelta& current_time);
  // End of Caller functions of MediaPlatformAPIListener

  void OnInitialized(media::PipelineStatus status);
  void OnFeedReady(FeedCallback callback,
                   media::FeedType type,
                   scoped_refptr<media::DecoderBuffer> buffer);

 private:
  PlayerEventCB player_event_cb_;
  media::StatisticsCB statistics_cb_;
  InitializeCallback init_cb_;

  mojo::Remote<neva_media::mojom::MediaPlatformAPIListener>
      media_platform_api_listener_remote_;
  scoped_refptr<media::MediaPlatformAPI> media_platform_api_;
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;

  // Helper for reading DecoderBuffer data from the DataPipe.
  std::unique_ptr<media::MojoDecoderBufferReader> mojo_decoder_buffer_reader_;

  base::WeakPtrFactory<MojoMediaPlatformAPIService> weak_factory_{this};
  MojoMediaPlatformAPIService(const MojoMediaPlatformAPIService&) = delete;
  MojoMediaPlatformAPIService& operator=(const MojoMediaPlatformAPIService&) =
      delete;
};

}  // namespace neva_media

#endif  // NEVA_NEVA_MEDIA_SERVICE_SERVICES_MOJO_MEDIA_PLATFORM_API_SERVICE_H_
