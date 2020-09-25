// Copyright 2014-2020 LG Electronics, Inc.
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

#include "media/blink/neva/webmediaplayer_neva.h"

#include <algorithm>
#include <limits>
#include <string>

#include "base/bind.h"
#include "base/callback.h"
#include "base/command_line.h"
#include "cc/layers/video_layer.h"
#include "cc/trees/layer_tree_host.h"
#include "content/renderer/media/neva/stream_texture_factory.h"
#include "gpu/GLES2/gl2extchromium.h"
#include "gpu/command_buffer/client/gles2_interface.h"
#include "gpu/command_buffer/common/mailbox_holder.h"
#include "gpu/command_buffer/common/sync_token.h"
#include "media/audio/null_audio_sink.h"
#include "media/base/bind_to_current_loop.h"
#include "media/base/media_content_type.h"
#include "media/base/media_log.h"
#include "media/base/media_switches.h"
#include "media/base/media_switches_neva.h"
#include "media/base/timestamp_constants.h"
#include "media/blink/neva/video_util_neva.h"
#include "media/blink/webmediaplayer_params.h"
#include "media/neva/media_constants.h"
#include "net/base/mime_util.h"
#include "third_party/blink/public/platform/web_media_player_source.h"
#include "third_party/blink/public/platform/web_rect.h"
#include "third_party/blink/public/platform/web_size.h"
#include "third_party/blink/public/platform/web_url.h"
#include "third_party/blink/public/platform/webaudiosourceprovider_impl.h"
#include "third_party/blink/public/web/modules/media/webmediaplayer_util.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_settings.h"
#include "third_party/blink/public/web/web_view.h"

using gpu::gles2::GLES2Interface;

using blink::WebMediaPlayer;
using blink::WebMediaPlayerClient;
using blink::WebRect;
using blink::WebSize;
using blink::WebString;
using media::PipelineStatus;
using media::WebMediaPlayerParams;

#define FUNC_LOG(x) VLOG(x) << __func__

namespace {

// Limits the range of playback rate.
const double kMinRate = -16.0;
const double kMaxRate = 16.0;

const char* ReadyStateToString(WebMediaPlayer::ReadyState state) {
#define STRINGIFY_READY_STATUS_CASE(state) \
  case WebMediaPlayer::ReadyState::state:  \
    return #state

  switch (state) {
    STRINGIFY_READY_STATUS_CASE(kReadyStateHaveNothing);
    STRINGIFY_READY_STATUS_CASE(kReadyStateHaveMetadata);
    STRINGIFY_READY_STATUS_CASE(kReadyStateHaveCurrentData);
    STRINGIFY_READY_STATUS_CASE(kReadyStateHaveFutureData);
    STRINGIFY_READY_STATUS_CASE(kReadyStateHaveEnoughData);
  }
  return "null";
}

const char* NetworkStateToString(WebMediaPlayer::NetworkState state) {
#define STRINGIFY_NETWORK_STATUS_CASE(state) \
  case WebMediaPlayer::NetworkState::state:  \
    return #state

  switch (state) {
    STRINGIFY_NETWORK_STATUS_CASE(kNetworkStateEmpty);
    STRINGIFY_NETWORK_STATUS_CASE(kNetworkStateIdle);
    STRINGIFY_NETWORK_STATUS_CASE(kNetworkStateLoading);
    STRINGIFY_NETWORK_STATUS_CASE(kNetworkStateLoaded);
    STRINGIFY_NETWORK_STATUS_CASE(kNetworkStateFormatError);
    STRINGIFY_NETWORK_STATUS_CASE(kNetworkStateNetworkError);
    STRINGIFY_NETWORK_STATUS_CASE(kNetworkStateDecodeError);
  }
  return "null";
}

const char* MediaErrorToString(media::MediaPlayerNeva::MediaError error) {
#define STRINGIFY_MEDIA_ERROR_CASE(error)         \
  case media::MediaPlayerNeva::MediaError::error: \
    return #error

  switch (error) {
    STRINGIFY_MEDIA_ERROR_CASE(MEDIA_ERROR_NONE);
    STRINGIFY_MEDIA_ERROR_CASE(MEDIA_ERROR_FORMAT);
    STRINGIFY_MEDIA_ERROR_CASE(MEDIA_ERROR_DECODE);
    STRINGIFY_MEDIA_ERROR_CASE(MEDIA_ERROR_NOT_VALID_FOR_PROGRESSIVE_PLAYBACK);
    STRINGIFY_MEDIA_ERROR_CASE(MEDIA_ERROR_INVALID_CODE);
  }
  return "null";
}

bool IsBackgroundedSuspendEnabled() {
#if 1
  /* TODO(neva): upstream changed that IsBackgroundSuspendEnabled() returns true
   * by default. As a result, it may cause any conflict with our suspend/resume
   * by FrameMediaController It needs to be revisited.
   */
  return false;
#else
  return base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kEnableMediaSuspend);
#endif
}

class SyncTokenClientImpl : public media::VideoFrame::SyncTokenClient {
 public:
  explicit SyncTokenClientImpl(gpu::gles2::GLES2Interface* gl) : gl_(gl) {}
  ~SyncTokenClientImpl() override {}
  void GenerateSyncToken(gpu::SyncToken* sync_token) override {
    gl_->GenSyncTokenCHROMIUM(sync_token->GetData());
  }
  void WaitSyncToken(const gpu::SyncToken& sync_token) override {
    gl_->WaitSyncTokenCHROMIUM(sync_token.GetConstData());
  }

 private:
  gpu::gles2::GLES2Interface* gl_;
};

}  // namespace

