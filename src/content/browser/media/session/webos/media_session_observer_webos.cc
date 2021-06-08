// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/media/session/webos/media_session_observer_webos.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/process/process.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "base/values.h"
#include "content/browser/media/session/media_session_impl.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/public/browser/media_session.h"
#include "media/base/bind_to_current_loop.h"
#include "services/media_session/public/cpp/media_position.h"
#include "services/media_session/public/mojom/audio_focus.mojom.h"
#include "third_party/blink/public/mojom/renderer_preferences.mojom.h"

using media_session::mojom::MediaPlaybackState;

namespace content {

namespace {

const char kAppId[] = "appId";
const char kMediaId[] = "mediaId";
const char kSubscribe[] = "subscribe";
const char kSubscribed[] = "subscribed";
const char kReturnValue[] = "returnValue";
const char kKeyEvent[] = "keyEvent";

const char kMediaMetaData[] = "mediaMetaData";
const char kMediaMetaDataTitle[] = "title";
const char kMediaMetaDataArtist[] = "artist";
const char kMediaMetaDataAlbum[] = "album";
const char kMediaMetaDataTotalDuration[] = "totalDuration";

const char kMediaPlayStatus[] = "playStatus";
const char kMediaPlayStatusStopped[] = "PLAYSTATE_STOPPED";
const char kMediaPlayStatusPaused[] = "PLAYSTATE_PAUSED";
const char kMediaPlayStatusPlaying[] = "PLAYSTATE_PLAYING";
const char kMediaPlayStatusNone[] = "PLAYSTATE_NONE";

const char kMediaPlayPosition[] = "playPosition";

const char kPlayEvent[] = "play";
const char kPauseEvent[] = "pause";
const char kNextEvent[] = "next";
const char kPreviousEvent[] = "previous";

const char kRegisterMediaSession[] = "registerMediaSession";
const char kUnregisterMediaSession[] = "unregisterMediaSession";
const char kActivateMediaSession[] = "activateMediaSession";
const char kDeactivateMediaSession[] = "deactivateMediaSession";
const char kSetMediaMetaData[] = "setMediaMetaData";
const char kSetMediaPlayStatus[] = "setMediaPlayStatus";
const char kSetMediaPlayPosition[] = "setMediaPlayPosition";

}  // namespace

#define BIND_TO_CURRENT_LOOP(function) \
  (media::BindToCurrentLoop(           \
      base::BindRepeating(function, base::Unretained(this))))

MediaSessionObserverWebOS::MediaSessionObserverWebOS(MediaSessionImpl* session)
    : media_session_(session) {
  DCHECK(media_session_);
  content::WebContents* web_contents =
      static_cast<WebContentsImpl*>(session->web_contents());

  DCHECK(web_contents);
  blink::mojom::RendererPreferences* renderer_prefs =
      web_contents->GetMutableRendererPrefs();

  DCHECK(renderer_prefs);
  luna_service_client_.reset(
      new base::LunaServiceClient(renderer_prefs->application_id));

  application_id_ = renderer_prefs->application_id + renderer_prefs->display_id;

  VLOG(0) << __func__ << " this: [" << this << "]"
            << " Application id: " << application_id_;

  session->AddObserver(observer_receiver_.BindNewPipeAndPassRemote());
}

MediaSessionObserverWebOS::~MediaSessionObserverWebOS() {
  VLOG(0) << __func__ << " this: [" << this << "]";
  UnregisterMediaSession();
}

void MediaSessionObserverWebOS::MediaSessionRequestChanged(
      const base::Optional<base::UnguessableToken>& request_id) {
  if (!session_id_.empty()) {
    // Previous session is active. Deactivate it.
    UnregisterMediaSession();
  }

  if (!request_id.has_value()) {
    LOG(ERROR) << __func__ << " Session id is not received";
    return;
  }

  if (!RegisterMediaSession(request_id->ToString())) {
    LOG(ERROR) << __func__ << " Register session failed for "
               << request_id->ToString();
    return;
  }

  if (!ActivateMediaSession(request_id->ToString())) {
    LOG(ERROR) << __func__ << " Activate session failed for "
               << request_id->ToString();
    return;
  }

  session_id_ = request_id->ToString();
}

void MediaSessionObserverWebOS::MediaSessionInfoChanged(
    media_session::mojom::MediaSessionInfoPtr session_info) {
  VLOG(1) << __func__ << " playback_state: " << session_info->playback_state;

  if (playback_state_ == session_info->playback_state)
    return;

  SetPlaybackStatusInternal(session_info->playback_state);
}

void MediaSessionObserverWebOS::MediaSessionMetadataChanged(
    const base::Optional<media_session::MediaMetadata>& metadata) {
  VLOG(1) << __func__;

  if (!metadata.has_value()) {
    LOG(ERROR) << __func__ << " Metadata is not received";
    return;
  }

  if (!metadata->title.empty())
    SetMetadataPropertyInternal(kMediaMetaDataTitle, metadata->title);

  if (!metadata->artist.empty())
    SetMetadataPropertyInternal(kMediaMetaDataArtist, metadata->artist);

  if (!metadata->album.empty())
    SetMetadataPropertyInternal(kMediaMetaDataAlbum, metadata->album);
}

void MediaSessionObserverWebOS::MediaSessionPositionChanged(
    const base::Optional<media_session::MediaPosition>& position) {
  VLOG(3) << __func__;

  if (!position.has_value()) {
    LOG(ERROR) << __func__ << "media position value is not available.";
    return;
  }

  SetMediaPositionInternal(position->GetPosition());

  base::TimeDelta new_duration = position->duration();
  if (duration_ == new_duration)
    return;

  duration_ = new_duration;
  SetMetadataPropertyInternal(kMediaMetaDataTotalDuration,
                              base::NumberToString16(duration_.InSecondsF()));
}

bool MediaSessionObserverWebOS::RegisterMediaSession(
    const std::string& session_id) {
  if (session_id.empty()) {
    LOG(ERROR) << __func__ << " Invalid session id";
    return false;
  }

  base::DictionaryValue register_root;
  register_root.SetKey(kMediaId, base::Value(session_id));
  register_root.SetKey(kAppId, base::Value(application_id_));
  register_root.SetKey(kSubscribe, base::Value(true));

  std::string register_payload;
  if (!base::JSONWriter::Write(register_root, &register_payload)) {
    LOG(ERROR) << __func__ << " Failed to write registMediaSession payload";
    return false;
  }

  VLOG(1) << __func__ << " payload: " << register_payload;
  luna_service_client_->Subscribe(
      base::LunaServiceClient::GetServiceURI(
          base::LunaServiceClient::URIType::MEDIACONTROLLER,
          kRegisterMediaSession),
      register_payload, &subscribe_key_,
      BIND_TO_CURRENT_LOOP(&MediaSessionObserverWebOS::HandleMediaKeyEvent));

  registered_ = true;
  return true;
}

void MediaSessionObserverWebOS::UnregisterMediaSession() {
  if (!registered_) {
    LOG(ERROR) << __func__ << " Session is already unregistered";
    return;
  }

  if (session_id_.empty()) {
    LOG(ERROR) << __func__ << " No registered session";
    return;
  }

  if (playback_state_ != MediaPlaybackState::kStopped)
    SetPlaybackStatusInternal(MediaPlaybackState::kStopped);

  base::DictionaryValue unregister_root;
  unregister_root.SetKey(kMediaId, base::Value(session_id_));

  std::string unregister_payload;
  if (!base::JSONWriter::Write(unregister_root, &unregister_payload)) {
    LOG(ERROR) << __func__ << " Failed to write unregistMediaSession payload";
    return;
  }

  VLOG(1) << __func__ << " payload: " << unregister_payload;
  luna_service_client_->CallAsync(
      base::LunaServiceClient::GetServiceURI(
          base::LunaServiceClient::URIType::MEDIACONTROLLER,
          kUnregisterMediaSession),
      unregister_payload,
      BIND_TO_CURRENT_LOOP(
          &MediaSessionObserverWebOS::CheckReplyStatusMessage));

  luna_service_client_->Unsubscribe(subscribe_key_);

  registered_ = false;
  session_id_ = std::string();
}

bool MediaSessionObserverWebOS::ActivateMediaSession(
    const std::string& session_id) {
  if (session_id.empty()) {
    LOG(ERROR) << __func__ << " Invalid session id";
    return false;
  }

  base::DictionaryValue activate_root;
  activate_root.SetKey(kMediaId, base::Value(session_id));

  std::string activate_payload;
  if (!base::JSONWriter::Write(activate_root, &activate_payload)) {
    LOG(ERROR) << __func__ << " Failed to write activateMediaSession payload";
    return false;
  }

  VLOG(1) << __func__ << " payload: " << activate_payload;
  luna_service_client_->CallAsync(
      base::LunaServiceClient::GetServiceURI(
          base::LunaServiceClient::URIType::MEDIACONTROLLER,
          kActivateMediaSession),
      activate_payload,
      BIND_TO_CURRENT_LOOP(
          &MediaSessionObserverWebOS::CheckReplyStatusMessage));

  return true;
}

void MediaSessionObserverWebOS::DeactivateMediaSession() {
  if (session_id_.empty()) {
    LOG(ERROR) << __func__ << " No active session";
    return;
  }

  base::DictionaryValue deactivate_root;
  deactivate_root.SetKey(kMediaId, base::Value(session_id_));

  std::string deactivate_payload;
  if (!base::JSONWriter::Write(deactivate_root, &deactivate_payload)) {
    LOG(ERROR) << __func__ << " Failed to write deactivateMediaSession payload";
    return;
  }

  VLOG(1) << __func__ << " payload: " << deactivate_payload;
  luna_service_client_->CallAsync(
      base::LunaServiceClient::GetServiceURI(
          base::LunaServiceClient::URIType::MEDIACONTROLLER,
          kDeactivateMediaSession),
      deactivate_payload,
      BIND_TO_CURRENT_LOOP(
          &MediaSessionObserverWebOS::CheckReplyStatusMessage));
}

void MediaSessionObserverWebOS::SetPlaybackStatusInternal(
    media_session::mojom::MediaPlaybackState playback_state) {
  if (session_id_.empty()) {
    LOG(ERROR) << __func__ << " No active session";
    return;
  }

  static std::map<MediaPlaybackState, std::string> kPlaybackStateMap = {
      {MediaPlaybackState::kPaused, kMediaPlayStatusPaused},
      {MediaPlaybackState::kPlaying, kMediaPlayStatusPlaying},
      {MediaPlaybackState::kStopped, kMediaPlayStatusStopped}};

  auto get_playback_status = [&](MediaPlaybackState status) {
    auto it = kPlaybackStateMap.find(status);
    if (it != kPlaybackStateMap.end())
      return it->second;
    return std::string(kMediaPlayStatusNone);
  };

  playback_state_ = playback_state;
  std::string play_status = get_playback_status(playback_state_);

  base::DictionaryValue playstatus_root;
  playstatus_root.SetKey(kMediaId, base::Value(session_id_));
  playstatus_root.SetKey(kMediaPlayStatus, base::Value(play_status));

  std::string playstatus_payload;
  if (!base::JSONWriter::Write(playstatus_root, &playstatus_payload)) {
    LOG(ERROR) << __func__ << " Failed to write setMediaPlayStatus payload";
    return;
  }

  VLOG(1) << __func__ << " payload: " << playstatus_payload;
  luna_service_client_->CallAsync(
      base::LunaServiceClient::GetServiceURI(
          base::LunaServiceClient::URIType::MEDIACONTROLLER,
          kSetMediaPlayStatus),
      playstatus_payload,
      BIND_TO_CURRENT_LOOP(
          &MediaSessionObserverWebOS::CheckReplyStatusMessage));
}

void MediaSessionObserverWebOS::SetMetadataPropertyInternal(
    const std::string& property,
    const base::string16& value) {
  base::DictionaryValue metadata;
  metadata.SetStringKey(property, base::UTF16ToUTF8(value));

  base::DictionaryValue metadata_root;
  metadata_root.SetStringKey(kMediaId, session_id_);
  metadata_root.SetKey(kMediaMetaData, std::move(metadata));

  std::string metadata_payload;
  if (!base::JSONWriter::Write(metadata_root, &metadata_payload)) {
    LOG(ERROR) << __func__ << " Failed to write setMediaMetaData payload";
    return;
  }

  VLOG(1) << __func__ << " payload: " << metadata_payload;
  luna_service_client_->CallAsync(
      base::LunaServiceClient::GetServiceURI(
          base::LunaServiceClient::URIType::MEDIACONTROLLER, kSetMediaMetaData),
      metadata_payload,
      BIND_TO_CURRENT_LOOP(
          &MediaSessionObserverWebOS::CheckReplyStatusMessage));
}

void MediaSessionObserverWebOS::SetMediaPositionInternal(
    const base::TimeDelta& position) {
  if (session_id_.empty()) {
    LOG(ERROR) << __func__ << " No active session.";
    return;
  }

  base::DictionaryValue playposition_root;
  playposition_root.SetStringKey(kMediaId, session_id_);
  playposition_root.SetStringKey(kMediaPlayPosition,
                                 std::to_string(position.InSecondsF()));

  std::string playposition_payload;
  if (!base::JSONWriter::Write(playposition_root, &playposition_payload)) {
    LOG(ERROR) << __func__ << " Failed to write Play Position payload";
    return;
  }
  VLOG(1) << __func__ << " playposition_payload: " << playposition_payload;

  luna_service_client_->CallAsync(
      base::LunaServiceClient::GetServiceURI(
          base::LunaServiceClient::URIType::MEDIACONTROLLER,
          kSetMediaPlayPosition),
      playposition_payload,
      BIND_TO_CURRENT_LOOP(
          &MediaSessionObserverWebOS::CheckReplyStatusMessage));
}

void MediaSessionObserverWebOS::HandleMediaKeyEvent(
    const std::string& payload) {
  VLOG(1) << __func__ << " payload: " << payload;
  base::Optional<base::Value> value = base::JSONReader::Read(payload);
  if (!value) {
    return;
  }

  auto response = base::DictionaryValue::From(
      base::Value::ToUniquePtrValue(std::move(*value)));

  auto return_value = response->FindBoolPath(kReturnValue);
  auto subscribed = response->FindBoolPath(kSubscribed);

  if (!return_value || !*return_value || !subscribed || !*subscribed) {
    LOG(ERROR) << __func__
               << " Failed to Register with MCS, session_id: " << session_id_;
    return;
  }

  const std::string* session_id = response->FindStringPath(kMediaId);

  if (!session_id || session_id->empty()) {
    LOG(WARNING) << __func__ << " Session ID is not sent";
    return;
  }

  if (session_id_ != *session_id) {
    VLOG(1) << __func__ << " Event recieved for other session. Ignore.";
    return;
  }

  const std::string* key_event = response->FindStringPath(kKeyEvent);
  if (key_event) {
    HandleMediaKeyEventInternal(key_event);
  } else {
    VLOG(0) << __func__ << " Successfully Registered with MCS, session_id: "
              << session_id_;
  }
}

void MediaSessionObserverWebOS::CheckReplyStatusMessage(
    const std::string& message) {
  VLOG(1) << __func__ << " message: " << message;
  base::Optional<base::Value> value = base::JSONReader::Read(message);
  if (!value) {
    return;
  }

  auto response = base::DictionaryValue::From(
      base::Value::ToUniquePtrValue(std::move(*value)));

  auto return_value = response->FindBoolPath(kReturnValue);
  if (!return_value || !*return_value) {
    LOG(ERROR) << __func__ << " MCS call Failed. message: " << message
               << " session_id: " << session_id_;
    return;
  }

  VLOG(1) << __func__ << " MCS call Success. message: " << message
          << " session_id: " << session_id_;
}

void MediaSessionObserverWebOS::HandleMediaKeyEventInternal(
    const std::string* key_event) {
  VLOG(0) << __func__ << " key_event: " << *key_event;

  static std::map<std::string, MediaKeyEvent> kEventKeyMap = {
      {kPlayEvent, MediaSessionObserverWebOS::MediaKeyEvent::kPlay},
      {kPauseEvent, MediaSessionObserverWebOS::MediaKeyEvent::kPause},
      {kNextEvent, MediaSessionObserverWebOS::MediaKeyEvent::kNext},
      {kPreviousEvent, MediaSessionObserverWebOS::MediaKeyEvent::kPrevious}};

  auto get_event_type = [&](const std::string* key) {
    std::map<std::string, MediaKeyEvent>::iterator it;
    it = kEventKeyMap.find(*key);
    if (it != kEventKeyMap.end())
      return it->second;
    return MediaSessionObserverWebOS::MediaKeyEvent::kUnsupported;
  };

  if (media_session_) {
    MediaKeyEvent event_type = get_event_type(key_event);
    switch (event_type) {
      case MediaSessionObserverWebOS::MediaKeyEvent::kPlay:
        media_session_->Resume(MediaSession::SuspendType::kUI);
        break;
      case MediaSessionObserverWebOS::MediaKeyEvent::kPause:
        media_session_->Suspend(MediaSession::SuspendType::kUI);
        break;
      case MediaSessionObserverWebOS::MediaKeyEvent::kNext:
        media_session_->NextTrack();
        break;
      case MediaSessionObserverWebOS::MediaKeyEvent::kPrevious:
        media_session_->PreviousTrack();
        break;
      default:
        NOTREACHED() << " key_event: " << key_event << " Not Handled !!!";
        break;
    }
  }
}

}  // namespace content
