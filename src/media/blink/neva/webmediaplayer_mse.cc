// Copyright 2015-2019 LG Electronics, Inc.
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

#include "media/blink/neva/webmediaplayer_mse.h"

#include "base/command_line.h"
#include "base/metrics/histogram.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/utf_string_conversions.h"
#include "cc/layers/layer.h"
#include "cc/layers/video_layer.h"
#include "cc/trees/layer_tree_host.h"
#include "media/audio/null_audio_sink.h"
#include "media/base/bind_to_current_loop.h"
#include "media/base/renderer_factory_selector.h"
#include "media/blink/neva/video_util_neva.h"
#include "media/blink/webcontentdecryptionmodule_impl.h"
#include "media/neva/media_constants.h"
#include "media/neva/media_platform_api.h"
#include "media/neva/media_preferences.h"
#include "third_party/blink/public/platform/web_media_player_client.h"
#include "third_party/blink/public/platform/webaudiosourceprovider_impl.h"
#include "ui/gfx/geometry/rect_f.h"

namespace media {

#define BIND_TO_RENDER_LOOP_VIDEO_FRAME_PROVIDER(function) \
  (DCHECK(main_task_runner_->BelongsToCurrentThread()),    \
   BindToCurrentLoop(                                      \
       base::Bind(function, (this->video_frame_provider_->AsWeakPtr()))))

#define BIND_TO_RENDER_LOOP(function)                   \
  (DCHECK(main_task_runner_->BelongsToCurrentThread()), \
   BindToCurrentLoop(base::Bind(function, weak_this_for_mse_)))

WebMediaPlayerMSE::WebMediaPlayerMSE(
    blink::WebLocalFrame* frame,
    blink::WebMediaPlayerClient* client,
    blink::WebMediaPlayerEncryptedMediaClient* encrypted_client,
    blink::WebMediaPlayerDelegate* delegate,
    std::unique_ptr<media::RendererFactorySelector> renderer_factory_selector,
    UrlIndex* url_index,
    std::unique_ptr<VideoFrameCompositor> compositor,
    const StreamTextureFactoryCreateCB& stream_texture_factory_create_cb,
    std::unique_ptr<WebMediaPlayerParams> params,
    std::unique_ptr<WebMediaPlayerParamsNeva> params_neva)
    : media::WebMediaPlayerImpl(frame,
                                client,
                                encrypted_client,
                                delegate,
                                std::move(renderer_factory_selector),
                                url_index,
                                std::move(compositor),
                                std::move(params)),
      additional_contents_scale_(params_neva->additional_contents_scale()),
      app_id_(params_neva->application_id().Utf8()),
      create_video_window_cb_(params_neva->get_create_video_window_callback()) {
  weak_this_for_mse_ = weak_factory_for_mse_.GetWeakPtr();

  // Use the null sink for our MSE player
  audio_source_provider_ = new blink::WebAudioSourceProviderImpl(
      new media::NullAudioSink(media_task_runner_), media_log_.get());

  video_frame_provider_ = std::make_unique<VideoFrameProviderImpl>(
      stream_texture_factory_create_cb, vfc_task_runner_);
  video_frame_provider_->SetWebLocalFrame(frame);
  video_frame_provider_->SetWebMediaPlayerClient(client);

  // Create MediaPlatformAPI
  auto create_media_platform_api_cb =
      params_neva->override_create_media_platform_api();

  if (create_media_platform_api_cb) {
    media_platform_api_ = create_media_platform_api_cb.Run(
        media_task_runner_, client_->IsVideo(), app_id_,
        BIND_TO_RENDER_LOOP(&WebMediaPlayerMSE::OnNaturalVideoSizeChanged),
        BIND_TO_RENDER_LOOP(&WebMediaPlayerMSE::OnResumed),
        BIND_TO_RENDER_LOOP(&WebMediaPlayerMSE::OnSuspended),
        BIND_TO_RENDER_LOOP_VIDEO_FRAME_PROVIDER(
            &VideoFrameProviderImpl::ActiveRegionChanged),
        BIND_TO_RENDER_LOOP(&WebMediaPlayerMSE::OnError));
  } else {
    media_platform_api_ = MediaPlatformAPI::Create(
        media_task_runner_, client_->IsVideo(), app_id_,
        BIND_TO_RENDER_LOOP(&WebMediaPlayerMSE::OnNaturalVideoSizeChanged),
        BIND_TO_RENDER_LOOP(&WebMediaPlayerMSE::OnResumed),
        BIND_TO_RENDER_LOOP(&WebMediaPlayerMSE::OnSuspended),
        BIND_TO_RENDER_LOOP_VIDEO_FRAME_PROVIDER(
            &VideoFrameProviderImpl::ActiveRegionChanged),
        BIND_TO_RENDER_LOOP(&WebMediaPlayerMSE::OnError));
  }

  media_platform_api_->SetMediaPreferences(
      MediaPreferences::Get()->GetRawMediaPreferences());
  media_platform_api_->SetMediaCodecCapabilities(
      MediaPreferences::Get()->GetMediaCodecCapabilities());

  geometry_update_helper_.reset(new VideoHoleGeometryUpdateHelper(
      client, params_neva->additional_contents_scale(),
      base::BindRepeating(&media::MediaPlatformAPI::SetDisplayWindow,
                          media_platform_api_),
      base::BindRepeating(&media::MediaPlatformAPI::SetVisibility,
                          media_platform_api_)));

  base::Optional<bool> is_audio_disabled = client_->IsAudioDisabled();
  if (is_audio_disabled.has_value())
    SetDisableAudio(*is_audio_disabled);

  renderer_factory_selector_->GetCurrentFactory()->SetMediaPlatformAPI(
      media_platform_api_);

  SetRenderMode(client_->RenderMode());

  delegate_->DidMediaCreated(delegate_id_,
                             !params_neva->use_unlimited_media_policy());
}

WebMediaPlayerMSE::~WebMediaPlayerMSE() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());

  if (video_layer_)
    video_layer_->StopUsingProvider();

  vfc_task_runner_->DeleteSoon(FROM_HERE, std::move(video_frame_provider_));

  if (media_platform_api_.get())
    media_platform_api_->Finalize();
}

