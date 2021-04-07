// Copyright 2017-2018 LG Electronics, Inc.
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

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_NEVA_WEB_MEDIA_PLAYER_CLIENT_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_NEVA_WEB_MEDIA_PLAYER_CLIENT_H_

#include "base/optional.h"
#include "third_party/blink/public/platform/web_media_player.h"
#include "third_party/blink/public/platform/web_rect.h"
#include "third_party/blink/public/platform/web_string.h"

namespace blink {
namespace neva {

class WebMediaPlayerClient {
 public:
  virtual WebString ContentMIMEType() const { return WebString(); }
  virtual WebString ContentTypeCodecs() const { return WebString(); }
  virtual WebString ContentTypeDecoder() const { return WebString(); }
  virtual WebString ContentCustomOption() const { return WebString(); }
  virtual WebString ContentMediaOption() const { return WebString(); }
  virtual WebString Referrer() const { return WebString(); }
  virtual WebString UserAgent() const { return WebString(); }
  virtual WebString Cookies() const { return WebString(); }
  virtual base::Optional<bool> IsAudioDisabled() const { return base::nullopt; }
  virtual bool IsVideo() const { return false; }
  virtual bool IsSuppressedMediaPlay() const { return false; }
  virtual blink::WebMediaPlayer::LoadType LoadType() const {
    return blink::WebMediaPlayer::LoadType::kLoadTypeURL;
  }
  virtual WebRect ScreenRect() { return WebRect(); }
  virtual WebMediaPlayer::RenderMode RenderMode() const {
    return WebMediaPlayer::RenderMode::RenderModeDefault;
  }
  virtual WebRect WebWidgetViewRect() { return WebRect(); }
  virtual void OnAudioFocusChanged() {}
  virtual void SendCustomMessage(const blink::WebMediaPlayer::MediaEventType,
                                 const WebString&) = 0;

 protected:
  ~WebMediaPlayerClient() = default;
};

}  // namespace neva
}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_PLATFORM_NEVA_WEB_MEDIA_PLAYER_CLIENT_H_
