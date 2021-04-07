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

#include "neva/neva_media_service/clients/mojo_media_platform_api.h"

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/optional.h"
#include "content/public/child/child_thread.h"
#include "content/public/common/service_manager_connection.h"
#include "media/base/media_switches_neva.h"
#include "media/mojo/common/media_type_converters.h"
#include "media/mojo/common/mojo_decoder_buffer_converter.h"
#include "services/service_manager/public/cpp/connector.h"

namespace neva_media {

// static
scoped_refptr<media::MediaPlatformAPI> MojoMediaPlatformAPI::Create(
    const scoped_refptr<base::SingleThreadTaskRunner>& media_task_runner,
    bool is_video,
    const std::string& app_id,
    const media::MediaPlatformAPI::NaturalVideoSizeChangedCB&
        natural_video_size_changed_cb,
    const base::Closure& resume_done_cb,
    const base::Closure& suspend_done_cb,
    const media::MediaPlatformAPI::ActiveRegionCB& active_region_cb,
    const media::PipelineStatusCB& error_cb,
    mojo::PendingRemote<mojom::MediaServiceProvider> pending_provider) {
  return base::MakeRefCounted<MojoMediaPlatformAPI>(
      media_task_runner, is_video, app_id, natural_video_size_changed_cb,
      resume_done_cb, suspend_done_cb, active_region_cb, error_cb,
      std::move(pending_provider));
}

MojoMediaPlatformAPI::MojoMediaPlatformAPI(
    const scoped_refptr<base::SingleThreadTaskRunner>& media_task_runner,
    bool is_video,
    const std::string& app_id,
    const media::MediaPlatformAPI::NaturalVideoSizeChangedCB&
        natural_video_size_changed_cb,
    const base::Closure& resume_done_cb,
    const base::Closure& suspend_done_cb,
    const media::MediaPlatformAPI::ActiveRegionCB& active_region_cb,
    const media::PipelineStatusCB& error_cb,
    mojo::PendingRemote<mojom::MediaServiceProvider> pending_provider)
    : is_video_(is_video),
      app_id_(app_id),
      pending_provider_(std::move(pending_provider)),
      media_task_runner_(media_task_runner),
      resume_done_cb_(resume_done_cb),
      suspend_done_cb_(suspend_done_cb),
      active_region_cb_(active_region_cb),
      natural_video_size_changed_cb_(natural_video_size_changed_cb),
      error_cb_(error_cb),
      media_platform_api_listener_receiver_(this),
      weak_factory_(this) {
  weak_this_ = weak_factory_.GetWeakPtr();
  media_task_runner_->PostTask(
      FROM_HERE, base::Bind(&MojoMediaPlatformAPI::OnCreate, weak_this_));
}

MojoMediaPlatformAPI::~MojoMediaPlatformAPI() {}

void MojoMediaPlatformAPI::OnCreate() {
  DCHECK(media_task_runner_->BelongsToCurrentThread());
  mojo::Remote<mojom::MediaServiceProvider> provider(
      std::move(pending_provider_));

  auto media_platform_api_listener_remote =
      media_platform_api_listener_receiver_.BindNewPipeAndPassRemote();

  provider->CreateMediaPlatformAPI(
      std::move(media_platform_api_listener_remote), is_video_, app_id_,
      mojo::MakeRequest(&media_platform_api_));
}

void MojoMediaPlatformAPI::Initialize(
    const media::AudioDecoderConfig& audio_config,
    const media::VideoDecoderConfig& video_config,
    const media::PipelineStatusCB& init_cb) {
  DCHECK(media_task_runner_->BelongsToCurrentThread());

  mojo::ScopedDataPipeConsumerHandle remote_consumer_handle;
  mojo_decoder_buffer_writer_ = media::MojoDecoderBufferWriter::Create(
      media::GetDefaultDecoderBufferConverterCapacity(
          media::DemuxerStream::VIDEO),
      &remote_consumer_handle);

  if (media_platform_api_) {
    init_cb_ = std::move(init_cb);

    base::Optional<media::AudioDecoderConfig> maybe_audio_config;
    base::Optional<media::VideoDecoderConfig> maybe_video_config;

    if (audio_config.IsValidConfig())
      maybe_audio_config = audio_config;
    if (video_config.IsValidConfig())
      maybe_video_config = video_config;

    media_platform_api_->Initialize(
        maybe_audio_config, maybe_video_config,
        std::move(remote_consumer_handle),
        base::Bind(&MojoMediaPlatformAPI::OnInitialized, weak_this_));
  }
}

void MojoMediaPlatformAPI::FinalizeMediaPlatformAPI() {
  DCHECK(media_task_runner_->BelongsToCurrentThread());
  weak_factory_.InvalidateWeakPtrs();
  media_platform_api_.reset();
  media_platform_api_listener_receiver_.reset();
}

void MojoMediaPlatformAPI::OnInitialized(media::PipelineStatus status) {
  DCHECK(media_task_runner_->BelongsToCurrentThread());
  std::move(init_cb_).Run(status);
}

void MojoMediaPlatformAPI::SetDisplayWindow(const gfx::Rect& rect,
                                            const gfx::Rect& in_rect,
                                            bool fullscreen) {
  DCHECK(media_task_runner_->BelongsToCurrentThread());
  if (media_platform_api_)
    media_platform_api_->SetDisplayWindow(rect, in_rect, fullscreen);
}

bool MojoMediaPlatformAPI::Feed(
    const scoped_refptr<media::DecoderBuffer>& buffer,
    media::FeedType type) {
  DCHECK(media_task_runner_->BelongsToCurrentThread());
  bool result;

  media::mojom::DecoderBufferPtr mojo_buffer =
      mojo_decoder_buffer_writer_->WriteDecoderBuffer(std::move(buffer));
  if (!mojo_buffer) {
    return false;
  }

  if (media_platform_api_ &&
      media_platform_api_->Feed(std::move(mojo_buffer), type, &result))
    return result;
  return false;
}

void MojoMediaPlatformAPI::Seek(base::TimeDelta time) {
  DCHECK(media_task_runner_->BelongsToCurrentThread());
  if (media_platform_api_)
    media_platform_api_->Seek(time);
}

void MojoMediaPlatformAPI::Suspend(media::SuspendReason reason) {
  if (!media_task_runner_->BelongsToCurrentThread()) {
    media_task_runner_->PostTask(
        FROM_HERE,
        base::Bind(&MojoMediaPlatformAPI::Suspend, weak_this_, reason));
    return;
  }

  if (media_platform_api_)
    media_platform_api_->Suspend(reason);
}

void MojoMediaPlatformAPI::OnSuspendDone() {
  DCHECK(media_task_runner_->BelongsToCurrentThread());
  if (!suspend_done_cb_.is_null())
    suspend_done_cb_.Run();
}

void MojoMediaPlatformAPI::Resume(
    base::TimeDelta paused_time,
    media::RestorePlaybackMode restore_playback_mode) {
  if (!media_task_runner_->BelongsToCurrentThread()) {
    media_task_runner_->PostTask(
        FROM_HERE, base::Bind(&MojoMediaPlatformAPI::Resume, weak_this_,
                              paused_time, restore_playback_mode));
    return;
  }

  if (media_platform_api_)
    media_platform_api_->Resume(paused_time, restore_playback_mode);
}

void MojoMediaPlatformAPI::OnResumeDone() {
  DCHECK(media_task_runner_->BelongsToCurrentThread());
  if (!resume_done_cb_.is_null())
    resume_done_cb_.Run();
}

void MojoMediaPlatformAPI::SetPlaybackRate(float playback_rate) {
  DCHECK(media_task_runner_->BelongsToCurrentThread());
  if (media_platform_api_)
    media_platform_api_->SetPlaybackRate(playback_rate);
}

void MojoMediaPlatformAPI::SetPlaybackVolume(double volume) {
  DCHECK(media_task_runner_->BelongsToCurrentThread());
  if (media_platform_api_)
    media_platform_api_->SetPlaybackVolume(volume);
}

bool MojoMediaPlatformAPI::AllowedFeedVideo() {
  DCHECK(media_task_runner_->BelongsToCurrentThread());
  bool result;
  if (media_platform_api_ && media_platform_api_->AllowedFeedVideo(&result))
    return result;
  return false;
}

bool MojoMediaPlatformAPI::AllowedFeedAudio() {
  DCHECK(media_task_runner_->BelongsToCurrentThread());
  bool result;
  if (media_platform_api_ && media_platform_api_->AllowedFeedAudio(&result))
    return result;
  return false;
}

void MojoMediaPlatformAPI::Finalize() {
  if (!media_task_runner_->BelongsToCurrentThread()) {
    media_task_runner_->PostTask(
        FROM_HERE, base::Bind(&MojoMediaPlatformAPI::Finalize, weak_this_));
    return;
  }

  if (media_platform_api_)
    media_platform_api_->Finalize();
}

void MojoMediaPlatformAPI::SetKeySystem(const std::string& key_system) {
  if (!media_task_runner_->BelongsToCurrentThread()) {
    media_task_runner_->PostTask(
        FROM_HERE, base::Bind(&MojoMediaPlatformAPI::SetKeySystem, weak_this_,
                              key_system));
    return;
  }

  if (media_platform_api_)
    media_platform_api_->SetKeySystem(key_system);
}

bool MojoMediaPlatformAPI::IsEOSReceived() {
  DCHECK(media_task_runner_->BelongsToCurrentThread());
  bool result;
  if (media_platform_api_ && media_platform_api_->IsEOSReceived(&result))
    return result;
  return false;
}

void MojoMediaPlatformAPI::UpdateVideoConfig(
    const media::VideoDecoderConfig& video_config) {
  DCHECK(media_task_runner_->BelongsToCurrentThread());
  if (media_platform_api_)
    media_platform_api_->UpdateVideoConfig(video_config);
}

void MojoMediaPlatformAPI::SetVisibility(bool visible) {
  DCHECK(media_task_runner_->BelongsToCurrentThread());
  if (media_platform_api_)
    media_platform_api_->SetVisibility(visible);
}

void MojoMediaPlatformAPI::SwitchToAutoLayout() {
  if (!media_task_runner_->BelongsToCurrentThread()) {
    media_task_runner_->PostTask(
        FROM_HERE,
        base::Bind(&MojoMediaPlatformAPI::SwitchToAutoLayout, weak_this_));
    return;
  }

  if (media_platform_api_)
    media_platform_api_->SwitchToAutoLayout();
}

void MojoMediaPlatformAPI::SetDisableAudio(bool disable) {
  if (!media_task_runner_->BelongsToCurrentThread()) {
    media_task_runner_->PostTask(
        FROM_HERE, base::Bind(&MojoMediaPlatformAPI::SetDisableAudio,
                              weak_this_, disable));
    return;
  }

  if (media_platform_api_)
    media_platform_api_->SetDisableAudio(disable);
}

bool MojoMediaPlatformAPI::HaveEnoughData() {
  DCHECK(media_task_runner_->BelongsToCurrentThread());
  bool result;
  if (media_platform_api_ && media_platform_api_->HaveEnoughData(&result))
    return result;
  return false;
}

void MojoMediaPlatformAPI::SetPlayerEventCb(
    const media::MediaPlatformAPI::PlayerEventCB& callback) {
  DCHECK(media_task_runner_->BelongsToCurrentThread());
  player_event_cb_ = callback;
}

void MojoMediaPlatformAPI::SetStatisticsCb(const media::StatisticsCB& cb) {
  DCHECK(media_task_runner_->BelongsToCurrentThread());
  statistics_cb_ = cb;
}

base::TimeDelta MojoMediaPlatformAPI::GetCurrentTime() {
  return client_current_time_.load();
}

void MojoMediaPlatformAPI::SetMediaLayerId(const std::string& media_layer_id) {
  if (!media_task_runner_->BelongsToCurrentThread()) {
    media_task_runner_->PostTask(
        FROM_HERE, base::Bind(&MojoMediaPlatformAPI::SetMediaLayerId,
                              weak_this_, media_layer_id));
    return;
  }

  if (media_platform_api_)
    media_platform_api_->SetMediaLayerId(media_layer_id);
}

void MojoMediaPlatformAPI::SetMediaPreferences(const std::string& preferences) {
  if (!media_task_runner_->BelongsToCurrentThread()) {
    media_task_runner_->PostTask(
        FROM_HERE, base::Bind(&MojoMediaPlatformAPI::SetMediaPreferences,
                              weak_this_, preferences));
    return;
  }

  if (media_platform_api_)
    media_platform_api_->SetMediaPreferences(preferences);
}

void MojoMediaPlatformAPI::SetMediaCodecCapabilities(
    const std::vector<media::MediaCodecCapability>& capabilities) {
  if (!media_task_runner_->BelongsToCurrentThread()) {
    media_task_runner_->PostTask(
        FROM_HERE, base::Bind(&MojoMediaPlatformAPI::SetMediaCodecCapabilities,
                              weak_this_, capabilities));
    return;
  }

  if (media_platform_api_)
    media_platform_api_->SetMediaCodecCapabilities(capabilities);
}

void MojoMediaPlatformAPI::OnNaturalVideoSizeChanged(const gfx::Size& size) {
  if (!natural_video_size_changed_cb_.is_null())
    natural_video_size_changed_cb_.Run(size);
}

void MojoMediaPlatformAPI::OnActiveRegionChanged(const gfx::Rect& rect) {
  if (!active_region_cb_.is_null())
    active_region_cb_.Run(rect);
}

void MojoMediaPlatformAPI::OnPipelineStatusChanged(
    media::PipelineStatus status) {
  if (!error_cb_.is_null())
    error_cb_.Run(status);
}

void MojoMediaPlatformAPI::OnPlayerEvent(media::PlayerEvent event) {
  if (!player_event_cb_.is_null())
    player_event_cb_.Run(event);
}

void MojoMediaPlatformAPI::OnStatistics(
    const media::PipelineStatistics& statistics) {
  if (!statistics_cb_.is_null())
    statistics_cb_.Run(statistics);
}

void MojoMediaPlatformAPI::OnUpdateCurrentTime(base::TimeDelta time) {
  client_current_time_.store(time);
}

}  // namespace neva_media