// static
bool WebMediaPlayerMSE::IsAvailable() {
  return MediaPlatformAPI::IsAvailable();
}

blink::WebMediaPlayer::LoadTiming WebMediaPlayerMSE::Load(
    LoadType load_type,
    const blink::WebMediaPlayerSource& source,
    CorsMode cors_mode) {
  DCHECK(source.IsURL());

  is_loading_ = true;
  pending_load_type_ = load_type;
  pending_source_ = blink::WebMediaPlayerSource(source.GetAsURL());
  pending_cors_mode_ = cors_mode;

  delegate_->DidMediaActivationNeeded(delegate_id_);

  return LoadTiming::kDeferred;
}

void WebMediaPlayerMSE::Play() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  if (!has_activation_permit_) {
    status_on_suspended_ = PlayingStatus;
    if (!client_->IsSuppressedMediaPlay())
      delegate_->DidMediaActivationNeeded(delegate_id_);
    return;
  }
  media::WebMediaPlayerImpl::Play();
}

void WebMediaPlayerMSE::Pause() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  if (is_suspended_) {
    status_on_suspended_ = PausedStatus;
    return;
  }
  media::WebMediaPlayerImpl::Pause();
}

void WebMediaPlayerMSE::SetRate(double rate) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  if (!has_activation_permit_) {
    if (!client_->IsSuppressedMediaPlay())
      delegate_->DidMediaActivationNeeded(delegate_id_);
    return;
  }
  media::WebMediaPlayerImpl::SetRate(rate);
}

void WebMediaPlayerMSE::SetVolume(double volume) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());

  media::WebMediaPlayerImpl::SetVolume(volume);
}

void WebMediaPlayerMSE::SetContentDecryptionModule(
    blink::WebContentDecryptionModule* cdm,
    blink::WebContentDecryptionModuleResult result) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());

  // call base-class implementation
  media::WebMediaPlayerImpl::SetContentDecryptionModule(cdm, result);
}