namespace media {

blink::WebMediaPlayer* WebMediaPlayerNeva::Create(
    blink::WebLocalFrame* frame,
    blink::WebMediaPlayerClient* client,
    blink::WebMediaPlayerDelegate* delegate,
    const StreamTextureFactoryCreateCB& stream_texture_factory_create_cb,
    std::unique_ptr<WebMediaPlayerParams> params,
    std::unique_ptr<WebMediaPlayerParamsNeva> params_neva) {
  blink::WebMediaPlayer::LoadType load_type = client->LoadType();
  media::MediaPlayerType media_player_type =
      MediaPlayerNevaFactory::GetMediaPlayerType(
          client->ContentMIMEType().Latin1());
  if (load_type == blink::WebMediaPlayer::kLoadTypeURL &&
      media_player_type != media::MediaPlayerType::kMediaPlayerTypeNone)
    return new WebMediaPlayerNeva(
        frame, client, delegate, stream_texture_factory_create_cb,
        media_player_type, std::move(params), std::move(params_neva));
  return nullptr;
}

bool WebMediaPlayerNeva::CanSupportMediaType(const std::string& mime) {
  if (MediaPlayerNevaFactory::GetMediaPlayerType(mime) ==
      media::MediaPlayerType::kMediaPlayerTypeNone)
    return false;
  else
    return true;
}

WebMediaPlayerNeva::WebMediaPlayerNeva(
    blink::WebLocalFrame* frame,
    blink::WebMediaPlayerClient* client,
    blink::WebMediaPlayerDelegate* delegate,
    const StreamTextureFactoryCreateCB& stream_texture_factory_create_cb,
    const media::MediaPlayerType media_player_type,
    std::unique_ptr<WebMediaPlayerParams> params,
    std::unique_ptr<WebMediaPlayerParamsNeva> params_neva)
    : frame_(frame),
      main_task_runner_(
          frame->GetTaskRunner(blink::TaskType::kMediaElementEvent)),
      client_(client),
      delegate_(delegate),
      delegate_id_(0),
      defer_load_cb_(params->defer_load_cb()),
      buffered_(static_cast<size_t>(1)),
      pending_seek_(false),
      seeking_(false),
      did_loading_progress_(false),
      create_media_player_neva_cb_(
          params_neva->override_create_media_player_neva()),
      network_state_(WebMediaPlayer::kNetworkStateEmpty),
      ready_state_(WebMediaPlayer::kReadyStateHaveNothing),
      is_playing_(false),
      has_size_info_(false),
      video_frame_provider_client_(NULL),
      media_log_(params->take_media_log()),
      interpolator_(&default_tick_clock_),
      playback_completed_(false),
      is_suspended_(client->IsSuppressedMediaPlay()),
      status_on_suspended_(UnknownStatus),
      // Threaded compositing isn't enabled universally yet.
      compositor_task_runner_(params->compositor_task_runner()
                                  ? params->compositor_task_runner()
                                  : base::ThreadTaskRunnerHandle::Get()),
      render_mode_(blink::WebMediaPlayer::RenderModeNone),
      active_video_region_changed_(false),
      app_id_(params_neva->application_id().Utf8().data()),
      is_loading_(false),
      create_video_window_cb_(params_neva->get_create_video_window_callback()) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());

  weak_this_ = weak_factory_.GetWeakPtr();

  if (delegate_)
    delegate_id_ = delegate_->AddObserver(this);

  media_log_->AddEvent<MediaLogEvent::kWebMediaPlayerCreated>(
      url::Origin(frame_->GetSecurityOrigin()).GetURL().spec());

  // Use the null sink if no sink was provided.
  audio_source_provider_ = new blink::WebAudioSourceProviderImpl(
      params->audio_renderer_sink(), media_log_.get());

  if (!create_media_player_neva_cb_)
    create_media_player_neva_cb_ =
        base::BindRepeating(&MediaPlayerNevaFactory::CreateMediaPlayerNeva);

  player_api_.reset(create_media_player_neva_cb_.Run(
      this, media_player_type, main_task_runner_, app_id_));

#if defined(NEVA_VIDEO_HOLE)
  geometry_update_helper_.reset(new VideoHoleGeometryUpdateHelper(
      client, params_neva->additional_contents_scale(),
      base::BindRepeating(&WebMediaPlayerNeva::SetDisplayWindow, weak_this_),
      base::BindRepeating(&WebMediaPlayerNeva::SetVisibility, weak_this_)));
#endif

  video_frame_provider_ = std::make_unique<VideoFrameProviderImpl>(
      stream_texture_factory_create_cb, compositor_task_runner_);
  video_frame_provider_->SetWebLocalFrame(frame);
  video_frame_provider_->SetWebMediaPlayerClient(client);
  SetRenderMode(GetClient()->RenderMode());
  base::Optional<bool> is_audio_disabled = GetClient()->IsAudioDisabled();
  if (is_audio_disabled.has_value())
    SetDisableAudio(*is_audio_disabled);

  bool require_media_resource = player_api_->RequireMediaResource() &&
                                !params_neva->use_unlimited_media_policy();
  delegate_->DidMediaCreated(delegate_id_, require_media_resource);

  EnsureVideoWindowCreated();
}

WebMediaPlayerNeva::~WebMediaPlayerNeva() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  GetClient()->SetCcLayer(nullptr);

  if (video_layer_.get()) {
    video_layer_->StopUsingProvider();
  }
  compositor_task_runner_->DeleteSoon(FROM_HERE,
                                      std::move(video_frame_provider_));

  media_log_->OnWebMediaPlayerDestroyed();

  if (delegate_) {
    delegate_->PlayerGone(delegate_id_);
    delegate_->RemoveObserver(delegate_id_);
  }
}

blink::WebMediaPlayer::LoadTiming WebMediaPlayerNeva::Load(
    LoadType load_type,
    const blink::WebMediaPlayerSource& src,
    CorsMode cors_mode) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  FUNC_LOG(1);

  DCHECK(src.IsURL());

  is_loading_ = true;

  // If preloading is expected, do load without permit from MediaStateManager.
  if (player_api_->IsPreloadable(
          GetClient()->ContentMediaOption().Utf8().data())) {
    DoLoad(load_type, src.GetAsURL(), cors_mode);
    return LoadTiming::kImmediate;
  }

  pending_load_type_ = load_type;
  pending_source_ = blink::WebMediaPlayerSource(src.GetAsURL());
  pending_cors_mode_ = cors_mode;

  delegate_->DidMediaActivationNeeded(delegate_id_);

  return LoadTiming::kDeferred;
}

