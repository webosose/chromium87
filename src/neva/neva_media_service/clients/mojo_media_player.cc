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

#include "neva/neva_media_service/clients/mojo_media_player.h"

#include "base/bind.h"
#include "base/callback.h"
#include "base/command_line.h"
#include "media/base/bind_to_current_loop.h"
#include "media/base/cdm_context.h"
#include "media/base/media_switches.h"
#include "media/base/pipeline.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "ui/gfx/geometry/rect_f.h"

namespace neva_media {

#define BIND_TO_RENDER_LOOP(function)                   \
  (DCHECK(main_task_runner_->BelongsToCurrentThread()), \
   media::BindToCurrentLoop(base::Bind(function, AsWeakPtr())))

MojoMediaPlayer::MojoMediaPlayer(
    mojo::PendingRemote<neva_media::mojom::MediaServiceProvider>
        pending_provider,
    media::MediaPlayerNevaClient* client,
    media::MediaPlayerType media_player_type,
    const scoped_refptr<base::SingleThreadTaskRunner>& main_task_runner,
    const std::string& app_id)
    : client_(client),
      main_task_runner_(main_task_runner),
      client_receiver_(this) {
  DVLOG(1) << __func__;

  mojo::Remote<neva_media::mojom::MediaServiceProvider> provider(
      std::move(pending_provider));

  provider->CreateMediaPlayer(media_player_type, app_id,
                              media_player_.BindNewPipeAndPassReceiver());
  media_player_->Connect(base::BindRepeating(&MojoMediaPlayer::OnConnected,
                                             base::Unretained(this)));
}

MojoMediaPlayer::~MojoMediaPlayer() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
}

void MojoMediaPlayer::Initialize(const bool is_video,
                                 const double current_time,
                                 const std::string& url,
                                 const std::string& mime_type,
                                 const std::string& referrer,
                                 const std::string& user_agent,
                                 const std::string& cookies,
                                 const std::string& media_option,
                                 const std::string& custom_option) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(1) << __func__ << " / url: " << url
           << " / media_option: " << media_option;

  if (media_player_)
    media_player_->Initialize(is_video, current_time, url, mime_type, referrer,
                              user_agent, cookies, media_option, custom_option);
}

void MojoMediaPlayer::Start() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(1) << __func__;
  if (media_player_)
    media_player_->Start();
}

void MojoMediaPlayer::Pause() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(1) << __func__;
  if (media_player_)
    media_player_->Pause();
}

void MojoMediaPlayer::Seek(const base::TimeDelta& time) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(1) << __func__;
  if (media_player_)
    media_player_->Seek(time);
}

void MojoMediaPlayer::SetVolume(double volume) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(1) << __func__;
  if (media_player_)
    media_player_->SetVolume(volume);
}

void MojoMediaPlayer::SetPoster(const GURL& poster) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(1) << __func__;
}

void MojoMediaPlayer::SetRate(double rate) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(1) << __func__;
  if (media_player_)
    media_player_->SetRate(rate);
}

void MojoMediaPlayer::SetPreload(MediaPlayerNeva::Preload preload) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(1) << __func__;
  if (!media_player_)
    return;
  switch (preload) {
    case MediaPlayerNeva::PreloadNone:
      media_player_->SetPreload(media::mojom::Preload::kPreloadNone);
      break;
    case MediaPlayerNeva::PreloadMetaData:
      media_player_->SetPreload(media::mojom::Preload::kPreloadMetaData);
      break;
    case MediaPlayerNeva::PreloadAuto:
      media_player_->SetPreload(media::mojom::Preload::kPreloadAuto);
      break;
    default:
      break;
  }
}

bool MojoMediaPlayer::IsPreloadable(const std::string& content_media_option) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(1) << __func__;
  bool result;
  if (media_player_ &&
      media_player_->IsPreloadable(content_media_option, &result))
    return result;
  return false;
}

bool MojoMediaPlayer::HasVideo() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(1) << __func__;
  bool result;
  if (media_player_ && media_player_->HasVideo(&result))
    return result;
  return false;
}

bool MojoMediaPlayer::HasAudio() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(1) << __func__;
  bool result;
  if (media_player_ && media_player_->HasAudio(&result))
    return result;
  return false;
}

bool MojoMediaPlayer::SelectTrack(const media::MediaTrackType type,
                                  const std::string& id) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(1) << __func__;
  if (!media_player_)
    return false;
  media_player_->SelectTrack(type, id);
  return true;
}

void MojoMediaPlayer::SwitchToAutoLayout() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(1) << __func__;
  if (media_player_)
    media_player_->SwitchToAutoLayout();
}

void MojoMediaPlayer::SetDisplayWindow(const gfx::Rect& out_rect,
                                       const gfx::Rect& in_rect,
                                       bool fullscreen,
                                       bool forced) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(1) << __func__ << "out_rect:" << out_rect.ToString()
           << " in_rect:" << in_rect.ToString();
  if (media_player_)
    media_player_->SetDisplayWindow(out_rect, in_rect, fullscreen, forced);
}

