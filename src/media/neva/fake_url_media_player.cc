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

#include "media/neva/fake_url_media_player.h"

#include "base/command_line.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "media/base/media_switches_neva.h"
#include "media/base/timestamp_constants.h"

#define FUNC_LOG(x) DVLOG(x) << __func__

#define VIDEO_HEIGHT 1920
#define VIDEO_WEIDTH 1080

namespace media {

static MediaPlayerNeva::MediaError convertToMediaError(PipelineStatus status) {
  switch (status) {
    case PIPELINE_OK:
      return MediaPlayerNeva::MEDIA_ERROR_NONE;

    case PIPELINE_ERROR_NETWORK:
      return MediaPlayerNeva::MEDIA_ERROR_INVALID_CODE;

    case PIPELINE_ERROR_DECODE:
      return MediaPlayerNeva::MEDIA_ERROR_DECODE;

    case PIPELINE_ERROR_DECRYPT:
    case PIPELINE_ERROR_ABORT:
    case PIPELINE_ERROR_INITIALIZATION_FAILED:
    case PIPELINE_ERROR_COULD_NOT_RENDER:
    case PIPELINE_ERROR_READ:
    case PIPELINE_ERROR_INVALID_STATE:
      return MediaPlayerNeva::MEDIA_ERROR_FORMAT;

    // Demuxer related errors.
    case DEMUXER_ERROR_COULD_NOT_OPEN:
    case DEMUXER_ERROR_COULD_NOT_PARSE:
    case DEMUXER_ERROR_NO_SUPPORTED_STREAMS:
      return MediaPlayerNeva::MEDIA_ERROR_INVALID_CODE;

    // Decoder related errors.
    case DECODER_ERROR_NOT_SUPPORTED:
      return MediaPlayerNeva::MEDIA_ERROR_FORMAT;

    // resource is released by policy action
    case DECODER_ERROR_RESOURCE_IS_RELEASED:
      return MediaPlayerNeva::MEDIA_ERROR_INVALID_CODE;

    // ChunkDemuxer related errors.
    case CHUNK_DEMUXER_ERROR_APPEND_FAILED:
    case CHUNK_DEMUXER_ERROR_EOS_STATUS_DECODE_ERROR:
    case CHUNK_DEMUXER_ERROR_EOS_STATUS_NETWORK_ERROR:
      return MediaPlayerNeva::MEDIA_ERROR_INVALID_CODE;

    // Audio rendering errors.
    case AUDIO_RENDERER_ERROR:
      return MediaPlayerNeva::MEDIA_ERROR_INVALID_CODE;

    default:
      return MediaPlayerNeva::MEDIA_ERROR_INVALID_CODE;
  }
  return MediaPlayerNeva::MEDIA_ERROR_NONE;
}

FakeURLMediaPlayer::FakeURLMediaPlayer(
    MediaPlayerNevaClient* client,
    const scoped_refptr<base::SingleThreadTaskRunner>& main_task_runner,
    const std::string& app_id)
    : client_(client), main_task_runner_(main_task_runner), app_id_(app_id) {
  LOG(ERROR) << __func__;
}

FakeURLMediaPlayer::~FakeURLMediaPlayer() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
}

void FakeURLMediaPlayer::Initialize(const bool is_video,
                                    const double current_time,
                                    const std::string& url,
                                    const std::string& mime_type,
                                    const std::string& referrer,
                                    const std::string& user_agent,
                                    const std::string& cookies,
                                    const std::string& media_option,
                                    const std::string& custom_option) {
  FUNC_LOG(1) << __func__ << " app_id: " << app_id_ << " / url: " << url
              << " / media_option: " << media_option
              << " / current_time: " << current_time;

  if (!base::StringToDouble(
          base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
              switches::kFakeUrlMediaDuration),
          &duration_))
    duration_ = 200.0f;

  client_->OnBufferingUpdate(100);
  OnBufferingState(kHaveMetadata);
  OnBufferingState(kLoadCompleted);
}