void WebMediaPlayerNeva::UpdatePlayingState(bool is_playing) {
  FUNC_LOG(1);
  if (is_playing == is_playing_)
    return;

  is_playing_ = is_playing;

  if (is_playing)
    interpolator_.StartInterpolating();
  else
    interpolator_.StopInterpolating();

  if (delegate_) {
    if (is_playing) {
      delegate_->DidPlay(delegate_id_);
    } else {
      // TODO(neva, sync-to-87): Even if OnPlaybackComplete() has not been
      // called yet, Blink may have already fired the ended event based on
      // current time relative to duration -- so we need to check both
      // possibilities here.
      delegate_->DidPause(delegate_id_, IsEnded());
    }
  }
}

void WebMediaPlayerNeva::DoLoad(LoadType load_type,
                                const blink::WebURL& url,
                                CorsMode cors_mode) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  FUNC_LOG(1);
  // We should use MediaInfoLoader for all URLs but because of missing
  // scheme handlers in WAM we use it only for file scheme for now.
  // By using MediaInfoLoader url gets passed to network delegate which
  // does proper whitelist filtering for local file access.
  GURL mediaUrl(url);
  if (mediaUrl.SchemeIsFile() || mediaUrl.SchemeIsFileSystem()) {
    info_loader_.reset(new MediaInfoLoader(
        mediaUrl,
        base::Bind(&WebMediaPlayerNeva::DidLoadMediaInfo, weak_this_)));
    info_loader_->Start(frame_);

    UpdateNetworkState(WebMediaPlayer::kNetworkStateLoading);
    UpdateReadyState(WebMediaPlayer::kReadyStateHaveNothing);
  } else {
    UpdateNetworkState(WebMediaPlayer::kNetworkStateLoading);
    UpdateReadyState(WebMediaPlayer::kReadyStateHaveNothing);
    DidLoadMediaInfo(true, mediaUrl);
  }
}

void WebMediaPlayerNeva::DidLoadMediaInfo(bool ok, const GURL& url) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  FUNC_LOG(1);
  if (!ok) {
    info_loader_.reset();
    UpdateNetworkState(WebMediaPlayer::kNetworkStateNetworkError);
    return;
  }

  media_log_->AddEvent<MediaLogEvent::kLoad>(url.spec());
  url_ = url;

  LoadMedia();
}

void WebMediaPlayerNeva::LoadMedia() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  FUNC_LOG(1);

#if defined(USE_GAV)
  if (!EnsureVideoWindowCreated()) {
    pending_load_media_ = true;
    return;
  }
  pending_load_media_ = false;
#endif

  player_api_->Initialize(
      GetClient()->IsVideo(), CurrentTime(), url_.spec(),
      std::string(GetClient()->ContentMIMEType().Utf8().data()),
      std::string(GetClient()->Referrer().Utf8().data()),
      std::string(GetClient()->UserAgent().Utf8().data()),
      std::string(GetClient()->Cookies().Utf8().data()),
      std::string(GetClient()->ContentMediaOption().Utf8().data()),
      std::string(GetClient()->ContentCustomOption().Utf8().data()));
}

void WebMediaPlayerNeva::OnActiveRegionChanged(const gfx::Rect& active_region) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  LOG(INFO) << __func__ << " (" << active_region.ToString() << ")";
  video_frame_provider_->ActiveRegionChanged(blink::WebRect(active_region));
  if (!NaturalSize().IsEmpty())
    video_frame_provider_->UpdateVideoFrame();
}

void WebMediaPlayerNeva::Play() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  LOG(INFO) << __func__;
  if (!has_activation_permit_) {
    LOG(INFO) << "block to play on suspended";
    status_on_suspended_ = PlayingStatus;
    if (!client_->IsSuppressedMediaPlay())
      delegate_->DidMediaActivationNeeded(delegate_id_);
    return;
  }

  UpdatePlayingState(true);
  player_api_->Start();
  // We think this time as if we have a first frame since platform mediaplayer
  // starts playing. If there is better point, it needs to go there.
  has_first_frame_ = true;

  media_log_->AddEvent<MediaLogEvent::kPlay>();

  if (delegate_)
    delegate_->DidPlay(delegate_id_);
}

void WebMediaPlayerNeva::Pause() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  LOG(INFO) << __func__;

  UpdatePlayingState(false);
  player_api_->Pause();

  paused_time_ = base::TimeDelta::FromSecondsD(CurrentTime());

  media_log_->AddEvent<MediaLogEvent::kPause>();

  if (delegate_) {
    delegate_->DidPause(delegate_id_, IsEnded());
  }
}

// TODO(wanchang): need to propagate to MediaPlayerNeva
bool WebMediaPlayerNeva::SupportsFullscreen() const {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  FUNC_LOG(1);
  return true;
}

void WebMediaPlayerNeva::Seek(double seconds) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  FUNC_LOG(1);

  playback_completed_ = false;

  // TODO(neva): We may need ConvertSecondsToTimestamp here
  // See RP's webmediaplayer_util.h.
  base::TimeDelta new_seek_time = base::TimeDelta::FromSecondsD(seconds);

  if (seeking_) {
    if (new_seek_time == seek_time_) {
      // Suppress all redundant seeks if unrestricted by media source
      // demuxer API.
      pending_seek_ = false;
      return;
    }

    pending_seek_ = true;
    pending_seek_time_ = new_seek_time;

    return;
  }

  seeking_ = true;
  seek_time_ = new_seek_time;

  // Kick off the asynchronous seek!
  player_api_->Seek(seek_time_);
  media_log_->AddEvent<MediaLogEvent::kSeek>(seconds);
}

void WebMediaPlayerNeva::SetRate(double rate) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  FUNC_LOG(1);

  // Limit rates to reasonable values by clamping.
  rate = std::max(kMinRate, std::min(rate, kMaxRate));

  if (!has_activation_permit_) {
    LOG(INFO) << "block to setRate on suspended";
    if (!client_->IsSuppressedMediaPlay())
      delegate_->DidMediaActivationNeeded(delegate_id_);
    return;
  }

  interpolator_.SetPlaybackRate(rate);
  player_api_->SetRate(rate);
  is_negative_playback_rate_ = rate < 0.0f;
}

