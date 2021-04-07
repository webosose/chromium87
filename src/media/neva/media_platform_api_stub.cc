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

#include "media/neva/media_platform_api_stub.h"

namespace media {

MediaPlatformAPIStub::MediaPlatformAPIStub() {}

MediaPlatformAPIStub::~MediaPlatformAPIStub() {}

// static
scoped_refptr<MediaPlatformAPI> MediaPlatformAPI::Create(
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
    bool video,
    const std::string& app_id,
    const NaturalVideoSizeChangedCB& natural_video_size_changed_cb,
    const base::Closure& resume_done_cb,
    const base::Closure& suspend_done_cb,
    const ActiveRegionCB& active_region_cb,
    const PipelineStatusCB& error_cb) {
  return base::MakeRefCounted<MediaPlatformAPIStub>();
}

bool MediaPlatformAPI::IsAvailable() {
  return false;
}

void MediaPlatformAPIStub::Initialize(const AudioDecoderConfig& audio_config,
                                      const VideoDecoderConfig& video_config,
                                      const PipelineStatusCB& init_cb) {}

void MediaPlatformAPIStub::SetDisplayWindow(const gfx::Rect& rect,
                                            const gfx::Rect& in_rect,
                                            bool fullscreen) {}

bool MediaPlatformAPIStub::Feed(const scoped_refptr<DecoderBuffer>& buffer,
                                FeedType type) {
  return false;
}

void MediaPlatformAPIStub::Seek(base::TimeDelta time) {}

void MediaPlatformAPIStub::Suspend(SuspendReason reason) {}

void MediaPlatformAPIStub::Resume(base::TimeDelta paused_time,
                                  RestorePlaybackMode restore_playback_mode) {}

void MediaPlatformAPIStub::SetPlaybackRate(float playback_rate) {}

void MediaPlatformAPIStub::SetPlaybackVolume(double volume) {}

bool MediaPlatformAPIStub::AllowedFeedVideo() {
  return false;
}

bool MediaPlatformAPIStub::AllowedFeedAudio() {
  return false;
}

void MediaPlatformAPIStub::Finalize() {}

void MediaPlatformAPIStub::SetKeySystem(const std::string& key_system) {}

bool MediaPlatformAPIStub::IsEOSReceived() {
  return false;
}

void MediaPlatformAPIStub::UpdateVideoConfig(
    const VideoDecoderConfig& video_config) {}

void MediaPlatformAPIStub::SetVisibility(bool visible) {}

void MediaPlatformAPIStub::SwitchToAutoLayout() {}

void MediaPlatformAPIStub::SetDisableAudio(bool disable) {}

bool MediaPlatformAPIStub::HaveEnoughData() {
  return false;
}

}  // namespace media