bool MojoMediaPlayer::UsesIntrinsicSize() const {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  bool result;
  if (media_player_ && media_player_->UsesIntrinsicSize(&result))
    return result;
  return false;
}

std::string MojoMediaPlayer::MediaId() const {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(1) << __func__;
  std::string result = "";
  if (media_player_)
    media_player_->MediaId(&result);
  return result;
}

bool MojoMediaPlayer::HasAudioFocus() const {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(1) << __func__;
  bool result;
  if (media_player_ && media_player_->HasAudioFocus(&result))
    return result;
  return false;
}

void MojoMediaPlayer::SetAudioFocus(bool focus) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(1) << __func__;
  if (media_player_)
    media_player_->SetAudioFocus(focus);
}

bool MojoMediaPlayer::HasVisibility() const {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(1) << __func__;
  bool result;
  if (media_player_ && media_player_->HasVisibility(&result))
    return result;
  return false;
}

void MojoMediaPlayer::SetVisibility(bool visibility) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(1) << __func__ << " visibility=" << visibility;
  if (media_player_)
    media_player_->SetVisibility(visibility);
}

void MojoMediaPlayer::Suspend(media::SuspendReason reason) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(1) << __func__;
  if (media_player_)
    media_player_->Suspend(reason);
}

void MojoMediaPlayer::Resume() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(1) << __func__;
  if (media_player_)
    media_player_->Resume();
}

bool MojoMediaPlayer::RequireMediaResource() const {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  bool result;
  if (media_player_ && media_player_->RequireMediaResource(&result))
    return result;
  return false;
}

bool MojoMediaPlayer::IsRecoverableOnResume() const {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  bool result;
  if (media_player_ && media_player_->IsRecoverableOnResume(&result))
    return result;
  return false;
}

void MojoMediaPlayer::SetDisableAudio(bool disable) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  if (media_player_)
    media_player_->SetDisableAudio(disable);
}

void MojoMediaPlayer::SetMediaLayerId(const std::string& media_layer_id) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  if (media_player_)
    media_player_->SetMediaLayerId(media_layer_id);
}

media::Ranges<base::TimeDelta> MojoMediaPlayer::GetBufferedTimeRanges() const {
  DCHECK(main_task_runner_->BelongsToCurrentThread());

  media::Ranges<base::TimeDelta> ranges = {};
  std::vector<neva_media::mojom::TimeDeltaPairPtr> vector_ranges = {};

  if (media_player_) {
    media_player_->GetBufferedTimeRanges(&vector_ranges);
    for (const auto& r : vector_ranges) {
      ranges.Add(r->start, r->end);
    }
  }
  return ranges;
}

void MojoMediaPlayer::OnMediaPlayerPlay() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  client_->OnMediaPlayerPlay();
}

void MojoMediaPlayer::OnMediaPlayerPause() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  client_->OnMediaPlayerPause();
}

void MojoMediaPlayer::OnPlaybackComplete() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  client_->OnPlaybackComplete();
}

void MojoMediaPlayer::OnMediaError(int error) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  client_->OnMediaError(error);
}

void MojoMediaPlayer::OnSeekComplete(base::TimeDelta current_time) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  client_->OnSeekComplete(current_time);
}

void MojoMediaPlayer::OnMediaMetadataChanged(base::TimeDelta duration,
                                             uint32_t width,
                                             uint32_t height,
                                             bool success) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  client_->OnMediaMetadataChanged(duration, width, height, success);
}

void MojoMediaPlayer::OnLoadComplete() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  client_->OnLoadComplete();
}

void MojoMediaPlayer::OnVideoSizeChanged(uint32_t width, uint32_t height) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  client_->OnVideoSizeChanged(width, height);
}

void MojoMediaPlayer::OnCustomMessage(
    const media::MediaEventType media_event_type,
    const std::string& detail) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  client_->OnCustomMessage(media_event_type, detail);
}

void MojoMediaPlayer::OnBufferingUpdate(int percentage) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  client_->OnBufferingUpdate(percentage);
}

void MojoMediaPlayer::OnTimeUpdate(base::TimeDelta current_timestamp,
                                   base::TimeTicks current_time_ticks) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  client_->OnTimeUpdate(current_timestamp, current_time_ticks);
}

void MojoMediaPlayer::OnAudioTracksUpdated(
    const std::vector<media::MediaTrackInfo>& audio_track_info) {
  client_->OnAudioTracksUpdated(audio_track_info);
}

void MojoMediaPlayer::OnAudioFocusChanged() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  client_->OnAudioFocusChanged();
}

void MojoMediaPlayer::OnActiveRegionChanged(const gfx::Rect& active_region) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  client_->OnActiveRegionChanged(active_region);
}

void MojoMediaPlayer::OnConnected(
    mojo::PendingAssociatedReceiver<neva_media::mojom::MediaPlayerListener>
        receiver) {
  client_receiver_.Bind(std::move(receiver));
}

bool MojoMediaPlayer::Send(const std::string& message) const {
  DVLOG(1) << __func__ << " , message: " << message;
  bool result;
  if (media_player_ && media_player_->Send(message, &result))
    return result;
  return false;
}

}  // namespace neva_media