void WebMediaPlayerNeva::SetVolume(double volume) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  FUNC_LOG(1);
  volume_ = volume;
  player_api_->SetVolume(volume_);
}

void WebMediaPlayerNeva::SetLatencyHint(double seconds) {
  NOTIMPLEMENTED_LOG_ONCE();
}

void WebMediaPlayerNeva::SetPreservesPitch(bool preserves_pitch) {
  NOTIMPLEMENTED_LOG_ONCE();
}

void WebMediaPlayerNeva::OnTimeUpdate() {
  NOTIMPLEMENTED_LOG_ONCE();
}

void WebMediaPlayerNeva::OnPictureInPictureAvailabilityChanged(bool available) {
  NOTIMPLEMENTED_LOG_ONCE();
}

void WebMediaPlayerNeva::SetPreload(Preload preload) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  FUNC_LOG(1);
  if (!EnsureVideoWindowCreated()) {
    pending_preload_ = preload;
    return;
  }
  pending_preload_ = base::nullopt;
  switch (preload) {
    case WebMediaPlayer::kPreloadNone:
      player_api_->SetPreload(MediaPlayerNeva::PreloadNone);
      break;
    case WebMediaPlayer::kPreloadMetaData:
      player_api_->SetPreload(MediaPlayerNeva::PreloadMetaData);
      break;
    case WebMediaPlayer::kPreloadAuto:
      player_api_->SetPreload(MediaPlayerNeva::PreloadAuto);
      break;
    default:
      break;
  }
}

bool WebMediaPlayerNeva::HasVideo() const {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  FUNC_LOG(1);
  return player_api_->HasVideo();
}

bool WebMediaPlayerNeva::HasAudio() const {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  FUNC_LOG(1);
  return player_api_->HasAudio();
}

bool WebMediaPlayerNeva::Paused() const {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  FUNC_LOG(1);
  return !is_playing_;
}

bool WebMediaPlayerNeva::Seeking() const {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  return seeking_;
}

double WebMediaPlayerNeva::Duration() const {
  DCHECK(main_task_runner_->BelongsToCurrentThread());

  if (ready_state_ == WebMediaPlayer::kReadyStateHaveNothing)
    return std::numeric_limits<double>::quiet_NaN();

  return duration_.InSecondsF();
}

double WebMediaPlayerNeva::CurrentTime() const {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  // If the player is processing a seek, return the seek time.
  // Blink may still query us if updatePlaybackState() occurs while seeking.
  if (Seeking()) {
    return pending_seek_ ? pending_seek_time_.InSecondsF()
                         : seek_time_.InSecondsF();
  }

  double current_time =
      std::min((const_cast<media::TimeDeltaInterpolator*>(&interpolator_))
                   ->GetInterpolatedTime(),
               duration_)
          .InSecondsF();

  // The time of interpolator updated from UMediaClient could be a little bigger
  // than the correct current time, this makes |current_time| a negative number
  // after the plaback time reaches at 0:00 by rewinding.
  // this conditional statement sets current_time's lower bound which is 00:00
  if (current_time < 0)
    current_time = 0;

  return current_time;
}

bool WebMediaPlayerNeva::IsEnded() const {
  return playback_completed_;
}

gfx::Size WebMediaPlayerNeva::NaturalSize() const {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  return natural_size_;
}

gfx::Size WebMediaPlayerNeva::VisibleSize() const {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  // FIXME: Need to check visible rect: really it is natural size.
  return natural_size_;
}

WebMediaPlayer::NetworkState WebMediaPlayerNeva::GetNetworkState() const {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  return network_state_;
}

WebMediaPlayer::ReadyState WebMediaPlayerNeva::GetReadyState() const {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  return ready_state_;
}

blink::WebMediaPlayer::SurfaceLayerMode
WebMediaPlayerNeva::GetVideoSurfaceLayerMode() const {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  return blink::WebMediaPlayer::SurfaceLayerMode::kNever;
}

blink::WebString WebMediaPlayerNeva::GetErrorMessage() const {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  return blink::WebString();
}

bool WebMediaPlayerNeva::WouldTaintOrigin() const {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  // TODO(neva) : Need to check return value
  return false;
}

blink::WebTimeRanges WebMediaPlayerNeva::Buffered() const {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  if (!player_api_)
    return blink::WebTimeRanges();

  media::Ranges<base::TimeDelta> ranges = player_api_->GetBufferedTimeRanges();

  return blink::ConvertToWebTimeRanges(ranges);
}

blink::WebTimeRanges WebMediaPlayerNeva::Seekable() const {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  if (ready_state_ < WebMediaPlayer::kReadyStateHaveMetadata)
    return blink::WebTimeRanges();

  // TODO(dalecurtis): Technically this allows seeking on media which return an
  // infinite duration.  While not expected, disabling this breaks semi-live
  // players, http://crbug.com/427412.
  const blink::WebTimeRange seekable_range(0.0, Duration());
  return blink::WebTimeRanges(&seekable_range, 1);
}

bool WebMediaPlayerNeva::DidLoadingProgress() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  bool ret = did_loading_progress_;
  did_loading_progress_ = false;
  return ret;
}

void WebMediaPlayerNeva::Paint(
    cc::PaintCanvas*,
    const blink::WebRect&,
    cc::PaintFlags&,
    int already_uploaded_id,
    blink::WebMediaPlayer::VideoFrameUploadMetadata* out_metadata) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  // TODO(wanchang): check android impl
  return;
}

double WebMediaPlayerNeva::MediaTimeForTimeValue(double timeValue) const {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  return base::TimeDelta::FromSecondsD(timeValue).InSecondsF();
}

unsigned WebMediaPlayerNeva::DecodedFrameCount() const {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  // TODO(wanchang): check android impl
  return 0;
}

