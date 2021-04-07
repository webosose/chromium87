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

#ifndef NEVA_NEVA_MEDIA_SERVICE_CLIENTS_MOJO_MEDIA_PLATFORM_API_H_
#define NEVA_NEVA_MEDIA_SERVICE_CLIENTS_MOJO_MEDIA_PLATFORM_API_H_

#include "base/component_export.h"
#include "base/memory/weak_ptr.h"
#include "media/mojo/common/mojo_decoder_buffer_converter.h"
#include "media/mojo/mojom/media_types.mojom.h"
#include "media/mojo/mojom/video_decoder.mojom.h"
#include "media/neva/media_platform_api.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "neva/neva_media_service/public/mojom/constants.mojom.h"
#include "neva/neva_media_service/public/mojom/media_platform_api.mojom.h"
#include "neva/neva_media_service/public/mojom/neva_media_service.mojom.h"

namespace neva_media {

class COMPONENT_EXPORT(NEVA_MEDIA_SERVICE) MojoMediaPlatformAPI
    : public media::MediaPlatformAPI,
      public mojom::MediaPlatformAPIListener {
 public:
  static scoped_refptr<media::MediaPlatformAPI> Create(
      const scoped_refptr<base::SingleThreadTaskRunner>& media_task_runner,
      bool video,
      const std::string& app_id,
      const media::MediaPlatformAPI::NaturalVideoSizeChangedCB&
          natural_video_size_changed_cb,
      const base::Closure& resume_done_cb,
      const base::Closure& suspend_done_cb,
      const media::MediaPlatformAPI::ActiveRegionCB& active_region_cb,
      const media::PipelineStatusCB& error_cb,
      mojo::PendingRemote<mojom::MediaServiceProvider> pending_provider);

  MojoMediaPlatformAPI(
      const scoped_refptr<base::SingleThreadTaskRunner>& media_task_runner,
      bool is_video,
      const std::string& app_id,
      const media::MediaPlatformAPI::NaturalVideoSizeChangedCB&
          natural_video_size_changed_cb,
      const base::Closure& resume_done_cb,
      const base::Closure& suspend_done_cb,
      const media::MediaPlatformAPI::ActiveRegionCB& active_region_cb,
      const media::PipelineStatusCB& error_cb,
      mojo::PendingRemote<mojom::MediaServiceProvider> pending_provider);

  // Implementation of MediaPlatformAPI
  void Initialize(const media::AudioDecoderConfig& audio_config,
                  const media::VideoDecoderConfig& video_config,
                  const media::PipelineStatusCB& init_cb) override;

  // rect and in_rect should not be empty.
  void SetDisplayWindow(const gfx::Rect& rect,
                        const gfx::Rect& in_rect,
                        bool fullscreen) override;
  bool Feed(const scoped_refptr<media::DecoderBuffer>& buffer,
            media::FeedType type) override;
  void Seek(base::TimeDelta time) override;
  void Suspend(media::SuspendReason reason) override;
  void Resume(base::TimeDelta paused_time,
              media::RestorePlaybackMode restore_playback_mode) override;
  void SetPlaybackRate(float playback_rate) override;
  void SetPlaybackVolume(double volume) override;
  bool AllowedFeedVideo() override;
  bool AllowedFeedAudio() override;
  void Finalize() override;
  void SetKeySystem(const std::string& key_system) override;
  bool IsEOSReceived() override;
  void UpdateVideoConfig(
      const media::VideoDecoderConfig& video_config) override;

  void SetVisibility(bool visible) override;

  void SwitchToAutoLayout() override;
  void SetDisableAudio(bool disable) override;

  bool HaveEnoughData() override;

  void SetPlayerEventCb(
      const media::MediaPlatformAPI::PlayerEventCB& callback) override;
  void SetStatisticsCb(const media::StatisticsCB& cb) override;

  base::TimeDelta GetCurrentTime() override;
  void SetMediaLayerId(const std::string& media_layer_id) override;
  void SetMediaPreferences(const std::string& preferences) override;
  void SetMediaCodecCapabilities(
      const std::vector<media::MediaCodecCapability>& capabilities) override;
  // End of implementation of MediaPlatformAPI

  // Implementation of MediaPlatformAPIListener
  void OnNaturalVideoSizeChanged(const gfx::Size& size) override;
  void OnResumeDone() override;
  void OnSuspendDone() override;
  void OnActiveRegionChanged(const gfx::Rect& rect) override;
  void OnPipelineStatusChanged(media::PipelineStatus status) override;
  void OnPlayerEvent(media::PlayerEvent event) override;
  void OnStatistics(const media::PipelineStatistics& statistics) override;
  void OnUpdateCurrentTime(base::TimeDelta current_time) override;
  // End of implementation of MediaPlatformAPIListener

  void FinalizeMediaPlatformAPI() override;

 private:
  ~MojoMediaPlatformAPI() override;
  friend class base::RefCountedThreadSafe<MojoMediaPlatformAPI>;

  void OnCreate();
  void OnInitialized(media::PipelineStatus status);

  bool is_video_;
  std::string app_id_;
  mojo::PendingRemote<mojom::MediaServiceProvider> pending_provider_;
  const scoped_refptr<base::SingleThreadTaskRunner> media_task_runner_;
  std::atomic<base::TimeDelta> client_current_time_ = {base::TimeDelta()};

  media::PipelineStatusCB init_cb_;

  base::Closure resume_done_cb_;
  base::Closure suspend_done_cb_;
  media::MediaPlatformAPI::ActiveRegionCB active_region_cb_;
  media::MediaPlatformAPI::NaturalVideoSizeChangedCB
      natural_video_size_changed_cb_;
  media::PipelineStatusCB error_cb_;

  media::MediaPlatformAPI::PlayerEventCB player_event_cb_;
  media::StatisticsCB statistics_cb_;

  std::string identifier_;
  mojom::MediaPlatformAPIPtr media_platform_api_;
  std::unique_ptr<media::MojoDecoderBufferWriter> mojo_decoder_buffer_writer_;

  mojo::Receiver<mojom::MediaPlatformAPIListener>
      media_platform_api_listener_receiver_;

  // For posting tasks from main thread to |media_task_runner_|.
  base::WeakPtr<MojoMediaPlatformAPI> weak_this_;
  base::WeakPtrFactory<MojoMediaPlatformAPI> weak_factory_;

  MojoMediaPlatformAPI(const MojoMediaPlatformAPI&) = delete;
  MojoMediaPlatformAPI& operator=(const MojoMediaPlatformAPI&) = delete;
};

}  // namespace neva_media

#endif  // NEVA_NEVA_MEDIA_SERVICE_CLIENTS_MOJO_MEDIA_PLATFORM_API_H_