void WebMediaPlayerMSE::EnteredFullscreen() {
  WebMediaPlayerImpl::EnteredFullscreen();
  geometry_update_helper_->SetFullscreenMode(true);
}

void WebMediaPlayerMSE::ExitedFullscreen() {
  WebMediaPlayerImpl::ExitedFullscreen();
  geometry_update_helper_->SetFullscreenMode(false);
}

void WebMediaPlayerMSE::OnPlay() {
  if (client_)
    client_->RequestPlay();
}

void WebMediaPlayerMSE::OnPause() {
  if (client_)
    client_->RequestPause();
}

void WebMediaPlayerMSE::OnSuspend() {
  if (is_suspended_) {
    delegate_->DidMediaSuspended(delegate_id_);
    return;
  }

  status_on_suspended_ = Paused() ? PausedStatus : PlayingStatus;

  if (status_on_suspended_ == PlayingStatus) {
    Pause();
    client_->RequestPause();
  }

  if (media_platform_api_.get()) {
    SuspendReason reason = client_->IsSuppressedMediaPlay()
                               ? SuspendReason::kBackgrounded
                               : SuspendReason::kSuspendedByPolicy;
    media_platform_api_->Suspend(reason);
  }

  is_suspended_ = true;
  has_activation_permit_ = false;

  if (HasVideo())
    video_frame_provider_->SetFrameType(VideoFrameProviderImpl::kBlack);

  // Usually we wait until OnSuspended(), but send DidMediaSuspended()
  // immediately when media_platform_api_ is null.
  if (!media_platform_api_.get())
    delegate_->DidMediaSuspended(delegate_id_);
}

void WebMediaPlayerMSE::OnSuspended() {
  LOG(INFO) << __func__;
  delegate_->DidMediaSuspended(delegate_id_);
}

void WebMediaPlayerMSE::OnMediaActivationPermitted() {
  // If we already have activation permit, just skip.
  if (has_activation_permit_) {
    delegate_->DidMediaActivated(delegate_id_);
    return;
  }

  has_activation_permit_ = true;

  if (is_loading_) {
    OnLoadPermitted();
    return;
  } else if (is_suspended_) {
    OnResume();
    return;
  }

  Play();
  client_->RequestPlay();
  delegate_->DidMediaActivated(delegate_id_);
}

void WebMediaPlayerMSE::OnResume() {
  if (!is_suspended_) {
    delegate_->DidMediaActivated(delegate_id_);
    return;
  }

  is_suspended_ = false;

  RestorePlaybackMode restore_playback_mode;

  restore_playback_mode = (status_on_suspended_ == PausedStatus)
                              ? RestorePlaybackMode::kPaused
                              : RestorePlaybackMode::kPlaying;

  if (media_platform_api_.get())
    media_platform_api_->Resume(paused_time_, restore_playback_mode);
  else {
    // Usually we wait until OnResumed(), but send DidMediaActivated()
    // immediately when media_platform_api_ is null.
    delegate_->DidMediaActivated(delegate_id_);
  }
}

void WebMediaPlayerMSE::OnResumed() {
  LOG(INFO) << __func__;
#if defined(NEVA_VIDEO_HOLE)
  geometry_update_helper_->UpdateVideoHoleBoundary();
#endif

  client_->RequestSeek(paused_time_.InSecondsF());

  if (status_on_suspended_ == PausedStatus) {
    Pause();
    status_on_suspended_ = UnknownStatus;
  } else {
    Play();
    client_->RequestPlay();
  }

  if (HasVideo()) {
    if (RenderTexture())
      video_frame_provider_->SetFrameType(VideoFrameProviderImpl::kTexture);
#if defined(NEVA_VIDEO_HOLE)
    else
      video_frame_provider_->SetFrameType(VideoFrameProviderImpl::kHole);
#endif
  }

  delegate_->DidMediaActivated(delegate_id_);
}

