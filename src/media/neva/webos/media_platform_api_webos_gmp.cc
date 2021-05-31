// Copyright (c) 2019-2021 LG Electronics, Inc.
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

#include "media/neva/webos/media_platform_api_webos_gmp.h"

#pragma GCC optimize("rtti")
#include <gmp/MediaPlayerClient.h>
#pragma GCC reset_options

#include <gmp/PlayerTypes.h>

#include "base/callback_helpers.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "content/public/common/content_switches.h"
#include "media/base/bind_to_current_loop.h"
#include "third_party/jsoncpp/source/include/json/json.h"

namespace media {

namespace {
const size_t kMaxPendingFeedSize = 15 * 1024 * 1024;  // 15MB
const base::TimeDelta kMaxFeedAheadSeconds = base::TimeDelta::FromSeconds(5);
const base::TimeDelta kMaxFeedAudioVideoDeltaSeconds =
    base::TimeDelta::FromSeconds(1);

GMP_VIDEO_CODEC video_codec[] = {
    GMP_VIDEO_CODEC_NONE,  GMP_VIDEO_CODEC_H264,  GMP_VIDEO_CODEC_VC1,
    GMP_VIDEO_CODEC_MPEG2, GMP_VIDEO_CODEC_MPEG4, GMP_VIDEO_CODEC_THEORA,
    GMP_VIDEO_CODEC_VP8,   GMP_VIDEO_CODEC_VP9,   GMP_VIDEO_CODEC_H265,
};

GMP_AUDIO_CODEC audio_codec[] = {
    GMP_AUDIO_CODEC_NONE,      GMP_AUDIO_CODEC_AAC,
    GMP_AUDIO_CODEC_MP3,       GMP_AUDIO_CODEC_PCM,
    GMP_AUDIO_CODEC_VORBIS,    GMP_AUDIO_CODEC_FLAC,
    GMP_AUDIO_CODEC_AMR_NB,    GMP_AUDIO_CODEC_AMR_WB,
    GMP_AUDIO_CODEC_PCM_MULAW, GMP_AUDIO_CODEC_GSM_MS,
    GMP_AUDIO_CODEC_PCM_S16BE, GMP_AUDIO_CODEC_PCM_S24BE,
    GMP_AUDIO_CODEC_OPUS,      GMP_AUDIO_CODEC_EAC3,
    GMP_AUDIO_CODEC_PCM_ALAW,  GMP_AUDIO_CODEC_ALAC,
    GMP_AUDIO_CODEC_AC3,
};

std::set<MediaPlatformAPIWebOSGmp*> g_media_apis_set_;
std::mutex g_media_apis_set_lock_;

const char* GmpNotifyTypeToString(gint type) {
#define STRINGIFY_NOTIFY_TYPE_CASE(type) \
  case type:                             \
    return #type

  switch (static_cast<NOTIFY_TYPE_T>(type)) {
    STRINGIFY_NOTIFY_TYPE_CASE(NOTIFY_LOAD_COMPLETED);
    STRINGIFY_NOTIFY_TYPE_CASE(NOTIFY_UNLOAD_COMPLETED);
    STRINGIFY_NOTIFY_TYPE_CASE(NOTIFY_SOURCE_INFO);
    STRINGIFY_NOTIFY_TYPE_CASE(NOTIFY_END_OF_STREAM);
    STRINGIFY_NOTIFY_TYPE_CASE(NOTIFY_CURRENT_TIME);
    STRINGIFY_NOTIFY_TYPE_CASE(NOTIFY_SEEK_DONE);
    STRINGIFY_NOTIFY_TYPE_CASE(NOTIFY_PLAYING);
    STRINGIFY_NOTIFY_TYPE_CASE(NOTIFY_PAUSED);
    STRINGIFY_NOTIFY_TYPE_CASE(NOTIFY_NEED_DATA);
    STRINGIFY_NOTIFY_TYPE_CASE(NOTIFY_ENOUGH_DATA);
    STRINGIFY_NOTIFY_TYPE_CASE(NOTIFY_SEEK_DATA);
    STRINGIFY_NOTIFY_TYPE_CASE(NOTIFY_ERROR);
    STRINGIFY_NOTIFY_TYPE_CASE(NOTIFY_VIDEO_INFO);
    STRINGIFY_NOTIFY_TYPE_CASE(NOTIFY_AUDIO_INFO);
    STRINGIFY_NOTIFY_TYPE_CASE(NOTIFY_BUFFER_FULL);
    STRINGIFY_NOTIFY_TYPE_CASE(NOTIFY_BUFFER_NEED);
    STRINGIFY_NOTIFY_TYPE_CASE(NOTIFY_BUFFER_RANGE);
    STRINGIFY_NOTIFY_TYPE_CASE(NOTIFY_BUFFERING_START);
    STRINGIFY_NOTIFY_TYPE_CASE(NOTIFY_BUFFERING_END);
    default:
      return "null";
  }
}
}  // namespace

// static
scoped_refptr<MediaPlatformAPI> MediaPlatformAPI::Create(
    const scoped_refptr<base::SingleThreadTaskRunner>& media_task_runner,
    bool video,
    const std::string& app_id,
    const NaturalVideoSizeChangedCB& natural_video_size_changed_cb,
    const base::Closure& resume_done_cb,
    const base::Closure& suspend_done_cb,
    const ActiveRegionCB& active_region_cb,
    const PipelineStatusCB& error_cb) {
  return base::MakeRefCounted<MediaPlatformAPIWebOSGmp>(
      media_task_runner, video, app_id, natural_video_size_changed_cb,
      resume_done_cb, suspend_done_cb, active_region_cb, error_cb);
}

bool MediaPlatformAPI::IsAvailable() {
  return true;
}

void MediaPlatformAPIWebOSGmp::Callback(const gint type,
                                        const gint64 num_value,
                                        const gchar* str_value,
                                        void* user_data) {
  VLOG(1) << " type=" << GmpNotifyTypeToString(type)
          << " num_value=" << num_value << " str_value=" << str_value
          << " data=" << user_data;
  MediaPlatformAPIWebOSGmp* that =
      static_cast<MediaPlatformAPIWebOSGmp*>(user_data);
  if (that && that->is_finalized_)
    return;

  {
    std::lock_guard<std::mutex> lock(g_media_apis_set_lock_);
    if (!that || g_media_apis_set_.find(that) == g_media_apis_set_.end()) {
      LOG(ERROR) << __func__ << " Callback for erased [" << that << "]";
      return;
    }
  }

  std::string string_value(str_value ? str_value : std::string());
  that->media_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&MediaPlatformAPIWebOSGmp::DispatchCallback,
                 base::Unretained(that), type, num_value, string_value));
}

