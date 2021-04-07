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

#include "neva/neva_media_service/services/mojo_media_platform_api_service.h"

#include "base/bind.h"
#include "base/callback.h"
#include "base/command_line.h"
#include "media/base/bind_to_current_loop.h"
#include "media/base/cdm_context.h"
#include "media/base/media_switches.h"
#include "media/base/pipeline.h"
#include "ui/gfx/geometry/rect_f.h"

#include "media/mojo/common/media_type_converters.h"

namespace neva_media {

#define BIND_TO_CURRENT_LOOP(function)             \
  (DCHECK(task_runner_->BelongsToCurrentThread()), \
   media::BindToCurrentLoop(base::Bind(function, weak_factory_.GetWeakPtr())))

MojoMediaPlatformAPIService::MojoMediaPlatformAPIService(
    mojo::PendingRemote<neva_media::mojom::MediaPlatformAPIListener>
        media_platform_api_listener_remote,
    bool is_video,
    const std::string& app_id)
    : media_platform_api_listener_remote_(
          std::move(media_platform_api_listener_remote)) {
  task_runner_ = base::ThreadTaskRunnerHandle::Get();
  media_platform_api_ = media::MediaPlatformAPI::Create(
      task_runner_, is_video, app_id,
      BIND_TO_CURRENT_LOOP(
          &MojoMediaPlatformAPIService::OnNaturalVideoSizeChanged),
      BIND_TO_CURRENT_LOOP(&MojoMediaPlatformAPIService::OnResumeDone),
      BIND_TO_CURRENT_LOOP(&MojoMediaPlatformAPIService::OnSuspendDone),
      BIND_TO_CURRENT_LOOP(&MojoMediaPlatformAPIService::OnActiveRegionChanged),
      BIND_TO_CURRENT_LOOP(
          &MojoMediaPlatformAPIService::OnPipelineStatusChanged));
  media_platform_api_->SetPlayerEventCb(
      BIND_TO_CURRENT_LOOP(&MojoMediaPlatformAPIService::OnPlayerEvent));
  media_platform_api_->SetUpdateCurrentTimeCb(
      BIND_TO_CURRENT_LOOP(&MojoMediaPlatformAPIService::OnUpdateCurrentTime));
}

MojoMediaPlatformAPIService::~MojoMediaPlatformAPIService() = default;

void MojoMediaPlatformAPIService::Initialize(
    const base::Optional<media::AudioDecoderConfig>& maybe_audio_config,
    const base::Optional<media::VideoDecoderConfig>& maybe_video_config,
    mojo::ScopedDataPipeConsumerHandle consumer_handle,
    InitializeCallback init_cb) {
  init_cb_ = std::move(init_cb);
  // Allocate decrypt-only DataPipe size based on video content.
  mojo_decoder_buffer_reader_.reset(
      new media::MojoDecoderBufferReader(std::move(consumer_handle)));

  media::AudioDecoderConfig audio_config;
  media::VideoDecoderConfig video_config;

  if (maybe_audio_config)
    audio_config = *maybe_audio_config;
  if (maybe_video_config)
    video_config = *maybe_video_config;

  if (media_platform_api_)
    media_platform_api_->Initialize(
        audio_config, video_config,
        base::Bind(&MojoMediaPlatformAPIService::OnInitialized,
                   weak_factory_.GetWeakPtr()));
}

void MojoMediaPlatformAPIService::OnInitialized(media::PipelineStatus status) {
  std::move(init_cb_).Run(status);
}

// rect and in_rect should not be empty.
void MojoMediaPlatformAPIService::SetDisplayWindow(const gfx::Rect& rect,
                                                   const gfx::Rect& in_rect,
                                                   bool fullscreen) {
  if (media_platform_api_)
    media_platform_api_->SetDisplayWindow(rect, in_rect, fullscreen);
}
void MojoMediaPlatformAPIService::Feed(media::mojom::DecoderBufferPtr buffer,
                                       media::FeedType type,
                                       FeedCallback callback) {
  mojo_decoder_buffer_reader_->ReadDecoderBuffer(
      std::move(buffer),
      base::BindOnce(&MojoMediaPlatformAPIService::OnFeedReady,
                     weak_factory_.GetWeakPtr(), std::move(callback), type));
}

void MojoMediaPlatformAPIService::OnFeedReady(
    FeedCallback callback,
    media::FeedType type,
    scoped_refptr<media::DecoderBuffer> buffer) {
  if (!buffer) {
    std::move(callback).Run(false);
    return;
  }

  if (media_platform_api_)
    std::move(callback).Run(media_platform_api_->Feed(buffer, type));
}

void MojoMediaPlatformAPIService::Seek(base::TimeDelta time) {
  if (media_platform_api_)
    media_platform_api_->Seek(time);
}

void MojoMediaPlatformAPIService::Suspend(media::SuspendReason reason) {
  if (media_platform_api_)
    media_platform_api_->Suspend(reason);
}

void MojoMediaPlatformAPIService::Resume(
    base::TimeDelta paused_time,
    media::RestorePlaybackMode restore_playback_mode) {
  if (media_platform_api_)
    media_platform_api_->Resume(paused_time, restore_playback_mode);
}

void MojoMediaPlatformAPIService::SetPlaybackRate(float playback_rate) {
  if (media_platform_api_)
    media_platform_api_->SetPlaybackRate(playback_rate);
}

void MojoMediaPlatformAPIService::SetPlaybackVolume(double volume) {
  if (media_platform_api_)
    media_platform_api_->SetPlaybackVolume(volume);
}

void MojoMediaPlatformAPIService::AllowedFeedAudio(
    AllowedFeedAudioCallback callback) {
  if (media_platform_api_)
    std::move(callback).Run(media_platform_api_->AllowedFeedAudio());
}

void MojoMediaPlatformAPIService::AllowedFeedVideo(
    AllowedFeedVideoCallback callback) {
  if (media_platform_api_)
    std::move(callback).Run(media_platform_api_->AllowedFeedVideo());
}

void MojoMediaPlatformAPIService::Finalize() {
  if (media_platform_api_)
    media_platform_api_->Finalize();
}

void MojoMediaPlatformAPIService::SetKeySystem(const std::string& key_system) {
  if (media_platform_api_)
    media_platform_api_->SetKeySystem(key_system);
}

void MojoMediaPlatformAPIService::IsEOSReceived(
    IsEOSReceivedCallback callback) {
  if (media_platform_api_)
    std::move(callback).Run(media_platform_api_->IsEOSReceived());
}

void MojoMediaPlatformAPIService::UpdateVideoConfig(
    const media::VideoDecoderConfig& video_config) {
  if (media_platform_api_)
    media_platform_api_->UpdateVideoConfig(video_config);
}

void MojoMediaPlatformAPIService::SetVisibility(bool visible) {
  if (media_platform_api_)
    media_platform_api_->SetVisibility(visible);
}

void MojoMediaPlatformAPIService::SwitchToAutoLayout() {
  if (media_platform_api_)
    media_platform_api_->SwitchToAutoLayout();
}
void MojoMediaPlatformAPIService::SetDisableAudio(bool disable) {
  if (media_platform_api_)
    media_platform_api_->SetDisableAudio(disable);
}

void MojoMediaPlatformAPIService::HaveEnoughData(
    HaveEnoughDataCallback callback) {
  if (media_platform_api_)
    std::move(callback).Run(media_platform_api_->HaveEnoughData());
}

void MojoMediaPlatformAPIService::SetMediaPreferences(
    const std::string& preferences) {
  if (media_platform_api_)
    media_platform_api_->SetMediaPreferences(preferences);
}

void MojoMediaPlatformAPIService::SetMediaCodecCapabilities(
    const std::vector<media::MediaCodecCapability>& capabilities) {
  if (media_platform_api_)
    media_platform_api_->SetMediaCodecCapabilities(capabilities);
}

void MojoMediaPlatformAPIService::SetMediaLayerId(
    const std::string& media_layer_id) {
  if (media_platform_api_)
    media_platform_api_->SetMediaLayerId(media_layer_id);
}

void MojoMediaPlatformAPIService::OnNaturalVideoSizeChanged(
    const gfx::Size& size) {
  if (media_platform_api_listener_remote_.get())
    media_platform_api_listener_remote_->OnNaturalVideoSizeChanged(size);
}

void MojoMediaPlatformAPIService::OnResumeDone() {
  if (media_platform_api_listener_remote_.get())
    media_platform_api_listener_remote_->OnResumeDone();
}

void MojoMediaPlatformAPIService::OnSuspendDone() {
  if (media_platform_api_listener_remote_.get())
    media_platform_api_listener_remote_->OnSuspendDone();
}

void MojoMediaPlatformAPIService::OnActiveRegionChanged(
    const gfx::Rect& active_region) {
  if (media_platform_api_listener_remote_.get())
    media_platform_api_listener_remote_->OnActiveRegionChanged(active_region);
}

void MojoMediaPlatformAPIService::OnPipelineStatusChanged(
    media::PipelineStatus status) {
  if (media_platform_api_listener_remote_.get())
    media_platform_api_listener_remote_->OnPipelineStatusChanged(status);
}

void MojoMediaPlatformAPIService::OnPlayerEvent(media::PlayerEvent event) {
  if (media_platform_api_listener_remote_.get())
    media_platform_api_listener_remote_->OnPlayerEvent(event);
}

void MojoMediaPlatformAPIService::OnUpdateCurrentTime(
    const base::TimeDelta& time) {
  if (media_platform_api_listener_remote_.get())
    media_platform_api_listener_remote_->OnUpdateCurrentTime(std::move(time));
}

}  // namespace neva_media