unsigned WebMediaPlayerNeva::DroppedFrameCount() const {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  // TODO(wanchang): check android impl
  return 0;
}

uint64_t WebMediaPlayerNeva::AudioDecodedByteCount() const {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  // TODO(wanchang): check android impl
  return 0;
}

uint64_t WebMediaPlayerNeva::VideoDecodedByteCount() const {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  // TODO(wanchang): check android impl
  return 0;
}

bool WebMediaPlayerNeva::HasAvailableVideoFrame() const {
  return has_first_frame_;
}

void WebMediaPlayerNeva::OnMediaMetadataChanged(base::TimeDelta duration,
                                                int width,
                                                int height,
                                                bool success) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  FUNC_LOG(1);
  bool need_to_signal_duration_changed = false;

  // For HLS streams, the reported duration may be zero for infinite streams.
  // See http://crbug.com/501213.
  if (duration.is_zero() && IsHLSStream())
    duration = media::kInfiniteDuration;

  // Update duration, if necessary, prior to ready state updates that may
  // cause duration() query.
  if (duration_ != duration) {
    duration_ = duration;
    // Client readyState transition from HAVE_NOTHING to HAVE_METADATA
    // already triggers a durationchanged event. If this is a different
    // transition, remember to signal durationchanged.
    if (ready_state_ > WebMediaPlayer::kReadyStateHaveNothing) {
      need_to_signal_duration_changed = true;
    }
  }

  if (ready_state_ < WebMediaPlayer::kReadyStateHaveMetadata) {
    UpdateReadyState(WebMediaPlayer::kReadyStateHaveMetadata);
  }

  // TODO(wolenetz): Should we just abort early and set network state to an
  // error if success == false? See http://crbug.com/248399
  if (success) {
    OnVideoSizeChanged(width, height);
  }

  if (need_to_signal_duration_changed)
    client_->DurationChanged();
}

void WebMediaPlayerNeva::OnLoadComplete() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  is_loading_ = false;
  if (ready_state_ < WebMediaPlayer::kReadyStateHaveEnoughData)
    UpdateReadyState(WebMediaPlayer::kReadyStateHaveEnoughData);
  delegate_->DidMediaActivated(delegate_id_);
}

void WebMediaPlayerNeva::OnPlaybackComplete() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  // When playback is about to finish, android media player often stops
  // at a time which is smaller than the duration. This makes webkit never
  // know that the playback has finished. To solve this, we set the
  // current time to media duration when OnPlaybackComplete() get called.
  // But in case of negative playback, we set the current time to zero.
  base::TimeDelta bound =
      is_negative_playback_rate_ ? base::TimeDelta() : duration_;
  interpolator_.SetBounds(
      bound, bound,
      base::TimeTicks::Now());  // TODO(wanchang): fix 3rd argument
  playback_completed_ = true;
  client_->TimeChanged();

  // If the loop attribute is set, timeChanged() will update the current time
  // to 0. It will perform a seek to 0. Issue a command to the player to start
  // playing after seek completes.
  if (is_playing_ && seeking_ && seek_time_.is_zero())
    player_api_->Start();
}

void WebMediaPlayerNeva::OnBufferingUpdate(int percentage) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  buffered_[0].end = Duration() * percentage / 100;
  did_loading_progress_ = true;
  did_loading_progress_ = true;

  if (percentage == 100 && network_state_ < WebMediaPlayer::kNetworkStateLoaded)
    UpdateNetworkState(WebMediaPlayer::kNetworkStateLoaded);
}

void WebMediaPlayerNeva::OnSeekComplete(const base::TimeDelta& current_time) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  seeking_ = false;
  if (pending_seek_) {
    pending_seek_ = false;
    Seek(pending_seek_time_.InSecondsF());
    return;
  }
  interpolator_.SetBounds(current_time, current_time, base::TimeTicks::Now());

  UpdateReadyState(WebMediaPlayer::kReadyStateHaveEnoughData);

  client_->TimeChanged();
}

void WebMediaPlayerNeva::OnMediaError(int error_type) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  LOG(ERROR) << __func__ << "("
             << MediaErrorToString((MediaPlayerNeva::MediaError)error_type)
             << ")";

  if (is_loading_) {
    is_loading_ = false;
    delegate_->DidMediaActivated(delegate_id_);
  }

  switch (error_type) {
    case MediaPlayerNeva::MEDIA_ERROR_FORMAT:
      UpdateNetworkState(WebMediaPlayer::kNetworkStateFormatError);
      break;
    case MediaPlayerNeva::MEDIA_ERROR_DECODE:
      UpdateNetworkState(WebMediaPlayer::kNetworkStateDecodeError);
      break;
    case MediaPlayerNeva::MEDIA_ERROR_NOT_VALID_FOR_PROGRESSIVE_PLAYBACK:
      UpdateNetworkState(WebMediaPlayer::kNetworkStateFormatError);
      break;
    case MediaPlayerNeva::MEDIA_ERROR_INVALID_CODE:
      break;
  }
  client_->Repaint();
}

void WebMediaPlayerNeva::OnVideoSizeChanged(int width, int height) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  FUNC_LOG(1);

  // Ignore OnVideoSizeChanged before kReadyStateHaveMetadata.
  // OnVideoSizeChanged will be called again from OnMediaMetadataChanged
  if (ready_state_ < WebMediaPlayer::kReadyStateHaveMetadata)
    return;

  // For HLS streams, a bogus empty size may be reported at first, followed by
  // the actual size only once playback begins. See http://crbug.com/509972.
  if (!has_size_info_ && width == 0 && height == 0 && IsHLSStream())
    return;

  has_size_info_ = true;
  if (natural_size_.width == width && natural_size_.height == height)
    return;

  natural_size_.width = width;
  natural_size_.height = height;

  client_->SizeChanged();

  if (video_window_remote_)
    video_window_remote_->SetNaturalVideoSize(natural_size_);
  // set video size first then update videoframe since videoframe
  // needs video size.
  video_frame_provider_->SetNaturalVideoSize(NaturalSize());
  video_frame_provider_->UpdateVideoFrame();

  // Lazily allocate compositing layer.
  if (!video_layer_) {
    video_layer_ = cc::VideoLayer::Create(video_frame_provider_.get(),
                                          media::VIDEO_ROTATION_0);
    client_->SetCcLayer(video_layer_.get());

    // If we're paused after we receive metadata for the first time, tell the
    // delegate we can now be safely suspended due to inactivity if a subsequent
    // play event does not occur.
    if (Paused() && delegate_)
      delegate_->DidPause(delegate_id_, IsEnded());
  }