MediaPlatformAPIWebOSGmp::MediaPlatformAPIWebOSGmp(
    const scoped_refptr<base::SingleThreadTaskRunner>& media_task_runner,
    bool video,
    const std::string& app_id,
    const NaturalVideoSizeChangedCB& natural_video_size_changed_cb,
    const base::Closure& resume_done_cb,
    const base::Closure& suspend_done_cb,
    const ActiveRegionCB& active_region_cb,
    const PipelineStatusCB& error_cb)
    : media_task_runner_(media_task_runner),
      app_id_(app_id),
      natural_video_size_changed_cb_(natural_video_size_changed_cb),
      resume_done_cb_(resume_done_cb),
      suspend_done_cb_(suspend_done_cb),
      active_region_cb_(active_region_cb),
      error_cb_(error_cb) {
  media_player_client_.reset(new gmp::player::MediaPlayerClient(app_id));
  media_player_client_->RegisterCallback(&MediaPlatformAPIWebOSGmp::Callback,
                                         this);
  buffer_queue_.reset(new BufferQueue());

  {
    std::lock_guard<std::mutex> lock(g_media_apis_set_lock_);
    g_media_apis_set_.insert(this);
  }

  SetState(State::CREATED);
}

MediaPlatformAPIWebOSGmp::~MediaPlatformAPIWebOSGmp() {}