void FakeURLMediaPlayer::Start() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  FUNC_LOG(1) << " playback_rate_ : " << playback_rate_;
  media_state_ = Playing;

  SetRate(playback_rate_);

  OnPlaybackStateChanged(true);
  client_->OnTimeUpdate(GetCurrentTime(), base::TimeTicks::Now());
  interpolator_.StartInterpolating();

  if (!time_update_timer_.IsRunning()) {
    time_update_timer_.Start(
        FROM_HERE,
        base::TimeDelta::FromMilliseconds(media::kTimeUpdateInterval), this,
        &FakeURLMediaPlayer::OnTimeUpdateTimerFired);
  }
}

void FakeURLMediaPlayer::Pause() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  FUNC_LOG(1);
  interpolator_.StopInterpolating();
  time_update_timer_.Stop();
  OnPlaybackStateChanged(false);
  client_->OnTimeUpdate(GetCurrentTime(), base::TimeTicks::Now());
}

void FakeURLMediaPlayer::Seek(const base::TimeDelta& time) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  FUNC_LOG(1) << "seektime : " << time.InSecondsF();

  media_state_to_be_restored_ = media_state_;
  media_state_ = Seeking;

  interpolator_.SetBounds(time, base::TimeDelta::FromSecondsD(duration_),
                          base::TimeTicks::Now());
  if (!end_of_stream_)
    OnSeekDone(PipelineStatus::PIPELINE_OK);
  else
    pending_seek_ = true;
}

void FakeURLMediaPlayer::SetVolume(double volume) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  FUNC_LOG(1) << " volume : " << volume;
  volume_ = volume;
}

void FakeURLMediaPlayer::SetPoster(const GURL& poster) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  FUNC_LOG(1);
}

void FakeURLMediaPlayer::SetRate(double rate) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  FUNC_LOG(1) << " : " << rate;

  playback_rate_ = rate;
  interpolator_.SetPlaybackRate(playback_rate_);
}

void FakeURLMediaPlayer::SetPreload(MediaPlayerNeva::Preload preload) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  FUNC_LOG(1);
  preload_ = preload;
}

bool FakeURLMediaPlayer::IsPreloadable(
    const std::string& content_media_option) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  FUNC_LOG(1);
  return true;
}

bool FakeURLMediaPlayer::HasVideo() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  FUNC_LOG(1);
  return true;
}

bool FakeURLMediaPlayer::HasAudio() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  FUNC_LOG(1);
  return true;
}

bool FakeURLMediaPlayer::SelectTrack(const MediaTrackType type,
                                     const std::string& id) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  FUNC_LOG(1);
  return true;
}

void FakeURLMediaPlayer::SwitchToAutoLayout() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  FUNC_LOG(1);
}

void FakeURLMediaPlayer::SetDisplayWindow(const gfx::Rect& outRect,
                                          const gfx::Rect& inRect,
                                          bool fullScreen,
                                          bool forced) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  FUNC_LOG(1) << "outRect:" << outRect.ToString()
              << " inRect:" << inRect.ToString();
}

bool FakeURLMediaPlayer::UsesIntrinsicSize() const {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  return false;
}

std::string FakeURLMediaPlayer::MediaId() const {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  FUNC_LOG(1);
  return "fake_media_id";
}

bool FakeURLMediaPlayer::HasAudioFocus() const {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  FUNC_LOG(1);
  return audio_focus_;
}

void FakeURLMediaPlayer::SetAudioFocus(bool focus) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  FUNC_LOG(1);
  audio_focus_ = focus;
}

bool FakeURLMediaPlayer::HasVisibility(void) const {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  FUNC_LOG(1);
  return visibility_;
}

void FakeURLMediaPlayer::SetVisibility(bool visibility) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  VLOG(1) << __func__ << " visibility=" << visibility;
  visibility_ = visibility;
}

