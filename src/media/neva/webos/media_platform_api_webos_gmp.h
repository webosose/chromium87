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

#ifndef MEDIA_NEVA_WEBOS_MEDIA_PLATFORM_API_WEBOS_GMP_H_
#define MEDIA_NEVA_WEBOS_MEDIA_PLATFORM_API_WEBOS_GMP_H_

#include "media/neva/media_platform_api.h"
#include "ui/gfx/geometry/rect.h"

#include <glib.h>

#include <mutex>

namespace gmp {
namespace player {
class MediaPlayerClient;
}
}  // namespace gmp

namespace base {
class SingleThreadTaskRunner;
}

namespace media {

class MEDIA_EXPORT MediaPlatformAPIWebOSGmp : public MediaPlatformAPI {
 public:
  static void Callback(const gint type,
                       const gint64 num_value,
                       const gchar* str_value,
                       void* user_data);

  MediaPlatformAPIWebOSGmp(
      const scoped_refptr<base::SingleThreadTaskRunner>& media_runner,
      bool video,
      const std::string& app_id,
      const NaturalVideoSizeChangedCB& natural_video_size_changed_cb,
      const base::Closure& resume_done_cb,
      const base::Closure& suspend_done_cb,
      const ActiveRegionCB& active_region_cb,
      const PipelineStatusCB& error_cb);

  ~MediaPlatformAPIWebOSGmp() override;

  // MediaPlatformAPI
  void Initialize(const AudioDecoderConfig& audio_config,
                  const VideoDecoderConfig& video_config,
                  const PipelineStatusCB& init_cb) override;
  void SetDisplayWindow(const gfx::Rect& rect,
                        const gfx::Rect& in_rect,
                        bool fullscreen) override;
  bool Feed(const scoped_refptr<DecoderBuffer>& buffer, FeedType type) override;
  void Seek(base::TimeDelta time) override;
  void Suspend(SuspendReason reason) override;
  void Resume(base::TimeDelta paused_time,
              RestorePlaybackMode restore_playback_mode) override;
  void SetPlaybackRate(float playback_rate) override;
  void SetPlaybackVolume(double volume) override;
  bool AllowedFeedVideo() override;
  bool AllowedFeedAudio() override;
  void Finalize() override;
  void SetKeySystem(const std::string& key_system) override {}
  bool IsEOSReceived() override;
  void UpdateVideoConfig(const VideoDecoderConfig& video_config) override;

  void SetVisibility(bool visible) override;

  void SwitchToAutoLayout() override {}
  void SetDisableAudio(bool disable) override {}
  bool HaveEnoughData() override;
  void SetPlayerEventCb(const PlayerEventCB& cb) override;
  void SetStatisticsCb(const StatisticsCB& cb) override;
  void SetUpdateCurrentTimeCb(const UpdateCurrentTimeCB& cb) override;

  void UpdateCurrentTime(const base::TimeDelta& time) override;
  // End of MediaPlatformAPI

 private:
  enum class State {
    INVALID = 0,
    CREATED,
    CREATED_SUSPENDED,
    LOADING,
    LOADED,
    PLAYING,
    PAUSED,
    SUSPENDED,
    RESUMING,
    SEEKING,
    RESTORING,
    FINALIZED
  };

  enum class RestoreDisplayWindowMode {
    DONT_RESTORE_DISPLAY_WINDOW = 0,
    RESTORE_DISPLAY_WINDOW
  };

  struct PendingSetDisplayWindow {
    bool was_set = false;
    gfx::Rect rect;
    gfx::Rect in_rect;
    bool fullscreen;
  };

  static const char* StateToString(State s);

  void DispatchCallback(const gint type,
                        const gint64 num_value,
                        const std::string& str_value);

  void PushEOS();
  void SetState(State);
  void PlayInternal();
  void PauseInternal(bool update_media = true);
  void SetVolumeInternal(double volume);

  enum FeedStatus { kFeedSucceeded, kFeedOverflowed, kFeedFailed };
  FeedStatus FeedInternal(const scoped_refptr<DecoderBuffer>& buffer,
                          FeedType type);

  void ResetFeedInfo();
  void SetMediaVideoData(const std::string& video_info);
  void ReInitialize(base::TimeDelta start_time);
  void NotifyLoadComplete();
  bool MakeLoadData(int64_t start_time, void* data);
  bool Loaded();
  std::string GetMediaID();
  bool IsReleasedMediaResource();
  void Unload();
  bool IsFeedableState() const;

  scoped_refptr<base::SingleThreadTaskRunner> media_task_runner_;
  std::string app_id_;

  gfx::Size natural_video_size_;
  NaturalVideoSizeChangedCB natural_video_size_changed_cb_;
  base::Closure resume_done_cb_;
  base::Closure suspend_done_cb_;
  ActiveRegionCB active_region_cb_;

  PipelineStatusCB error_cb_;
  PipelineStatusCB init_cb_;
  PipelineStatusCB seek_cb_;

  base::Closure size_change_cb_;

  std::recursive_mutex recursive_mutex_;

  State state_ = State::INVALID;

  base::TimeDelta feeded_audio_pts_ = kNoTimestamp;
  base::TimeDelta feeded_video_pts_ = kNoTimestamp;
  int64_t audio_eos_received_ = false;
  int64_t video_eos_received_ = false;
  double playback_volume_ = 1.0;
  bool received_eos_ = false;

  // class BufferQueue;
  std::unique_ptr<BufferQueue> buffer_queue_;

  gfx::Rect window_rect_;
  gfx::Rect window_in_rect_;

  float playback_rate_ = 0.0f;

  AudioDecoderConfig audio_config_;
  VideoDecoderConfig video_config_;

  bool play_internal_ = false;
  bool released_media_resource_ = false;
  bool is_destructed_ = false;
  bool is_suspended_ = false;
  bool load_completed_ = false;
  bool is_finalized_ = false;

  base::TimeDelta resume_time_;
  struct PendingSetDisplayWindow pending_set_display_window_;

  PlayerEventCB player_event_cb_;
  UpdateCurrentTimeCB update_current_time_cb_;

  std::unique_ptr<gmp::player::MediaPlayerClient> media_player_client_;

  DISALLOW_COPY_AND_ASSIGN(MediaPlatformAPIWebOSGmp);
};

}  // namespace media

#endif  // MEDIA_NEVA_WEBOS_MEDIA_PLATFORM_API_WEBOS_GMP_H_