void MediaPlatformAPIWebOSGmp::Initialize(
    const AudioDecoderConfig& audio_config,
    const VideoDecoderConfig& video_config,
    const PipelineStatusCB& init_cb) {
  VLOG(1);

  DCHECK(media_task_runner_->BelongsToCurrentThread());
  DCHECK(!init_cb.is_null());

  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);

  audio_config_ = audio_config;
  video_config_ = video_config;
  init_cb_ = init_cb;

  MEDIA_LOAD_DATA_T load_data;
  if (!MakeLoadData(0, &load_data)) {
    LOG(ERROR) << "[" << this << "] " << __func__
               << " Making load data info failed!";
    return;
  }

  if (is_suspended_) {
    VLOG(1) << " -> prevent background init";
    released_media_resource_ = true;
    std::move(init_cb_).Run(PIPELINE_OK);
    return;
  }

  VLOG(2) << " -> call NotifyForeground";
  media_player_client_->NotifyForeground();

  if (!media_player_client_->Load(&load_data)) {
    LOG(ERROR) << "[" << this << "] " << __func__
               << " media_player_client_->Load failed!";
    std::move(error_cb_).Run(PIPELINE_ERROR_DECODE);
    return;
  }
  ResetFeedInfo();

  std::move(init_cb_).Run(PIPELINE_OK);
}

void MediaPlatformAPIWebOSGmp::SetDisplayWindow(const gfx::Rect& rect,
                                                const gfx::Rect& in_rect,
                                                bool fullscreen) {
  VLOG(1);

  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);

  // Store display window rect info and set this if media_player_client
  // available.
  if (!media_player_client_.get()) {
    pending_set_display_window_.was_set = true;
    pending_set_display_window_.rect = rect;
    pending_set_display_window_.in_rect = in_rect;
    pending_set_display_window_.fullscreen = fullscreen;
    return;
  }

  window_rect_ = rect;
  window_in_rect_ = in_rect;

  if (window_in_rect_ == gfx::Rect() || fullscreen) {
    media_player_client_->SetDisplayWindow(window_rect_.x(), window_rect_.y(),
                                           window_rect_.width(),
                                           window_rect_.height(), fullscreen);
    LOG(INFO) << __func__ << " rect=" << rect.ToString()
              << " fullscreen=" << fullscreen;
  } else {
    media_player_client_->SetCustomDisplayWindow(
        window_in_rect_.x(), window_in_rect_.y(), window_in_rect_.width(),
        window_in_rect_.height(), window_rect_.x(), window_rect_.y(),
        window_rect_.width(), window_rect_.height(), fullscreen);
    LOG(INFO) << __func__ << " in_rect=" << in_rect.ToString()
              << " rect=" << rect.ToString() << " fullscreen=" << fullscreen;
  }
}

bool MediaPlatformAPIWebOSGmp::Feed(const scoped_refptr<DecoderBuffer>& buffer,
                                    FeedType type) {
  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);

  if (is_finalized_ || released_media_resource_)
    return true;

  buffer_queue_->Push(buffer, type);

  while (!buffer_queue_->Empty()) {
    FeedStatus feed_status = FeedInternal(buffer_queue_->Front().first,
                                          buffer_queue_->Front().second);
    switch (feed_status) {
      case kFeedSucceeded: {
        buffer_queue_->Pop();
        continue;
      }
      case kFeedFailed: {
        LOG(ERROR) << "[" << this << "] " << __func__ << " feed failed!";
        return false;
      }
      case kFeedOverflowed: {
        if (buffer_queue_->DataSize() > kMaxPendingFeedSize) {
          LOG(INFO) << "[" << this << "] " << __func__ << " pending feed("
                    << buffer_queue_->DataSize() << ") exceeded the limit("
                    << kMaxPendingFeedSize << ")";
          return false;
        }
        LOG(INFO) << "[" << this << "] " << __func__
                  << " buffer_full: pending feed size="
                  << buffer_queue_->DataSize();
        return true;
      }
    }
  }
  return true;
}

