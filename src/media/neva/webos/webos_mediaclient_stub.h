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

#ifndef MEDIA_NEVA_WEBOS_WEBOS_MEDIACLIENT_STUB_H_
#define MEDIA_NEVA_WEBOS_WEBOS_MEDIACLIENT_STUB_H_

#include "base/memory/weak_ptr.h"
#include "media/neva/webos/webos_mediaclient.h"

namespace media {

class WebOSMediaClientStub
    : public WebOSMediaClient,
      public base::SupportsWeakPtr<WebOSMediaClientStub> {
 public:
  WebOSMediaClientStub();
  ~WebOSMediaClientStub() override;

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
  double GetPlaybackVolume() const override;
  void SetPlaybackVolume(double volume, bool forced = false) override;
  bool SelectTrack(const MediaTrackType type, const std::string& id) override;
  void Suspend(SuspendReason reason) override;
  void Resume() override;
  bool IsRecoverableOnResume() override;
  void SetPreload(Preload preload) override;
  bool IsPreloadable(const std::string& content_media_option) override;
  std::string MediaId() override;

  double GetDuration() const override;
  void SetDuration(double duration) override;
  double GetCurrentTime() override;
  void SetCurrentTime(double time) override;

  media::Ranges<base::TimeDelta> GetBufferedTimeRanges() const override;
  bool HasAudio() override;
  bool HasVideo() override;
  gfx::Size GetNaturalVideoSize() override;
  void SetNaturalVideoSize(const gfx::Size& size) override;

  bool SetDisplayWindow(const gfx::Rect& outRect,
                        const gfx::Rect& inRect,
                        bool fullScreen,
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
  bool CheckUseMediaPlayerManager(const std::string& media_option) override;
  void SetDisableAudio(bool disable) override;
};

}  // namespace media

#endif  // MEDIA_NEVA_WEBOS_WEBOS_MEDIACLIENT_STUB_H_