void WebMediaPlayerMSE::OnLoadPermitted() {
  // call base-class implementation
  if (!EnsureVideoWindowCreated()) {
    pending_load_media_ = true;
    return;
  }
  media::WebMediaPlayerImpl::Load(pending_load_type_, pending_source_,
                                  pending_cors_mode_);
}

void WebMediaPlayerMSE::OnNaturalVideoSizeChanged(
    const gfx::Size& natural_video_size) {
  LOG(INFO) << __func__ << " width: " << natural_video_size.width()
            << " , height: " << natural_video_size.height();
  natural_video_size_ = natural_video_size;
  geometry_update_helper_->SetNaturalVideoSize(natural_video_size_);
  if (video_window_remote_)
    video_window_remote_->SetNaturalVideoSize(natural_video_size_);
}

void WebMediaPlayerMSE::OnError(PipelineStatus metadata) {
  if (is_loading_) {
    is_loading_ = false;
    delegate_->DidMediaActivated(delegate_id_);
  }
  media::WebMediaPlayerImpl::OnError(metadata);
}

void WebMediaPlayerMSE::OnMetadata(const PipelineMetadata& metadata) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());

  if (is_loading_) {
    is_loading_ = false;
    delegate_->DidMediaActivated(delegate_id_);
  }

  // Cache the |time_to_metadata_| to use for adjusting the TimeToFirstFrame and
  // TimeToPlayReady metrics later if we end up doing a suspended startup.
  time_to_metadata_ = base::TimeTicks::Now() - load_start_time_;
  media_metrics_provider_->SetTimeToMetadata(time_to_metadata_);
  RecordTimingUMA("Media.TimeToMetadata", time_to_metadata_);

  MaybeSetContainerNameForMetrics();

  pipeline_metadata_ = metadata;
  if (power_status_helper_)
    power_status_helper_->SetMetadata(metadata);

  UMA_HISTOGRAM_ENUMERATION(
      "Media.VideoRotation",
      metadata.video_decoder_config.video_transformation().rotation,
      VIDEO_ROTATION_MAX + 1);

  if (HasAudio()) {
    media_metrics_provider_->SetHasAudio(metadata.audio_decoder_config.codec());
    RecordEncryptionScheme("Audio",
                           metadata.audio_decoder_config.encryption_scheme());
  }

  if (HasVideo()) {
    media_metrics_provider_->SetHasVideo(metadata.video_decoder_config.codec());
    RecordEncryptionScheme("Video",
                           metadata.video_decoder_config.encryption_scheme());

    // TODO(neva): In here, we don't use natural size from platform api.
    // We need to ensure that it is really fine.

    if (pipeline_metadata_.video_decoder_config.video_transformation()
                .rotation == VIDEO_ROTATION_90 ||
        pipeline_metadata_.video_decoder_config.video_transformation()
                .rotation == VIDEO_ROTATION_270) {
      gfx::Size size = pipeline_metadata_.natural_size;
      pipeline_metadata_.natural_size = gfx::Size(size.height(), size.width());
    }

    // TODO(neva): We don't support media::kUseSurfaceLayerForVideo feature.
    CHECK(!surface_layer_for_video_enabled_);

    DCHECK(!video_layer_);

    // Assume that first frame is received
    if (!has_first_frame_)
      media::WebMediaPlayerImpl::OnFirstFrame(base::TimeTicks::Now());

    video_frame_provider_->SetNaturalVideoSize(pipeline_metadata_.natural_size);
    video_frame_provider_->UpdateVideoFrame();

    video_layer_ = cc::VideoLayer::Create(
        video_frame_provider_.get(),
        pipeline_metadata_.video_decoder_config.video_transformation()
            .rotation);
    video_layer_->SetContentsOpaque(opaque_);
    client_->SetCcLayer(video_layer_.get());
  }

  if (observer_)
    observer_->OnMetadataChanged(pipeline_metadata_);

  // TODO(dalecurtis): Don't create these until kReadyStateHaveFutureData; when
  // we create them early we just increase the chances of needing to throw them
  // away unnecessarily.
  CreateWatchTimeReporter();
  CreateVideoDecodeStatsReporter();

  // SetReadyState() may trigger all sorts of calls into this class (e.g.,
  // Play(), Pause(), etc) so do it last to avoid unexpected states during the
  // calls. An exception to this is UpdatePlayState(), which is safe to call and
  // needs to use the new ReadyState in its calculations.
  SetReadyState(WebMediaPlayer::kReadyStateHaveMetadata);
  UpdatePlayState();
}

