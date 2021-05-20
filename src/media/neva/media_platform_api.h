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

#ifndef MEDIA_NEVA_MEDIA_PLATFORM_API_H
#define MEDIA_NEVA_MEDIA_PLATFORM_API_H

#include <queue>
#include <set>
#include "base/optional.h"
#include "base/synchronization/condition_variable.h"
#include "base/synchronization/lock.h"
#include "media/base/audio_decoder_config.h"
#include "media/base/decoder_buffer.h"
#include "media/base/pipeline_status.h"
#include "media/base/video_decoder_config.h"
#include "media/neva/media_codec_capability.h"
#include "media/neva/media_constants.h"
#include "ui/gfx/geometry/rect.h"

namespace base {
class SingleThreadTaskRunner;
}

namespace media {
class MEDIA_EXPORT MediaPlatformAPI
    : public base::RefCountedThreadSafe<media::MediaPlatformAPI> {
 public:
  using ActiveRegionCB = base::Callback<void(const gfx::Rect&)>;
  using NaturalVideoSizeChangedCB = base::Callback<void(const gfx::Size&)>;
  using PlayerEventCB = base::Callback<void(PlayerEvent)>;
  using UpdateCurrentTimeCB =
      base::RepeatingCallback<void(const base::TimeDelta&)>;

  MediaPlatformAPI();

  static scoped_refptr<MediaPlatformAPI> Create(
      const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
      bool video,
      const std::string& app_id,
      const NaturalVideoSizeChangedCB& natural_video_size_changed_cb,
      const base::Closure& resume_done_cb,
      const base::Closure& suspend_done_cb,
      const ActiveRegionCB& active_region_cb,
      const PipelineStatusCB& error_cb);

  // Return true if the implementation of MediaPlatformAPI is available to play
  // media.
  static bool IsAvailable();

  virtual void Initialize(const AudioDecoderConfig& audio_config,
                          const VideoDecoderConfig& video_config,
                          const PipelineStatusCB& init_cb) = 0;

  // rect and in_rect should not empty.
  virtual void SetDisplayWindow(const gfx::Rect& rect,
                                const gfx::Rect& in_rect,
                                bool fullscreen) = 0;
  virtual bool Feed(const scoped_refptr<DecoderBuffer>& buffer,
                    FeedType type) = 0;
  virtual void Seek(base::TimeDelta time) = 0;
  virtual void Suspend(SuspendReason reason) = 0;
  virtual void Resume(base::TimeDelta paused_time,
                      RestorePlaybackMode restore_playback_mode) = 0;
  virtual void SetPlaybackRate(float playback_rate) = 0;
  virtual void SetPlaybackVolume(double volume) = 0;
  virtual bool AllowedFeedVideo() = 0;
  virtual bool AllowedFeedAudio() = 0;
  virtual void Finalize() = 0;
  virtual void SetKeySystem(const std::string& key_system) = 0;
  virtual bool IsEOSReceived() = 0;
  virtual void UpdateVideoConfig(const VideoDecoderConfig& video_config) {}

  virtual void SetVisibility(bool visible) = 0;

  virtual void SwitchToAutoLayout() = 0;
  virtual void SetDisableAudio(bool disable) = 0;
  virtual bool HaveEnoughData() = 0;
  virtual void SetMediaLayerId(const std::string& media_layer_id);
  virtual void SetMediaPreferences(const std::string& preferences);

  virtual base::TimeDelta GetCurrentTime();

  // Note: Returning reference is important. Some downstream implementations
  // may use get_media_layer_id().c_str() without copying the memory. So we
  // should ensure this value will not be freed.
  const std::string& get_media_layer_id() const { return media_layer_id_; }

  virtual void SetPlayerEventCb(const PlayerEventCB& cb) {}
  virtual void SetStatisticsCb(const StatisticsCB& cb) {}

  virtual void SetUpdateCurrentTimeCb(const UpdateCurrentTimeCB& cb) {}
  virtual void FinalizeMediaPlatformAPI() {}

  virtual void SetMediaCodecCapabilities(
      const std::vector<MediaCodecCapability>& capabilities);

 protected:
  virtual ~MediaPlatformAPI();
  virtual void UpdateCurrentTime(const base::TimeDelta& time);

  class BufferQueue {
   public:
    BufferQueue();
    ~BufferQueue();
    void Push(const scoped_refptr<DecoderBuffer>& buffer, FeedType type);
    const std::pair<scoped_refptr<DecoderBuffer>, FeedType> Front();
    void Pop();
    void Clear();

    bool Empty();
    size_t DataSize() const;

   private:
    using DecoderBufferPair = std::pair<scoped_refptr<DecoderBuffer>, FeedType>;
    using DecoderBufferQueue = std::queue<DecoderBufferPair>;
    DecoderBufferQueue queue_;
    size_t data_size_;

    BufferQueue(const BufferQueue&) = delete;
    BufferQueue& operator=(const BufferQueue&) = delete;
  };

  // Find a capability for a given |codec|. Note that this differs with
  // MediaPreferences::GetMediaCodecCapabilityFor'Type'().
  base::Optional<MediaCodecCapability> GetMediaCodecCapabilityForCodec(
      const std::string& codec);

  std::unique_ptr<BufferQueue> buffer_queue_;

  std::string preferences_;

 private:
  friend class base::RefCountedThreadSafe<MediaPlatformAPI>;

  // Should be set 0 sec before updating by pipeline
  base::TimeDelta current_time_ = base::TimeDelta::FromSeconds(0);
  // Used access current_time_
  base::Lock current_time_lock_;

  std::string media_layer_id_;

  std::vector<MediaCodecCapability> capabilities_;

  MediaPlatformAPI(const MediaPlatformAPI&) = delete;
  MediaPlatformAPI& operator=(const MediaPlatformAPI&) = delete;
};

using CreateMediaPlatformAPICB =
    base::RepeatingCallback<decltype(MediaPlatformAPI::Create)>;

}  // namespace media

#endif  // MEDIA_NEVA_MEDIA_PLATFORM_API_H
