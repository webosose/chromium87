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

#ifndef MEDIA_NEVA_WEBOS_MEDIA_PLATFORM_API_WEBOS_STUB_H_
#define MEDIA_NEVA_WEBOS_MEDIA_PLATFORM_API_WEBOS_STUB_H_

#include "media/neva/media_platform_api.h"
#include "ui/gfx/geometry/rect.h"

namespace media {

class MEDIA_EXPORT MediaPlatformAPIWebOSStub : public MediaPlatformAPI {
 public:
  typedef base::Callback<void(const gfx::Rect&)> ActiveRegionCB;
  MediaPlatformAPIWebOSStub();
  ~MediaPlatformAPIWebOSStub() override;

  void Initialize(const AudioDecoderConfig& audio_config,
                  const VideoDecoderConfig& video_config,
                  const PipelineStatusCB& init_cb) override;
  void SwitchToAutoLayout() override;
  void SetDisableAudio(bool disable) override;
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
  void SetKeySystem(const std::string& key_system) override;
  bool IsEOSReceived() override;
  void UpdateVideoConfig(const VideoDecoderConfig& video_config) override;
  void SetVisibility(bool visible) override;
  bool HaveEnoughData() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(MediaPlatformAPIWebOSStub);
};

}  // namespace media

#endif  // MEDIA_NEVA_WEBOS_MEDIA_PLATFORM_API_WEBOS_STUB_H_