void FakeURLMediaPlayer::Suspend(SuspendReason reason) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  FUNC_LOG(1);
  if (is_suspended_)
    return;

  is_suspended_ = true;
}

void FakeURLMediaPlayer::Resume() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  FUNC_LOG(1);
  if (!is_suspended_)
    return;

  is_suspended_ = false;
}

bool FakeURLMediaPlayer::RequireMediaResource() const {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  return true;
}

bool FakeURLMediaPlayer::IsRecoverableOnResume() const {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  return true;
}

void FakeURLMediaPlayer::SetDisableAudio(bool disable) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
}

void FakeURLMediaPlayer::OnPlaybackStateChanged(bool playing) {
  FUNC_LOG(1) << " : " << playing;

  if (playing) {
    media_state_ = Playing;
    client_->OnMediaPlayerPlay();
  } else {
    media_state_ = Paused;
    client_->OnMediaPlayerPause();
  }
}

void FakeURLMediaPlayer::OnStreamEnded() {
  FUNC_LOG(1);
  time_update_timer_.Stop();
  end_of_stream_ = true;
  pending_seek_ = true;

  client_->OnPlaybackComplete();

  if (pending_seek_) {
    OnSeekDone(PipelineStatus::PIPELINE_OK);
    pending_seek_ = false;
    end_of_stream_ = false;
  }
}

void FakeURLMediaPlayer::OnSeekDone(PipelineStatus status) {
  FUNC_LOG(1);

  if (status != media::PIPELINE_OK) {
    OnError(status);
    return;
  }

  client_->OnSeekComplete(
      base::TimeDelta::FromSecondsD(GetCurrentTime().InSecondsF()));

  media_state_ = media_state_to_be_restored_;
  SetRate(playback_rate_);

  if (media_state_ == Playing)
    OnPlaybackStateChanged(true);
  else
    OnPlaybackStateChanged(false);
}

void FakeURLMediaPlayer::OnError(PipelineStatus error) {
  FUNC_LOG(1);
  media_state_ = Error;
  client_->OnMediaError(convertToMediaError(error));
}

void FakeURLMediaPlayer::OnBufferingState(BufferingState buffering_state) {
  FUNC_LOG(2) << " state:" << buffering_state;

  // TODO(neva): Ensure following states.
  switch (buffering_state) {
    case kHaveMetadata: {
      gfx::Size videoSize = gfx::Size(VIDEO_HEIGHT, VIDEO_WEIDTH);
      media_state_ = Loading;
      client_->OnMediaMetadataChanged(base::TimeDelta::FromSecondsD(duration_),
                                      videoSize.width(), videoSize.height(),
                                      true);
    } break;
    case kLoadCompleted:
      client_->OnLoadComplete();
      media_state_ = Ready;
      break;
    case kPreloadCompleted:
      client_->OnLoadComplete();
      media_state_ = Ready;
      break;
    case kPrerollCompleted:
      break;
    case kBufferingStart:
      break;
    case kBufferingEnd:
      break;
    case kNetworkStateLoading:
      break;
    case kNetworkStateLoaded:
      break;
  }
}

base::TimeDelta FakeURLMediaPlayer::GetCurrentTime() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  FUNC_LOG(2) << interpolator_.GetInterpolatedTime().InSecondsF();
  return interpolator_.GetInterpolatedTime();
}

void FakeURLMediaPlayer::OnTimeUpdateTimerFired() {
  FUNC_LOG(2) << " : " << GetCurrentTime().InSecondsF()
              << " playback_rate_ : " << playback_rate_;
  if (client_)
    client_->OnTimeUpdate(GetCurrentTime(), base::TimeTicks::Now());

  if (GetCurrentTime().InSecondsF() >= duration_) {
    base::TimeDelta bound = (playback_rate_ < 0.0f)
                                ? base::TimeDelta()
                                : base::TimeDelta::FromSecondsD(duration_);

    interpolator_.SetBounds(bound, bound, base::TimeTicks::Now());
    OnStreamEnded();
  }
}

}  // namespace media