void MediaPlatformAPIWebOSGmp::Seek(base::TimeDelta time) {
  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);
  VLOG(1) << " time=" << time.InMilliseconds() << "ms";

  if (!media_player_client_)
    return;

  ResetFeedInfo();

  UpdateCurrentTime(time);

  if (!load_completed_) {
    // clear incompletely loaded pipeline
    if (resume_time_ != time) {
      media_player_client_.reset(NULL);
      LOG(INFO) << "[" << this << "] " << __func__
                << " Load is not finished, try to reinitialize";
      ReInitialize(time);
    }
    return;
  }

  SetState(State::SEEKING);

  unsigned seek_time = static_cast<unsigned>(time.InMilliseconds());
  media_player_client_->Seek(seek_time);
}

void MediaPlatformAPIWebOSGmp::Suspend(SuspendReason reason) {
  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);
  VLOG(1) << " media_player_client_=" << media_player_client_.get()
          << "is_finalized_=" << is_finalized_;

  if (!media_player_client_) {
    if (suspend_done_cb_)
      suspend_done_cb_.Run();
    return;
  }

  is_suspended_ = true;
  if (!load_completed_)
    return;

  if (is_finalized_) {
    media_player_client_->NotifyBackground();
    return;
  }

  Unload();

  window_rect_.SetRect(0, 0, 0, 0);
  window_in_rect_.SetRect(0, 0, 0, 0);
  natural_video_size_.SetSize(0, 0);

  if (suspend_done_cb_)
    suspend_done_cb_.Run();
}

void MediaPlatformAPIWebOSGmp::Resume(
    base::TimeDelta paused_time,
    RestorePlaybackMode restore_playback_mode) {
  VLOG(1) << " paused_time=" << paused_time.InMilliseconds() << "ms"
          << " restore_mode="
          << (restore_playback_mode == RestorePlaybackMode::kPlaying
                  ? "kPlaying"
                  : "kPaused");

  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);

  is_suspended_ = false;
  if (released_media_resource_) {
    if (playback_rate_ == 0.0f)
      play_internal_ = false;
    media_task_runner_->PostTask(
        FROM_HERE,
        base::Bind(&MediaPlatformAPIWebOSGmp::ReInitialize, this, paused_time));
    if (resume_done_cb_)
      resume_done_cb_.Run();
    return;
  }

  if (load_completed_)
    media_player_client_->NotifyForeground();
}

void MediaPlatformAPIWebOSGmp::SetPlaybackRate(float playback_rate) {
  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);
  VLOG(1) << " rate(" << playback_rate_ << " -> " << playback_rate << ")";

  float current_playback_rate = playback_rate_;
  playback_rate_ = playback_rate;

  if (!media_player_client_) {
    LOG(ERROR) << "[" << this << "] " << __func__
               << " media_player_client_ is null";
    return;
  }

  if (playback_rate > 0.0f) {
    VLOG(1) << " load_completed_=" << load_completed_;
    if (load_completed_)
      PlayInternal();
    else
      play_internal_ = true;
    return;
  }

  if (current_playback_rate != 0.0f && playback_rate == 0.0f) {
    VLOG(1) << " call PauseInternal()";
    PauseInternal();
  }
}

void MediaPlatformAPIWebOSGmp::SetPlaybackVolume(double volume) {
  VLOG(1) << " " << playback_volume_ << " -> " << volume;

  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);

  if (!load_completed_) {
    playback_volume_ = volume;
    return;
  }

  if (playback_volume_ == volume)
    return;

  SetVolumeInternal(volume);

  playback_volume_ = volume;
}

bool MediaPlatformAPIWebOSGmp::AllowedFeedVideo() {
  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);
  if (!video_config_.IsValidConfig() || !IsFeedableState())
    return false;

  if (feeded_video_pts_ == kNoTimestamp)
    return true;

  base::TimeDelta video_audio_delta = feeded_video_pts_ - feeded_audio_pts_;
  base::TimeDelta buffered_video_time = feeded_video_pts_ - GetCurrentTime();

  return audio_config_.IsValidConfig()
             ? video_audio_delta < kMaxFeedAudioVideoDeltaSeconds
             : buffered_video_time < kMaxFeedAheadSeconds;
}