#if defined(NEVA_VIDEO_HOLE)
  if (!RenderTexture()) {
    geometry_update_helper_->SetNaturalVideoSize(NaturalSize());
  }
#endif
}

void WebMediaPlayerNeva::OnAudioTracksUpdated(
    const std::vector<MediaTrackInfo>& audio_track_info) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  for (auto& audio_track : audio_track_info) {
    // Check current id is already added or not.
    auto it = std::find_if(audio_track_ids_.begin(), audio_track_ids_.end(),
                           [&audio_track](const MediaTrackId& id) {
                             return audio_track.id == id.second;
                           });
    if (it != audio_track_ids_.end())
      continue;

    // TODO(neva): Use kind info. And as per comment in WebMediaPlayerImpl,
    // only the first audio track is enabled by default to match blink logic.
    WebMediaPlayer::TrackId track_id = GetClient()->AddAudioTrack(
        blink::WebString::FromUTF8(audio_track.id),
        blink::WebMediaPlayerClient::kAudioTrackKindMain,
        blink::WebString::FromUTF8("Audio Track"),
        blink::WebString::FromUTF8(audio_track.language), false);
    if (!track_id.IsNull() && !track_id.IsEmpty())
      audio_track_ids_.push_back(MediaTrackId(track_id, audio_track.id));
  }

  // TODO(neva): Should we remove unavailable audio track?
}

void WebMediaPlayerNeva::OnTimeUpdate(base::TimeDelta current_timestamp,
                                      base::TimeTicks current_time_ticks) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());

  if (Seeking())
    return;

  // Compensate the current_timestamp with the IPC latency.
  base::TimeDelta lower_bound =
      base::TimeTicks::Now() - current_time_ticks + current_timestamp;
  base::TimeDelta upper_bound = lower_bound;
  // We should get another time update in about |kTimeUpdateInterval|
  // milliseconds.
  if (is_playing_) {
    upper_bound += base::TimeDelta::FromMilliseconds(kTimeUpdateInterval);
  }

  if (lower_bound > upper_bound)
    upper_bound = lower_bound;
  interpolator_.SetBounds(
      lower_bound, upper_bound,
      current_time_ticks);  // TODO(wanchang): check 3rd argument
}

void WebMediaPlayerNeva::OnMediaPlayerPlay() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  UpdatePlayingState(true);
  client_->RequestPlay();
}

void WebMediaPlayerNeva::OnMediaPlayerPause() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  UpdatePlayingState(false);
  client_->RequestPause();
}

void WebMediaPlayerNeva::UpdateNetworkState(
    WebMediaPlayer::NetworkState state) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  VLOG(1) << __func__ << "(" << NetworkStateToString(state) << ")";
  if (ready_state_ == WebMediaPlayer::kReadyStateHaveNothing &&
      (state == WebMediaPlayer::kNetworkStateNetworkError ||
       state == WebMediaPlayer::kNetworkStateDecodeError)) {
    // Any error that occurs before reaching ReadyStateHaveMetadata should
    // be considered a format error.
    network_state_ = WebMediaPlayer::kNetworkStateFormatError;
  } else {
    network_state_ = state;
  }
  // Always notify to ensure client has the latest value.
  GetClient()->NetworkStateChanged();
}

void WebMediaPlayerNeva::UpdateReadyState(WebMediaPlayer::ReadyState state) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  VLOG(1) << __func__ << "(" << ReadyStateToString(state) << ")";

  if (state == WebMediaPlayer::kReadyStateHaveEnoughData &&
      url_.SchemeIs("file") &&
      network_state_ == WebMediaPlayer::kNetworkStateLoading)
    UpdateNetworkState(WebMediaPlayer::kNetworkStateLoaded);

  ready_state_ = state;
  // Always notify to ensure client has the latest value.
  GetClient()->ReadyStateChanged();
}

// Do a GPU-GPU texture copy of the current video frame to |texture|,
// reallocating |texture| at the appropriate size with given internal
// format, format, and type if necessary. If the copy is impossible
// or fails, it returns false.

// TODO(wanchang): |target| |level| are added. need to check why they are added.
bool WebMediaPlayerNeva::CopyVideoTextureToPlatformTexture(
    gpu::gles2::GLES2Interface* web_graphics_context,
    unsigned int target,
    unsigned int texture,
    unsigned internal_format,
    unsigned format,
    unsigned type,
    int level,
    bool premultiply_alpha,
    bool flip_y,
    int already_uploaded_id,
    VideoFrameUploadMetadata* out_metadata) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  if (!RenderTexture())
    return false;
  scoped_refptr<media::VideoFrame> video_frame;
  {
    base::AutoLock auto_lock(video_frame_provider_->GetLock());
    video_frame = video_frame_provider_->GetCurrentFrame();
  }

  if (!video_frame.get() || !video_frame->HasTextures()) {
    return false;
  }

  const gpu::MailboxHolder& mailbox_holder = video_frame->mailbox_holder(0);
  if (mailbox_holder.texture_target != GL_TEXTURE_2D) {
    return false;
  }

// Since this method changes which texture is bound to the TEXTURE_2D target,
// ideally it would restore the currently-bound texture before returning.
// The cost of getIntegerv is sufficiently high, however, that we want to
// avoid it in user builds. As a result assume (below) that |texture| is
// bound when this method is called, and only verify this fact when
// DCHECK_IS_ON.
#if DCHECK_IS_ON()
  GLint bound_texture = 0;
  web_graphics_context->GetIntegerv(GL_TEXTURE_BINDING_2D, &bound_texture);
  DCHECK_EQ(static_cast<GLuint>(bound_texture), texture);