void WebMediaPlayerMSE::OnVideoWindowCreated(const ui::VideoWindowInfo& info) {
  video_window_info_ = info;
  video_frame_provider_->SetOverlayPlaneId(info.window_id);
  media_platform_api_->SetMediaLayerId(info.native_window_id);
  if (!natural_video_size_.IsEmpty())
    video_window_remote_->SetNaturalVideoSize(natural_video_size_);
  ContinuePlayerWithWindowId();
}

void WebMediaPlayerMSE::OnVideoWindowDestroyed() {
  video_window_info_ = base::nullopt;
  video_window_client_receiver_.reset();
}

void WebMediaPlayerMSE::OnVideoWindowGeometryChanged(const gfx::Rect& rect) {
#if defined(NEVA_VIDEO_HOLE)
  geometry_update_helper_->SetMediaLayerGeometry(rect);
#endif
}

void WebMediaPlayerMSE::OnVideoWindowVisibilityChanged(bool visibility) {
#if defined(NEVA_VIDEO_HOLE)
  geometry_update_helper_->SetMediaLayerVisibility(visibility);
#endif
}

// It returns if video window is already created and can be continued to next
// step.
bool WebMediaPlayerMSE::EnsureVideoWindowCreated() {
  if (video_window_info_)
    return true;
  // |is_bound()| would be true if we already requested so we need to just wait
  // for response
  if (video_window_client_receiver_.is_bound())
    return false;

  mojo::PendingRemote<ui::mojom::VideoWindowClient> pending_client;
  video_window_client_receiver_.Bind(
      pending_client.InitWithNewPipeAndPassReceiver());

  mojo::PendingRemote<ui::mojom::VideoWindow> pending_window_remote;
  create_video_window_cb_.Run(
      std::move(pending_client),
      pending_window_remote.InitWithNewPipeAndPassReceiver(),
      ui::VideoWindowParams());
  video_window_remote_.Bind(std::move(pending_window_remote));
  return false;
}

void WebMediaPlayerMSE::ContinuePlayerWithWindowId() {
  if (pending_load_media_) {
    media::WebMediaPlayerImpl::Load(pending_load_type_, pending_source_,
                                    pending_cors_mode_);
    pending_load_media_ = false;
  }
}

scoped_refptr<VideoFrame> WebMediaPlayerMSE::GetCurrentFrameFromCompositor()
    const {
  TRACE_EVENT0("media", "WebMediaPlayerMSE::GetCurrentFrameFromCompositor");

  return video_frame_provider_->GetCurrentFrame();
}

double WebMediaPlayerMSE::TimelineOffset() const {
  return media::WebMediaPlayerImpl::timelineOffset();
}

bool WebMediaPlayerMSE::UsesIntrinsicSize() const {
  return MediaPreferences::Get()->UseIntrinsicSizeForMSE();
}

void WebMediaPlayerMSE::SetRenderMode(blink::WebMediaPlayer::RenderMode mode) {
  if (render_mode_ == mode)
    return;

  render_mode_ = mode;
  if (RenderTexture()) {
    video_frame_provider_->SetFrameType(VideoFrameProviderImpl::kTexture);
#if defined(USE_VIDEO_TEXTURE)
    if (gfx::VideoTexture::IsSupported())
      media_platform_api_->SwitchToAutoLayout();
#endif
  } else {
#if defined(NEVA_VIDEO_HOLE)
    video_frame_provider_->SetFrameType(VideoFrameProviderImpl::kHole);
#endif
  }
}

void WebMediaPlayerMSE::SetDisableAudio(bool disable) {
  LOG(INFO) << __func__ << " disable=" << disable;
  media_platform_api_->SetDisableAudio(disable);
}

}  // namespace media
