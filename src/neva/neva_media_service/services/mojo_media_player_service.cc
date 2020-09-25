// Copyright 2019-2020 LG Electronics, Inc.
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

#include "neva/neva_media_service/services/mojo_media_player_service.h"

#include "base/bind.h"
#include "base/callback.h"
#include "base/command_line.h"
#include "media/base/bind_to_current_loop.h"
#include "media/base/cdm_context.h"
#include "media/base/media_switches.h"
#include "media/base/pipeline.h"
#include "media/neva/media_player_neva_factory.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"
#include "ui/gfx/geometry/rect_f.h"

namespace neva_media {

MojoMediaPlayerService::MojoMediaPlayerService(
    media::MediaPlayerType media_player_type,
    const std::string& app_id) {
  // TODO(neva): It was designed that media players in MediaPlayerNeva are
  // working with main thread(main task runner) in renderer process. In neva
  // media service, this rule is violated. We call MojoMediaPlayerService ->
  // MediaPlayerNeva by 'unknown' thread, and run callback in created task
  // runner. So we need to make sure right way.

  media_player_neva_.reset(media::MediaPlayerNevaFactory::CreateMediaPlayerNeva(
      this, media_player_type,
      base::ThreadTaskRunnerHandle::Get(), app_id));
}

MojoMediaPlayerService::~MojoMediaPlayerService() {}

void MojoMediaPlayerService::Connect(ConnectCallback callback) {
  mojom::MediaPlayerListenerAssociatedPtr listener;
  mojom::MediaPlayerListenerAssociatedRequest request =
      mojo::MakeRequest(&(listener));
  listeners_.AddPtr(std::move(listener));
  std::move(callback).Run(std::move(request));
}

void MojoMediaPlayerService::Initialize(const bool is_video,
                                        const double current_time,
                                        const std::string& url,
                                        const std::string& mime,
                                        const std::string& referrer,
                                        const std::string& user_agent,
                                        const std::string& cookies,
                                        const std::string& media_option,
                                        const std::string& custom_option) {
  media_player_neva_->Initialize(is_video, current_time, url, mime, referrer,
                                 user_agent, cookies, media_option,
                                 custom_option);
}

void MojoMediaPlayerService::Start() {
  media_player_neva_->Start();
}

void MojoMediaPlayerService::Pause() {
  media_player_neva_->Pause();
}

void MojoMediaPlayerService::Seek(const base::TimeDelta time) {
  media_player_neva_->Seek(time);
}

void MojoMediaPlayerService::SetRate(double rate) {
  media_player_neva_->SetRate(rate);
}

void MojoMediaPlayerService::SetVolume(double volume) {
  media_player_neva_->SetVolume(volume);
}

void MojoMediaPlayerService::SetPoster(const GURL& poster) {
  media_player_neva_->SetPoster(poster);
}

void MojoMediaPlayerService::SetPreload(media::mojom::Preload preload) {
  media::MediaPlayerNeva::Preload converted_value;

  switch (preload) {
    case media::mojom::Preload::kPreloadNone:
      converted_value = media::MediaPlayerNeva::Preload::PreloadNone;
      break;
    case media::mojom::Preload::kPreloadMetaData:
      converted_value = media::MediaPlayerNeva::Preload::PreloadMetaData;
      break;
    case media::mojom::Preload::kPreloadAuto:
      converted_value = media::MediaPlayerNeva::Preload::PreloadAuto;
      break;
  }

  media_player_neva_->SetPreload(converted_value);
}

void MojoMediaPlayerService::IsPreloadable(
    const std::string& content_media_option,
    IsPreloadableCallback callback) {
  std::move(callback).Run(
      media_player_neva_->IsPreloadable(content_media_option));
}

void MojoMediaPlayerService::HasVideo(HasVideoCallback callback) {
  std::move(callback).Run(media_player_neva_->HasVideo());
}

void MojoMediaPlayerService::HasAudio(HasAudioCallback callback) {
  std::move(callback).Run(media_player_neva_->HasAudio());
}

void MojoMediaPlayerService::SelectTrack(const media::MediaTrackType type,
                                         const std::string& id) {
  media_player_neva_->SelectTrack(type, id);
}

void MojoMediaPlayerService::SwitchToAutoLayout() {
  media_player_neva_->SwitchToAutoLayout();
}

void MojoMediaPlayerService::SetDisplayWindow(const gfx::Rect& out_rect,
                                              const gfx::Rect& in_rect,
                                              bool full_screen,
                                              bool forced) {
  media_player_neva_->SetDisplayWindow(out_rect, in_rect, full_screen, forced);
}

void MojoMediaPlayerService::UsesIntrinsicSize(
    UsesIntrinsicSizeCallback callback) {
  std::move(callback).Run(media_player_neva_->UsesIntrinsicSize());
}

void MojoMediaPlayerService::MediaId(MediaIdCallback callback) {
  std::move(callback).Run(media_player_neva_->MediaId());
}

void MojoMediaPlayerService::HasAudioFocus(HasAudioFocusCallback callback) {
  std::move(callback).Run(media_player_neva_->HasAudioFocus());
}

void MojoMediaPlayerService::SetAudioFocus(bool focus) {
  media_player_neva_->SetAudioFocus(focus);
}

void MojoMediaPlayerService::HasVisibility(HasVisibilityCallback callback) {
  std::move(callback).Run(media_player_neva_->HasVisibility());
}

void MojoMediaPlayerService::SetVisibility(bool visibility) {
  media_player_neva_->SetVisibility(visibility);
}

void MojoMediaPlayerService::Suspend(media::SuspendReason reason) {
  media_player_neva_->Suspend(reason);
}

void MojoMediaPlayerService::Resume() {
  media_player_neva_->Resume();
}

void MojoMediaPlayerService::RequireMediaResource(
    RequireMediaResourceCallback callback) {
  std::move(callback).Run(media_player_neva_->RequireMediaResource());
}

void MojoMediaPlayerService::IsRecoverableOnResume(
    IsRecoverableOnResumeCallback callback) {
  std::move(callback).Run(media_player_neva_->IsRecoverableOnResume());
}

void MojoMediaPlayerService::SetDisableAudio(bool disable) {
  media_player_neva_->SetDisableAudio(disable);
}

void MojoMediaPlayerService::SetMediaLayerId(
    const std::string& media_layer_id) {
  media_player_neva_->SetMediaLayerId(media_layer_id);
}

void MojoMediaPlayerService::GetBufferedTimeRanges(
    GetBufferedTimeRangesCallback callback) {
  std::move(callback).Run(GetBufferedTimeRangesVector());
}

std::vector<neva_media::mojom::TimeDeltaPairPtr>
MojoMediaPlayerService::GetBufferedTimeRangesVector() const {
  std::vector<neva_media::mojom::TimeDeltaPairPtr> vector_ranges = {};
  media::Ranges<base::TimeDelta> ranges =
      media_player_neva_->GetBufferedTimeRanges();

  for (size_t i = 0; i < ranges.size(); i++)
    vector_ranges.push_back(
        mojom::TimeDeltaPair(ranges.start(i), ranges.end(i)).Clone());

  return vector_ranges;
}

void MojoMediaPlayerService::OnMediaMetadataChanged(base::TimeDelta duration,
                                                    int width,
                                                    int height,
                                                    bool success) {
  listeners_.ForAllPtrs([&](mojom::MediaPlayerListener* listener) {
    listener->OnMediaMetadataChanged(duration, width, height, success);
  });
}

void MojoMediaPlayerService::OnLoadComplete() {
  listeners_.ForAllPtrs(
      [](mojom::MediaPlayerListener* listener) { listener->OnLoadComplete(); });
}

void MojoMediaPlayerService::OnPlaybackComplete() {
  listeners_.ForAllPtrs([](mojom::MediaPlayerListener* listener) {
    listener->OnPlaybackComplete();
  });
}

void MojoMediaPlayerService::OnSeekComplete(
    const base::TimeDelta& current_time) {
  listeners_.ForAllPtrs([&current_time](mojom::MediaPlayerListener* listener) {
    listener->OnSeekComplete(current_time);
  });
}

void MojoMediaPlayerService::OnMediaError(int error) {
  listeners_.ForAllPtrs([&error](mojom::MediaPlayerListener* listener) {
    listener->OnMediaError(error);
  });
}

void MojoMediaPlayerService::OnVideoSizeChanged(int width, int height) {
  listeners_.ForAllPtrs([&](mojom::MediaPlayerListener* listener) {
    listener->OnVideoSizeChanged(width, height);
  });
}

void MojoMediaPlayerService::OnMediaPlayerPlay() {
  listeners_.ForAllPtrs([](mojom::MediaPlayerListener* listener) {
    listener->OnMediaPlayerPlay();
  });
}

void MojoMediaPlayerService::OnMediaPlayerPause() {
  listeners_.ForAllPtrs([](mojom::MediaPlayerListener* listener) {
    listener->OnMediaPlayerPause();
  });
}

void MojoMediaPlayerService::OnCustomMessage(
    const media::MediaEventType media_event_type,
    const std::string& detail) {
  listeners_.ForAllPtrs([&](mojom::MediaPlayerListener* listener) {
    listener->OnCustomMessage(media_event_type, detail);
  });
}

void MojoMediaPlayerService::OnBufferingUpdate(int percentage) {
  listeners_.ForAllPtrs([&percentage](mojom::MediaPlayerListener* listener) {
    listener->OnBufferingUpdate(percentage);
  });
}

void MojoMediaPlayerService::OnAudioTracksUpdated(
    const std::vector<media::MediaTrackInfo>& audio_track_info) {
  listeners_.ForAllPtrs(
      [&audio_track_info](mojom::MediaPlayerListener* listener) {
        listener->OnAudioTracksUpdated(audio_track_info);
      });
}

void MojoMediaPlayerService::OnTimeUpdate(base::TimeDelta current_timestamp,
                                          base::TimeTicks current_time_ticks) {
  listeners_.ForAllPtrs([&](mojom::MediaPlayerListener* listener) {
    listener->OnTimeUpdate(current_timestamp, base::TimeTicks::Now());
  });
}

void MojoMediaPlayerService::OnActiveRegionChanged(
    const gfx::Rect& active_region) {
  listeners_.ForAllPtrs([&active_region](mojom::MediaPlayerListener* listener) {
    listener->OnActiveRegionChanged(active_region);
  });
}

void MojoMediaPlayerService::OnAudioFocusChanged() {
  listeners_.ForAllPtrs([](mojom::MediaPlayerListener* listener) {
    listener->OnAudioFocusChanged();
  });
}

void MojoMediaPlayerService::Send(const std::string& message,
                                  SendCallback callback) {
  std::move(callback).Run(media_player_neva_->Send(message));
}

}  // namespace neva_media