bool MediaPlatformAPIWebOSGmp::AllowedFeedAudio() {
  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);
  if (!audio_config_.IsValidConfig() || !IsFeedableState())
    return false;

  if (feeded_audio_pts_ == kNoTimestamp)
    return true;

  base::TimeDelta buffered_audio_time = feeded_audio_pts_ - GetCurrentTime();

  return buffered_audio_time < kMaxFeedAheadSeconds;
}

void MediaPlatformAPIWebOSGmp::Finalize() {
  VLOG(1);

  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);

  is_destructed_ = true;
  is_finalized_ = true;

  {
    std::lock_guard<std::mutex> lock(g_media_apis_set_lock_);
    g_media_apis_set_.erase(this);
  }

  if (media_player_client_.get())
    media_player_client_.reset(NULL);

  ResetFeedInfo();
}

bool MediaPlatformAPIWebOSGmp::IsEOSReceived() {
  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);
  return received_eos_;
}

void MediaPlatformAPIWebOSGmp::UpdateVideoConfig(
    const VideoDecoderConfig& video_config) {
  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);
  VLOG(1);
  video_config_ = video_config;
}

void MediaPlatformAPIWebOSGmp::SetVisibility(bool visible) {
  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);
  // TODO: once SetVisibility api is ready, below workaround will be removed
  if (visible == false)
    SetDisplayWindow(gfx::Rect(0, 0, 1, 1), gfx::Rect(0, 0, 1, 1), false);
}

bool MediaPlatformAPIWebOSGmp::HaveEnoughData() {
  bool has_audio = audio_config_.IsValidConfig();
  bool has_video = video_config_.IsValidConfig();

  base::TimeDelta enough_size = has_video
                                    ? base::TimeDelta::FromMilliseconds(3000)
                                    : base::TimeDelta::FromMilliseconds(500);

  if (state_ == State::SEEKING || !load_completed_)
    return false;

  base::TimeDelta current_time = GetCurrentTime();

  if (has_audio && feeded_audio_pts_ - current_time < enough_size)
    return false;
  if (has_video && feeded_video_pts_ - current_time < enough_size)
    return false;
  return true;
}

void MediaPlatformAPIWebOSGmp::SetPlayerEventCb(const PlayerEventCB& callback) {
  player_event_cb_ = callback;
}

void MediaPlatformAPIWebOSGmp::SetStatisticsCb(const StatisticsCB& cb) {
  // In GST, we don't support PipelineStatistics(dropped frame).
}

void MediaPlatformAPIWebOSGmp::SetUpdateCurrentTimeCb(
    const UpdateCurrentTimeCB& callback) {
  update_current_time_cb_ = callback;
}

void MediaPlatformAPIWebOSGmp::UpdateCurrentTime(const base::TimeDelta& time) {
  if (update_current_time_cb_)
    update_current_time_cb_.Run(time);
  MediaPlatformAPI::UpdateCurrentTime(std::move(time));
}

// static
const char* MediaPlatformAPIWebOSGmp::StateToString(State s) {
  static const char* state_string[] = {
      "INVALID",   "CREATED",   "CREATED_SUSPENDED", "LOADING",  "LOADED",
      "PLAYING",   "PAUSED",    "SUSPENDED",         "RESUMING", "SEEKING",
      "RESTORING", "FINALIZED",
  };
  if (s > State::FINALIZED)
    return "INVALID";
  return state_string[static_cast<int>(s)];
}