#endif

  web_graphics_context->WaitSyncTokenCHROMIUM(
      mailbox_holder.sync_token.GetConstData());

  uint32_t src_texture = web_graphics_context->CreateAndConsumeTextureCHROMIUM(
      mailbox_holder.mailbox.name);

  // Application itself needs to take care of setting the right flip_y
  // value down to get the expected result.
  // flip_y==true means to reverse the video orientation while
  // flip_y==false means to keep the intrinsic orientation.
  web_graphics_context->CopyTextureCHROMIUM(
      src_texture, 0, mailbox_holder.texture_target, texture, level,
      internal_format, type, flip_y, premultiply_alpha, false);

  web_graphics_context->DeleteTextures(1, &src_texture);

  // The flush() operation is not necessary here. It is kept since the
  // performance will be better when it is added than not.
  web_graphics_context->Flush();

  SyncTokenClientImpl client(web_graphics_context);
  video_frame->UpdateReleaseSyncToken(&client);
  return true;
}

base::WeakPtr<WebMediaPlayer> WebMediaPlayerNeva::AsWeakPtr() {
  return weak_this_;
}

scoped_refptr<blink::WebAudioSourceProviderImpl>
WebMediaPlayerNeva::GetAudioSourceProvider() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  return audio_source_provider_;
}

void WebMediaPlayerNeva::Repaint() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());

  GetClient()->Repaint();
}

#if defined(NEVA_VIDEO_HOLE)
void WebMediaPlayerNeva::SetDisplayWindow(const gfx::Rect& disp_rect,
                                          const gfx::Rect& src_rect,
                                          bool fullscreen) {
  LOG(INFO) << __func__ << " called SetDisplayWindow("
            << "out=[" << disp_rect.ToString() << "]"
            << ", in=[" << src_rect.ToString() << "]"
            << ", is_fullscreen=" << fullscreen << ")";
  player_api_->SetDisplayWindow(disp_rect, src_rect, fullscreen, true);
}

void WebMediaPlayerNeva::SetVisibility(bool visibility) {
  if (player_api_)
    player_api_->SetVisibility(visibility);
}

void WebMediaPlayerNeva::EnteredFullscreen() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  FUNC_LOG(1);
  geometry_update_helper_->SetFullscreenMode(true);
}

void WebMediaPlayerNeva::ExitedFullscreen() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  FUNC_LOG(1);
  geometry_update_helper_->SetFullscreenMode(false);
}
#endif

blink::WebMediaPlayerClient* WebMediaPlayerNeva::GetClient() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DCHECK(client_);
  return client_;
}

void WebMediaPlayerNeva::OnSuspend() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  if (is_suspended_) {
    delegate_->DidMediaSuspended(delegate_id_);
    return;
  }

  is_suspended_ = true;
  has_activation_permit_ = false;
  status_on_suspended_ = Paused() ? PausedStatus : PlayingStatus;
  if (status_on_suspended_ == PlayingStatus) {
    client_->RequestPause();
  }
  if (HasVideo()) {
    video_frame_provider_->SetFrameType(VideoFrameProviderImpl::kBlack);
  }
  SuspendReason reason = client_->IsSuppressedMediaPlay()
                             ? SuspendReason::kBackgrounded
                             : SuspendReason::kSuspendedByPolicy;
  player_api_->Suspend(reason);
  delegate_->DidMediaSuspended(delegate_id_);
}

void WebMediaPlayerNeva::OnMediaActivationPermitted() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
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
  GetClient()->RequestPlay();
  delegate_->DidMediaActivated(delegate_id_);
}

void WebMediaPlayerNeva::OnResume() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  if (!is_suspended_) {
    delegate_->DidMediaActivated(delegate_id_);
    return;
  }

  is_suspended_ = false;

  if (HasVideo()) {
    if (RenderTexture())
      video_frame_provider_->SetFrameType(VideoFrameProviderImpl::kTexture);
#if defined(NEVA_VIDEO_HOLE)
    else
      video_frame_provider_->SetFrameType(VideoFrameProviderImpl::kHole);
#endif
    video_frame_provider_->UpdateVideoFrame();
  }

  if (!player_api_->IsRecoverableOnResume()) {
    player_api_.reset(create_media_player_neva_cb_.Run(
        this,
        MediaPlayerNevaFactory::GetMediaPlayerType(
            client_->ContentMIMEType().Latin1()),
        main_task_runner_, app_id_));
    if (video_window_info_)
      player_api_->SetMediaLayerId(video_window_info_->native_window_id);
    player_api_->SetVolume(volume_);
    LoadMedia();
#if defined(NEVA_VIDEO_HOLE)
    geometry_update_helper_->UpdateVideoHoleBoundary();
#endif
  } else {
    player_api_->Resume();
  }

  if (status_on_suspended_ == PlayingStatus) {
    client_->RequestPlay();
    status_on_suspended_ = UnknownStatus;
  }

  delegate_->DidMediaActivated(delegate_id_);
}

void WebMediaPlayerNeva::OnLoadPermitted() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  LOG(ERROR) << __func__;

  FUNC_LOG(1);
  if (!defer_load_cb_.is_null()) {
    defer_load_cb_.Run(
        base::Bind(&WebMediaPlayerNeva::DoLoad, weak_this_, pending_load_type_,
                   pending_source_.GetAsURL(), pending_cors_mode_));
    return;
  }

  DoLoad(pending_load_type_, pending_source_.GetAsURL(), pending_cors_mode_);
}

bool WebMediaPlayerNeva::UsesIntrinsicSize() const {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  return player_api_->UsesIntrinsicSize();
}

blink::WebString WebMediaPlayerNeva::MediaId() const {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  return blink::WebString::FromUTF8(player_api_->MediaId());
}

bool WebMediaPlayerNeva::HasAudioFocus() const {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  return player_api_->HasAudioFocus();
}

