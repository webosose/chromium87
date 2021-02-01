// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_MEDIA_SESSION_WEBOS_MEDIA_SESSION_OBSERVER_WEBOS_H_
#define CONTENT_BROWSER_MEDIA_SESSION_WEBOS_MEDIA_SESSION_OBSERVER_WEBOS_H_

#include <string>

#include "base/neva/webos/luna_service_client.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "services/media_session/public/mojom/media_session.mojom.h"

namespace base {
class UnguessableToken;
}  // namespace base

namespace content {

class MediaSessionImpl;
class WebContentsImpl;

// This class is interlayer between native MediaSession and MCS
// MediaSession. This class is owned by the native MediaSession and will
// unregister MCS MediaSession when the native MediaSession is destroyed.
class MediaSessionObserverWebOS final
    : public media_session::mojom::MediaSessionObserver {
 public:
  explicit MediaSessionObserverWebOS(MediaSessionImpl* session);
  MediaSessionObserverWebOS(const MediaSessionObserverWebOS&) = delete;
  MediaSessionObserverWebOS& operator=(const MediaSessionObserverWebOS&) = delete;
  ~MediaSessionObserverWebOS() override;

  // media_session::mojom::MediaSessionObserver implementation:
  void MediaSessionRequestChanged(
      const base::Optional<base::UnguessableToken>& request_id) override;
  void MediaSessionInfoChanged(
      media_session::mojom::MediaSessionInfoPtr session_info) override;
  void MediaSessionMetadataChanged(
      const base::Optional<media_session::MediaMetadata>& metadata) override;
  void MediaSessionPositionChanged(
      const base::Optional<media_session::MediaPosition>& position) override;
  void MediaSessionActionsChanged(
      const std::vector<media_session::mojom::MediaSessionAction>& action)
      override {}
  void MediaSessionImagesChanged(
      const base::flat_map<media_session::mojom::MediaSessionImageType,
                           std::vector<media_session::MediaImage>>& images)
      override {}

 private:
  enum class MediaKeyEvent {
    kUnsupported = 0,
    kPlay,
    kPause,
    kNext,
    kPrevious,
  };

  // Registers media session with MCS
  bool RegisterMediaSession(const std::string& session_id);

  // Unregisters media session with MCS
  void UnregisterMediaSession();

  // Activates media session with MCS
  bool ActivateMediaSession(const std::string& session_id);

  // Deactivates media session with MCS
  void DeactivateMediaSession();

  // Sets the current Playback Status to MCS.
  void SetPlaybackStatusInternal(
      media_session::mojom::MediaPlaybackState playback_state);

  // Sets a value on the Metadata property and sends to MCS if necessary.
  void SetMetadataPropertyInternal(const std::string& property,
                                   const base::string16& value);

  // Receives the response from MCS and takes necessary action.
  void HandleMediaKeyEvent(const std::string& payload);

  // Checks the status of the MCS request.
  void CheckReplyStatusMessage(const std::string& message);

  // Handles the key events received.
  void HandleMediaKeyEventInternal(const std::string* key_event);

  // True if we have registered to com.webos.service.mediacontroller service.
  bool registered_ = false;

  MediaSessionImpl* const media_session_;

  std::string application_id_;
  std::string session_id_;

  LSMessageToken subscribe_key_ = 0;
  std::unique_ptr<base::LunaServiceClient> luna_service_client_;

  media_session::mojom::MediaPlaybackState playback_state_ =
      media_session::mojom::MediaPlaybackState::kStopped;

  mojo::Receiver<media_session::mojom::MediaSessionObserver> observer_receiver_{
      this};
};

}  // namespace content

#endif  // CONTENT_BROWSER_MEDIA_SESSION_WEBOS_MEDIA_SESSION_OBSERVER_WEBOS_H_