// private helper functions
void MediaPlatformAPIWebOSGmp::DispatchCallback(const gint type,
                                                const gint64 num_value,
                                                const std::string& str_value) {
  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);
  VLOG(1) << " type=" << GmpNotifyTypeToString(type)
          << " numValue=" << num_value << " strValue=" << str_value;

  switch (static_cast<NOTIFY_TYPE_T>(type)) {
    case NOTIFY_LOAD_COMPLETED:
      NotifyLoadComplete();
      break;
    case NOTIFY_END_OF_STREAM:
      LOG(INFO) << "[" << this << "] " << __func__ << " NOTIFY_END_OF_STREAM";
      received_eos_ = true;
      break;
    case NOTIFY_CURRENT_TIME:
      if (state_ != State::SEEKING)
        UpdateCurrentTime(base::TimeDelta::FromMilliseconds(num_value));
      break;
    case NOTIFY_SEEK_DONE: {
      LOG(INFO) << "[" << this << "] " << __func__
                << " NOTIFY_SEEK_DONE, playback_rate_" << playback_rate_;
      if (playback_rate_ > 0.0f)
        PlayInternal();
      else
        SetState(State::PLAYING);
      if (player_event_cb_)
        player_event_cb_.Run(PlayerEvent::kSeekDone);
      break;
    }
    case NOTIFY_UNLOAD_COMPLETED:
    case NOTIFY_PLAYING:
    case NOTIFY_PAUSED:
    case NOTIFY_NEED_DATA:
    case NOTIFY_ENOUGH_DATA:
    case NOTIFY_SEEK_DATA:
      break;
    case NOTIFY_ERROR:
      LOG(ERROR) << "[" << this << "] " << __func__ << " error=" << num_value;
      if (num_value == GMP_ERROR_RES_ALLOC) {
        is_finalized_ = true;
        released_media_resource_ = true;
        load_completed_ = false;
        media_player_client_.reset(NULL);
      } else if (num_value == GMP_ERROR_STREAM) {
        if (error_cb_)
          std::move(error_cb_).Run(PIPELINE_ERROR_DECODE);
      } else if (num_value == GMP_ERROR_ASYNC) {
        if (error_cb_)
          std::move(error_cb_).Run(PIPELINE_ERROR_ABORT);
      }
      break;
    case NOTIFY_VIDEO_INFO:
      SetMediaVideoData(str_value);
      break;
    case NOTIFY_AUDIO_INFO:
    case NOTIFY_BUFFER_FULL:
      if (player_event_cb_)
        player_event_cb_.Run(PlayerEvent::kBufferFull);
      break;
    case NOTIFY_BUFFER_NEED:
      if (player_event_cb_)
        player_event_cb_.Run(PlayerEvent::kBufferLow);
      break;
    default:
      LOG(WARNING) << "[" << this << "] " << __func__
                   << " default case type=" << type;
      break;
  }
}

void MediaPlatformAPIWebOSGmp::PushEOS() {
  VLOG(1);

  if (is_finalized_ || released_media_resource_)
    return;

  if (media_player_client_)
    media_player_client_->PushEndOfStream();
}

void MediaPlatformAPIWebOSGmp::SetState(State next_state) {
  LOG(INFO) << "[" << this << "] " << __func__ << " " << StateToString(state_)
            << " -> " << StateToString(next_state);

  state_ = next_state;
}

void MediaPlatformAPIWebOSGmp::PlayInternal() {
  VLOG(1);

  if (load_completed_) {
    media_player_client_->SetPlaybackRate(playback_rate_);
    media_player_client_->Play();
    SetState(State::PLAYING);
  }

  play_internal_ = true;
}

void MediaPlatformAPIWebOSGmp::PauseInternal(bool update_media) {
  VLOG(1) << " media_player_client=" << media_player_client_.get();

  if (!media_player_client_)
    return;

  if (update_media)
    media_player_client_->Pause();

  SetState(State::PAUSED);
}

void MediaPlatformAPIWebOSGmp::SetVolumeInternal(double volume) {
  VLOG(1) << " volume=" << volume;

  if (!media_player_client_)
    return;

  media_player_client_->SetVolume(static_cast<int>(volume * 100));
}