void WebMediaPlayerNeva::SetAudioFocus(bool focus) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  player_api_->SetAudioFocus(focus);
}

void WebMediaPlayerNeva::OnCustomMessage(
    const media::MediaEventType media_event_type,
    const std::string& detail) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  FUNC_LOG(1) << __func__ << " detail: " << detail;

  blink::WebMediaPlayer::MediaEventType converted_event_type =
      blink::WebMediaPlayer::kMediaEventNone;

  switch (media_event_type) {
    case media::MediaEventType::kMediaEventNone:
      converted_event_type =
          blink::WebMediaPlayer::MediaEventType::kMediaEventNone;
      break;
    case media::MediaEventType::kMediaEventUpdateUMSMediaInfo:
      converted_event_type =
          blink::WebMediaPlayer::MediaEventType::kMediaEventUpdateUMSMediaInfo;
      break;
    case media::MediaEventType::kMediaEventBroadcastErrorMsg:
      converted_event_type =
          blink::WebMediaPlayer::MediaEventType::kMediaEventBroadcastErrorMsg;
      break;
    case media::MediaEventType::kMediaEventDvrErrorMsg:
      converted_event_type =
          blink::WebMediaPlayer::MediaEventType::kMediaEventDvrErrorMsg;
      break;
    case media::MediaEventType::kMediaEventUpdateCameraState:
      converted_event_type =
          blink::WebMediaPlayer::MediaEventType::kMediaEventUpdateCameraState;
      break;
    case media::MediaEventType::kMediaEventPipelineStarted:
      converted_event_type =
          blink::WebMediaPlayer::MediaEventType::kMediaEventPipelineStarted;
      break;
  }

  client_->SendCustomMessage(converted_event_type,
                             blink::WebString::FromUTF8(detail));
}

void WebMediaPlayerNeva::OnAudioFocusChanged() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  client_->OnAudioFocusChanged();
}

void WebMediaPlayerNeva::SetRenderMode(blink::WebMediaPlayer::RenderMode mode) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  if (render_mode_ == mode)
    return;

  render_mode_ = mode;
  if (RenderTexture()) {
    video_frame_provider_->SetFrameType(VideoFrameProviderImpl::kTexture);
    player_api_->SwitchToAutoLayout();
    LOG(INFO) << __func__ << " called SwitchToAutoLayout";
  } else {
#if defined(NEVA_VIDEO_HOLE)
    video_frame_provider_->SetFrameType(VideoFrameProviderImpl::kHole);
#endif
  }
}

void WebMediaPlayerNeva::SetDisableAudio(bool disable) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  if (audio_disabled_ == disable)
    return;
  LOG(INFO) << __func__ << " disable=" << disable;
  player_api_->SetDisableAudio(disable);
}

void WebMediaPlayerNeva::EnabledAudioTracksChanged(
    const blink::WebVector<TrackId>& enabled_track_ids) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  auto it = std::find_if(
      audio_track_ids_.begin(), audio_track_ids_.end(),
      [&enabled_track_ids](const MediaTrackId& id) {
        return enabled_track_ids[enabled_track_ids.size() - 1] == id.first;
      });
  if (it != audio_track_ids_.end())
    player_api_->SelectTrack(MediaTrackType::kAudio, it->second);
}

bool WebMediaPlayerNeva::IsHLSStream() const {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  const GURL& url = redirected_url_.is_empty() ? url_ : redirected_url_;
  return (url.SchemeIsHTTPOrHTTPS() || url.SchemeIsFile()) &&
         url.spec().find("m3u8") != std::string::npos;
}

void WebMediaPlayerNeva::OnMediaSourceOpened(
    blink::WebMediaSource* web_media_source) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  client_->MediaSourceOpened(web_media_source);
}

void WebMediaPlayerNeva::OnVideoWindowCreated(const ui::VideoWindowInfo& info) {
  video_window_info_ = info;
  video_frame_provider_->SetOverlayPlaneId(info.window_id);
  player_api_->SetMediaLayerId(info.native_window_id);
  if (natural_size_.width && natural_size_.height)
    video_window_remote_->SetNaturalVideoSize(natural_size_);
  ContinuePlayerWithWindowId();
}

void WebMediaPlayerNeva::OnVideoWindowDestroyed() {
  video_window_info_ = base::nullopt;
  video_window_client_receiver_.reset();
}

void WebMediaPlayerNeva::OnVideoWindowGeometryChanged(const gfx::Rect& rect) {
#if defined(NEVA_VIDEO_HOLE)
  geometry_update_helper_->SetMediaLayerGeometry(rect);
#endif
}

void WebMediaPlayerNeva::OnVideoWindowVisibilityChanged(bool visibility) {
#if defined(NEVA_VIDEO_HOLE)
  geometry_update_helper_->SetMediaLayerVisibility(visibility);
#endif
}

// It returns if video window is already created and can be continued to next
// step.
bool WebMediaPlayerNeva::EnsureVideoWindowCreated() {
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

void WebMediaPlayerNeva::ContinuePlayerWithWindowId() {
  if (pending_preload_)
    SetPreload(pending_preload_.value());
  if (pending_load_media_)
    LoadMedia();
}

void WebMediaPlayerNeva::OnFrameHidden() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  if (!IsBackgroundedSuspendEnabled())
    return;

  OnSuspend();
}

void WebMediaPlayerNeva::OnFrameShown() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  if (!IsBackgroundedSuspendEnabled())
    return;

  OnResume();
}

void WebMediaPlayerNeva::OnEnterPictureInPicture() {
  NOTIMPLEMENTED_LOG_ONCE();
}

void WebMediaPlayerNeva::OnExitPictureInPicture() {
  NOTIMPLEMENTED_LOG_ONCE();
}

void WebMediaPlayerNeva::OnSetAudioSink(const std::string& sink_id) {
  NOTIMPLEMENTED_LOG_ONCE();
}

bool WebMediaPlayerNeva::Send(const std::string& message) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  FUNC_LOG(1) << "message:  " <<  message;
  if (message.empty())
    return false;

  return player_api_->Send(message);
}

}  // namespace media
