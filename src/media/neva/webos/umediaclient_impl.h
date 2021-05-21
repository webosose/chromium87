// Copyright 2018-2020 LG Electronics, Inc.
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

#ifndef MEDIA_NEVA_WEBOS_UMEDIACLIENT_IMPL_H_
#define MEDIA_NEVA_WEBOS_UMEDIACLIENT_IMPL_H_

#include <memory>
#include <string>

#include <uMediaClient.h>

#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/neva/webos/luna_service_client.h"
#include "media/base/pipeline.h"
#include "media/base/ranges.h"
#include "media/neva/webos/webos_mediaclient.h"

namespace base {
class Lock;
class SingleThreadTaskRunner;
}  // namespace base

namespace media {
class LunaServiceClient;
class SystemMediaManager;

class UMediaClientImpl : public WebOSMediaClient,
                         public uMediaServer::uMediaClient,
                         public base::SupportsWeakPtr<UMediaClientImpl> {
 public:
  UMediaClientImpl(
      const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
      base::WeakPtr<WebOSMediaClient::EventListener> event_listener,
      const std::string& app_id);
  ~UMediaClientImpl();

  // WebOSMediaClient implementations
  void Load(bool video,
            double current_time,
            bool is_local_source,
            const std::string& url,
            const std::string& mime_type,
            const std::string& referrer,
            const std::string& user_agent,
            const std::string& cookies,
            const std::string& payload) override;
  void Seek(base::TimeDelta time,
            const media::PipelineStatusCB& seek_cb) override;
  float GetPlaybackRate() const override;
  void SetPlaybackRate(float playback_rate) override;
  double GetPlaybackVolume() const override { return volume_; }
  void SetPlaybackVolume(double volume, bool forced = false) override;
  bool SelectTrack(const MediaTrackType type, const std::string& id) override;
  void Suspend(SuspendReason reason) override;
  void Resume() override;
  bool IsRecoverableOnResume() override;
  void SetPreload(Preload preload) override;
  bool IsPreloadable(const std::string& content_media_option) override;
  std::string MediaId() override;

  double GetDuration() const override { return duration_; }
  void SetDuration(double duration) override { duration_ = duration; }
  double GetCurrentTime() override { return current_time_; }
  void SetCurrentTime(double time) override { current_time_ = time; }

  media::Ranges<base::TimeDelta> GetBufferedTimeRanges() const override;
  bool HasAudio() override { return has_audio_; }
  bool HasVideo() override { return has_video_; }
  gfx::Size GetNaturalVideoSize() override { return natural_video_size_; }
  void SetNaturalVideoSize(const gfx::Size& size) override {
    natural_video_size_ = size;
  }

  bool SetDisplayWindow(const gfx::Rect&,
                        const gfx::Rect&,
                        bool fullscreen,
                        bool forced = false) override;
  void SetVisibility(bool visible) override;
  bool Visibility() override;
  void SetFocus() override;
  bool Focus() override;
  void SwitchToAutoLayout() override;
  bool DidLoadingProgress() override;
  bool UsesIntrinsicSize() const override;
  void Unload() override;
  bool IsSupportedBackwardTrickPlay() override;
  bool IsSupportedPreload() override;
  bool CheckUseMediaPlayerManager(const std::string& mediaOption) override;
  void SetDisableAudio(bool disable) override;
  void SetMediaLayerId(const std::string& media_layer_id) override;

  // uMediaServer::uMediaClient implementations
  bool onPlaying() override;
  bool onPaused() override;
  bool onSeekDone() override;
  bool onEndOfStream() override;
  bool onLoadCompleted() override;
  bool onPreloadCompleted() override;
  bool onUnloadCompleted() override;
  bool onCurrentTime(int64_t currentTime) override;
#if UMS_INTERNAL_API_VERSION == 2
  bool onAudioInfo(const struct ums::audio_info_t&) override;
  bool onVideoInfo(const struct ums::video_info_t&) override;
  bool onSourceInfo(const struct ums::source_info_t&) override;
#else
  bool onAudioInfo(const struct uMediaServer::audio_info_t&) override;
  bool onVideoInfo(const struct uMediaServer::video_info_t&) override;
  bool onSourceInfo(const struct uMediaServer::source_info_t&) override;
#endif
  bool onBufferRange(const struct uMediaServer::buffer_range_t&) override;
  bool onError(int64_t errorCode, const std::string& errorText) override;
  bool onExternalSubtitleTrackInfo(
      const struct uMediaServer::external_subtitle_track_info_t&) override;
  bool onUserDefinedChanged(const char* message) override;
  bool onBufferingStart() override;
  bool onBufferingEnd() override;

  // dispatch event
  void DispatchPlaying();
  void DispatchPaused();
  void DispatchSeekDone();
  void DispatchEndOfStream(bool isForward);
  void DispatchLoadCompleted();
  void DispatchUnloadCompleted();
  void DispatchPreloadCompleted();
  void DispatchCurrentTime(int64_t currentTime);
  void DispatchBufferRange(const struct uMediaServer::buffer_range_t&);
#if UMS_INTERNAL_API_VERSION == 2
  void DispatchSourceInfo(const struct ums::source_info_t&);
  void DispatchAudioInfo(const struct ums::audio_info_t&);
  void DispatchVideoInfo(const struct ums::video_info_t&);
#else
  void DispatchSourceInfo(const struct uMediaServer::source_info_t&);
  void DispatchAudioInfo(const struct uMediaServer::audio_info_t&);
  void DispatchVideoInfo(const struct uMediaServer::video_info_t&);
#endif
  void DispatchError(int64_t errorCode, const std::string& errorText);
  void DispatchExternalSubtitleTrackInfo(
      const struct uMediaServer::external_subtitle_track_info_t&);
  void DispatchUserDefinedChanged(const std::string& message);
  void DispatchBufferingStart();
  void DispatchBufferingEnd();

  double GetStartDate() const { return start_date_; }
  bool IsSeekable() { return seekable_; }
  bool IsEnded() { return ended_; }
  bool IsReleasedMediaResource() { return released_media_resource_; }
  media::Ranges<base::TimeDelta> GetSeekableTimeRanges();
  void InitializeSeeking() { is_seeking_ = false; }
  void ResetEnded() { ended_ = false; }
  bool IsMpegDashContents();
  bool Send(const std::string& message);

 private:
  typedef enum {
    LOADING_STATE_NONE,
    LOADING_STATE_PRELOADING,
    LOADING_STATE_PRELOADED,
    LOADING_STATE_LOADING,
    LOADING_STATE_LOADED,
    LOADING_STATE_UNLOADING,
    LOADING_STATE_UNLOADED
  } LoadingState;

  typedef enum {
    LOADING_ACTION_NONE,
    LOADING_ACTION_LOAD,
    LOADING_ACTION_UNLOAD
  } LoadingAction;

  std::string UpdateMediaOption(const std::string& mediaOption, double start);
  bool IsRequiredUMSInfo();
  bool IsInsufficientSourceInfo();
  bool IsAdaptiveStreaming();
  bool IsNotSupportedSourceInfo();
  bool IsAppName(const char* app_name);
  bool Is2kVideoAndOver();
  bool IsSupportedAudioOutputOnTrickPlaying();
  bool IsSupportedSeekableRanges();

  void SetMediaVideoData(const struct uMediaServer::video_info_t&,
                         bool forced = false);

  void EnableSubtitle(bool enable);

  bool CheckAudioOutput(float playback_rate);
  media::PipelineStatus CheckErrorCode(int64_t errorCode);
  void LoadInternal();
  bool UnloadInternal();
  bool LoadAsyncInternal(const std::string& uri,
                         AudioStreamClass audio_class,
                         const std::string& media_payload);
  bool PreloadInternal(const std::string& uri,
                       AudioStreamClass audio_class,
                       const std::string& media_payload);
  void NotifyForeground();
  inline bool is_loading() { return loading_state_ == LOADING_STATE_LOADING; }
  inline bool is_loaded() { return loading_state_ == LOADING_STATE_LOADED; }
  void ResetPlayerState();

  base::WeakPtr<WebOSMediaClient::EventListener> event_listener_;
  media::PipelineStatusCB seek_cb_;
  bool buffering_state_have_meta_data_ = false;
  double duration_ = 0.0f;
  double current_time_ = 0.0f;
  double buffer_start_ = 0.0f;
  double buffer_end_ = 0.0f;
  double buffer_end_at_last_didLoadingProgress_ = 0.0f;
  int64_t buffer_remaining_ = 0;
  double start_date_ = std::numeric_limits<double>::quiet_NaN();
  bool video_ = false;
  bool seekable_ = true;
  bool ended_ = false;
  bool has_video_ = false;
  bool has_audio_ = false;
  bool fullscreen_ = false;
  std::map<std::string, int32_t> audio_track_ids_;
  bool is_local_source_ = false;
  bool is_usb_file_ = false;
  bool is_seeking_ = false;
  bool is_suspended_ = false;
  bool use_umsinfo_ = false;
  bool use_backward_trick_ = false;
  bool use_pipeline_preload_ = false;
  bool use_set_uri_ = false;
  bool use_dass_control_ = false;
  bool updated_source_info_ = false;
  bool buffering_ = false;
  bool requests_play_ = false;
  bool requests_pause_ = false;
  bool released_media_resource_ = false;
  std::string media_transport_type_;
  gfx::Size natural_video_size_;
  gfx::Size video_size_;
  gfx::Size pixel_aspect_ratio_{1, 1};
  float playback_rate_ = 0;
  float playback_rate_on_eos_ = 0;
  float playback_rate_on_paused_ = 1.0f;
  double volume_ = 1.0f;
  const scoped_refptr<base::SingleThreadTaskRunner> main_task_runner_;
  base::LunaServiceClient luna_service_client_;
  std::string app_id_;
  std::string url_;
  std::string mime_type_;
  std::string referrer_;
  std::string user_agent_;
  std::string cookies_;
  std::string previous_source_info_;
  std::string previous_video_info_;
  std::string previous_user_defined_changed_;
  std::string previous_media_video_data_;
  std::string updated_payload_;
  gfx::Rect previous_display_window_;
  std::unique_ptr<SystemMediaManager> system_media_manager_;

  mutable base::Lock lock_;
  media::Ranges<base::TimeDelta> seekable_ranges_;

  WebOSMediaClient::Preload preload_ = WebOSMediaClient::Preload::kPreloadNone;

  LoadingState loading_state_ =
      LOADING_STATE_NONE;  // unloaded, loading, loaded, unloading
  LoadingAction pending_loading_action_ = LOADING_ACTION_NONE;

  base::WeakPtr<UMediaClientImpl> weak_ptr_;
  bool audio_disabled_ = false;
  std::string media_layer_id_;

  DISALLOW_COPY_AND_ASSIGN(UMediaClientImpl);
};

}  // namespace media

#endif  // MEDIA_NEVA_WEBOS_UMEDIACLIENT_IMPL_H_