MediaPlatformAPIWebOSGmp::FeedStatus MediaPlatformAPIWebOSGmp::FeedInternal(
    const scoped_refptr<DecoderBuffer>& buffer,
    FeedType type) {
  if (is_finalized_ || released_media_resource_)
    return kFeedSucceeded;

  uint64_t pts = buffer->timestamp().InMicroseconds() * 1000;
  const uint8_t* data = buffer->data();
  size_t size = buffer->data_size();

  if (buffer->end_of_stream()) {
    LOG(INFO) << "[" << this << "] " << __func__ << " EOS("
              << (type == FeedType::kAudio ? "Audio" : "Video")
              << ") received!";
    if (type == FeedType::kAudio)
      audio_eos_received_ = true;
    else
      video_eos_received_ = true;

    if (audio_eos_received_ && video_eos_received_)
      PushEOS();

    return kFeedSucceeded;
  }

  MEDIA_DATA_CHANNEL_T es_type = MEDIA_DATA_CH_NONE;
  if (type == FeedType::kVideo)
    es_type = MEDIA_DATA_CH_A;
  else if (type == FeedType::kAudio)
    es_type = MEDIA_DATA_CH_B;

  const guint8* p_buffer = static_cast<const guint8*>(data);

  MEDIA_STATUS_T media_status =
      media_player_client_->Feed(p_buffer, size, pts, es_type);
  if (media_status != MEDIA_OK) {
    LOG(WARNING) << "[" << this << "] " << __func__
                 << " media_status=" << media_status;
    if (media_status == MEDIA_BUFFER_FULL)
      return kFeedOverflowed;
    return kFeedFailed;
  }

  if (type == FeedType::kAudio)
    feeded_audio_pts_ = buffer->timestamp();
  else
    feeded_video_pts_ = buffer->timestamp();

  return kFeedSucceeded;
}

void MediaPlatformAPIWebOSGmp::ResetFeedInfo() {
  feeded_audio_pts_ = kNoTimestamp;
  feeded_video_pts_ = kNoTimestamp;
  audio_eos_received_ = !audio_config_.IsValidConfig();
  video_eos_received_ = !video_config_.IsValidConfig();
  received_eos_ = false;

  buffer_queue_->Clear();
}

void MediaPlatformAPIWebOSGmp::SetMediaVideoData(
    const std::string& video_info_str) {
  DCHECK(media_task_runner_->BelongsToCurrentThread());
  Json::Reader reader;
  Json::Value json_video_info;
  if (reader.parse(video_info_str, json_video_info) &&
      json_video_info.isObject() &&
      json_video_info.isMember("videoInfo")) {
    Json::Value video_info = json_video_info["videoInfo"];
    if (video_info.isMember("video") &&
        video_info["video"].isMember("width") &&
        video_info["video"].isMember("height")) {
      gfx::Size natural_video_size(video_info["video"]["width"].asUInt(),
                                   video_info["video"]["height"].asUInt());

      if (natural_video_size_ != natural_video_size) {
        natural_video_size_ = natural_video_size;
        if (natural_video_size_changed_cb_)
          natural_video_size_changed_cb_.Run(natural_video_size_);
      }
    }
  }
}

void MediaPlatformAPIWebOSGmp::ReInitialize(base::TimeDelta start_time) {
  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);
  VLOG(1);

  if (is_destructed_)
    return;

  if (is_finalized_)
    is_finalized_ = false;

  uint64_t pts = start_time.InMicroseconds() * 1000;
  resume_time_ = start_time;

  if (media_player_client_)
    media_player_client_.reset(NULL);

  media_player_client_.reset(new gmp::player::MediaPlayerClient(app_id_));
  media_player_client_->RegisterCallback(&MediaPlatformAPIWebOSGmp::Callback,
                                         this);

  if (pending_set_display_window_.was_set) {
    SetDisplayWindow(pending_set_display_window_.rect,
                     pending_set_display_window_.in_rect,
                     pending_set_display_window_.fullscreen);
    pending_set_display_window_.was_set = false;
    VLOG(1) << __func__ << " pending_set_display_window_ used";
  }

  MEDIA_LOAD_DATA_T load_data;
  if (!MakeLoadData(pts, &load_data)) {
    LOG(ERROR) << "[" << this << "] " << __func__
               << ": Making load data info failed!";
    return;
  }

  VLOG(1) << " call NotifyForeground";
  media_player_client_->NotifyForeground();

  if (!media_player_client_->Load(&load_data)) {
    LOG(ERROR) << "[" << this << "] " << __func__
               << ": media_player_client_->Load failed!";
    std::move(error_cb_).Run(PIPELINE_ERROR_DECODE);
    return;
  }

  released_media_resource_ = false;
}

