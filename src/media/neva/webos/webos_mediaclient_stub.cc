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

#include "media/neva/webos/webos_mediaclient_stub.h"

namespace media {

// static
std::unique_ptr<WebOSMediaClient> WebOSMediaClient::Create(
    const scoped_refptr<base::SingleThreadTaskRunner>& main_task_runner,
    base::WeakPtr<WebOSMediaClient::EventListener> event_listener,
    std::string& app_id) {
  return std::make_unique<WebOSMediaClientStub>();
}

WebOSMediaClientStub::WebOSMediaClientStub() {}

WebOSMediaClientStub::~WebOSMediaClientStub() {}

void WebOSMediaClientStub::Load(bool video,
                                double current_time,
                                bool is_local_source,
                                const std::string& url,
                                const std::string& mime_type,
                                const std::string& referrer,
                                const std::string& user_agent,
                                const std::string& cookies,
                                const std::string& payload) {}

void WebOSMediaClientStub::Seek(base::TimeDelta time,
                                const media::PipelineStatusCB& seek_cb) {}

float WebOSMediaClientStub::GetPlaybackRate() const {
  return 0.0f;
}

void WebOSMediaClientStub::SetPlaybackRate(float playback_rate) {}

double WebOSMediaClientStub::GetPlaybackVolume() const {
  return 0.0f;
}

void WebOSMediaClientStub::SetPlaybackVolume(double volume, bool forced) {}

bool WebOSMediaClientStub::SelectTrack(const MediaTrackType type,
                                       const std::string& id) {
  return false;
}

void WebOSMediaClientStub::Suspend(SuspendReason reason) {}

void WebOSMediaClientStub::Resume() {}

bool WebOSMediaClientStub::IsRecoverableOnResume() {
  return true;
}

void WebOSMediaClientStub::SetPreload(Preload preload) {}

bool WebOSMediaClientStub::IsPreloadable(
    const std::string& content_media_option) {
  return false;
}

std::string WebOSMediaClientStub::MediaId() {
  return std::string();
}

double WebOSMediaClientStub::GetDuration() const {
  return 0.0f;
}

void WebOSMediaClientStub::SetDuration(double duration) {}

double WebOSMediaClientStub::GetCurrentTime() {
  return 0.0f;
}

void WebOSMediaClientStub::SetCurrentTime(double time) {}

media::Ranges<base::TimeDelta> WebOSMediaClientStub::GetBufferedTimeRanges()
    const {
  return media::Ranges<base::TimeDelta>();
}

bool WebOSMediaClientStub::HasAudio() {
  return false;
}

bool WebOSMediaClientStub::HasVideo() {
  return false;
}

gfx::Size WebOSMediaClientStub::GetNaturalVideoSize() {
  return gfx::Size();
};

void WebOSMediaClientStub::SetNaturalVideoSize(const gfx::Size& size) {}

bool WebOSMediaClientStub::SetDisplayWindow(const gfx::Rect& outRect,
                                            const gfx::Rect& inRect,
                                            bool fullScreen,
                                            bool forced) {
  return false;
}

void WebOSMediaClientStub::SetVisibility(bool visible) {}

bool WebOSMediaClientStub::Visibility() {
  return false;
}

void WebOSMediaClientStub::SetFocus() {}

bool WebOSMediaClientStub::Focus() {
  return false;
}

void WebOSMediaClientStub::SwitchToAutoLayout() {}

bool WebOSMediaClientStub::DidLoadingProgress() {
  return false;
}

bool WebOSMediaClientStub::UsesIntrinsicSize() const {
  return true;
}

void WebOSMediaClientStub::Unload() {}

bool WebOSMediaClientStub::IsSupportedBackwardTrickPlay() {
  return false;
}

bool WebOSMediaClientStub::IsSupportedPreload() {
  return false;
}

bool WebOSMediaClientStub::CheckUseMediaPlayerManager(
    const std::string& media_option) {
  return false;
}

void WebOSMediaClientStub::SetDisableAudio(bool disable) {}

}  // namespace media