void MediaPlatformAPIWebOSGmp::NotifyLoadComplete() {
  VLOG(1) << " state_=" << StateToString(state_)
          << " play_internal_=" << play_internal_;

  load_completed_ = true;

  if (player_event_cb_)
    player_event_cb_.Run(PlayerEvent::kLoadCompleted);

  if (play_internal_)
    PlayInternal();

  SetVolumeInternal(playback_volume_);
}

bool MediaPlatformAPIWebOSGmp::MakeLoadData(int64_t start_time, void* data) {
  MEDIA_LOAD_DATA_T* load_data = static_cast<MEDIA_LOAD_DATA_T*>(data);
  load_data->maxWidth = 1920;
  load_data->maxHeight = 1080;
  load_data->maxFrameRate = 30;

  if (video_config_.IsValidConfig()) {
    VLOG(1) << " video_codec=" << video_config_.codec();
    std::string codec_type =
        base::ToUpperASCII(GetCodecName(video_config_.codec()));
    base::Optional<MediaCodecCapability> capability =
        GetMediaCodecCapabilityForCodec(codec_type);
    if (capability.has_value()) {
      load_data->maxWidth = capability->width;
      load_data->maxHeight = capability->height;
      load_data->maxFrameRate = capability->frame_rate;
    } else {
      LOG(ERROR) << "[" << this << "] " << __func__
                 << " Not Supported Video Codec(" << codec_type << ")";
      return false;
    }

    load_data->videoCodec = video_codec[video_config_.codec()];
    load_data->width = video_config_.natural_size().width();
    load_data->height = video_config_.natural_size().height();
    load_data->frameRate = 30;
    load_data->extraData = (void*)video_config_.extra_data().data();
    load_data->extraSize = video_config_.extra_data().size();
#if defined(USE_GAV)
    load_data->windowId = const_cast<char*>(get_media_layer_id().c_str());
#endif
  }

  if (audio_config_.IsValidConfig()) {
    VLOG(1) << " audio_codec=" << audio_config_.codec();
    load_data->audioCodec = audio_codec[audio_config_.codec()];
    load_data->channels = audio_config_.channel_layout();
    load_data->sampleRate = audio_config_.samples_per_second();
    load_data->bitRate = audio_config_.bits_per_channel();
    load_data->bitsPerSample = 8 * audio_config_.bytes_per_frame();
  }
  load_data->ptsToDecode = start_time;

  VLOG(1) << " Outgoing codec info audio=" << load_data->audioCodec
          << " video=" << load_data->videoCodec;

  return true;
}

bool MediaPlatformAPIWebOSGmp::Loaded() {
  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);
  return load_completed_;
}

std::string MediaPlatformAPIWebOSGmp::GetMediaID() {
  VLOG(1);

  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);

  if (!media_player_client_)
    return std::string();

  const char* media_id = media_player_client_->GetMediaID();
  return media_id ? media_id : std::string();
}

bool MediaPlatformAPIWebOSGmp::IsReleasedMediaResource() {
  VLOG(1);
  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);
  return released_media_resource_;
}

void MediaPlatformAPIWebOSGmp::Unload() {
  VLOG(1);

  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);

  if (load_completed_) {
    load_completed_ = false;
    released_media_resource_ = true;

    if (media_player_client_) {
      is_finalized_ = true;
      VLOG(1) << "[" << this << "] " << __func__ << " destroy media client id("
              << GetMediaID().c_str() << ")";
      media_player_client_.reset(NULL);
    }
  }
}

bool MediaPlatformAPIWebOSGmp::IsFeedableState() const {
  DCHECK(media_task_runner_->BelongsToCurrentThread());
  switch (state_) {
    case State::INVALID:
    case State::FINALIZED:
      return false;
    default:
      return true;
  }
}

}  // namespace media
