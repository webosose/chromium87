// Copyright 2017-2020 LG Electronics, Inc.
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

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_HTML_MEDIA_NEVA_HTML_MEDIA_ELEMENT_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_HTML_MEDIA_NEVA_HTML_MEDIA_ELEMENT_H_

#include <stdio.h>

#include <type_traits>

#include "base/time/time.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "third_party/blink/public/common/browser_interface_broker_proxy.h"
#include "third_party/blink/public/common/widget/screen_info.h"
#include "third_party/blink/public/platform/web_media_player_client.h"
#include "third_party/blink/public/platform/web_rect.h"
#include "third_party/blink/public/web/web_view_client.h"
#include "third_party/blink/public/web/web_widget_client.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/events/custom_event.h"
#include "third_party/blink/renderer/core/dom/events/event_queue.h"
#include "third_party/blink/renderer/core/exported/web_view_impl.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/settings.h"
#include "third_party/blink/renderer/core/frame/web_frame_widget_base.h"
#include "third_party/blink/renderer/core/frame/web_local_frame_impl.h"
#include "third_party/blink/renderer/core/html/time_ranges.h"
#include "third_party/blink/renderer/core/loader/cookie_jar.h"
#include "third_party/blink/renderer/core/page/page.h"
#include "third_party/blink/renderer/platform/bindings/to_v8.h"
#include "third_party/blink/renderer/platform/network/mime/content_type.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"
#include "third_party/blink/renderer/platform/weborigin/security_policy.h"

namespace blink {
namespace neva {

template <typename original_t>
class HTMLMediaElement {
 public:
  HTMLMediaElement();

  ScriptValue getStartDate(ScriptState* script_state) const;
  double GetStartDate() const;
  const String mediaId() const;
  bool IsPlayed() const;

  const IntRect VideoRectInScreen() const;
  const IntRect WidgetViewRect() const;

  // Neva audio focus extensions
  bool webosMediaFocus() const;
  void setWebosMediaFocus(bool focus);

  bool send(const String& message);

 protected:
  void ScheduleEvent(const AtomicString& event_name, const String& detail);
  void ParseContentType(const ContentType& contentType);

  // platform(ex webos) media framework updates the time at 200ms intervals.
  // Set the follwing in consideration of the overlap interval.
  void SetMaxTimeupdateEventFrequency();

  bool cached_audio_focus_ : 1;

  // Holds the "timeline offset" as described in the HTML5 spec. Represents the
  // number of seconds since January 1, 1970 or NaN if no offset is in effect.
  double m_timelineOffset;

  // The spec says to fire periodic timeupdate events (those sent while playing)
  // every "15 to 250ms". we choose the slowest frequency
  base::TimeDelta kMaxTimeupdateEventFrequency;

  String m_contentMIMEType;
  String m_contentTypeCodecs;
  String m_contentTypeDecoder;
  String m_contentCustomOption;
  String m_contentMediaOption;
  blink::WebMediaPlayer::RenderMode m_renderMode;
  bool m_support_preload_none_on_mse;

  base::Optional<bool> has_noaudio_attr_ = base::nullopt;
};

template <typename original_t>
HTMLMediaElement<original_t>::HTMLMediaElement()
    : cached_audio_focus_(false),
      m_timelineOffset(std::numeric_limits<double>::quiet_NaN()),
      m_renderMode(blink::WebMediaPlayer::RenderModeHole),
      m_support_preload_none_on_mse(
          RuntimeEnabledFeatures::SupportPreloadNoneOnMSEEnabled()) {}

template <typename original_t>
ScriptValue HTMLMediaElement<original_t>::getStartDate(
    ScriptState* script_state) const {
  // getStartDate() returns a Date instance.
  return ScriptValue(
      script_state->GetIsolate(),
      ToV8(base::Time::FromJsTime(GetStartDate()), script_state));
}

template <typename original_t>
double HTMLMediaElement<original_t>::GetStartDate() const {
  return m_timelineOffset;
}

template <typename original_t>
const IntRect HTMLMediaElement<original_t>::VideoRectInScreen() const {
  const original_t* self(static_cast<const original_t*>(this));

  if (!self->GetLayoutObject())
    return IntRect();

  // FIXME: this should probably respect transforms
  return self->GetDocument().View()->FrameToScreen(
      self->GetLayoutObject()->AbsoluteBoundingBoxRect());
}

template <typename original_t>
const IntRect HTMLMediaElement<original_t>::WidgetViewRect() const {
  const original_t* self(static_cast<const original_t*>(this));

  LocalFrame* frame = self->GetDocument().GetFrame();

  if (!frame)
    return IntRect();

  WebFrameWidgetBase* frame_widget =
      WebLocalFrameImpl::FromFrame(frame)->LocalRootFrameWidget();

  if (!frame_widget)
    return IntRect();

  return IntRect(frame_widget->ViewRect());
}

template <typename original_t>
const String HTMLMediaElement<original_t>::mediaId() const {
  const original_t* self(static_cast<const original_t*>(this));
  DCHECK(RuntimeEnabledFeatures::UMSExtensionEnabled());

  if (self->GetWebMediaPlayer())
    return self->GetWebMediaPlayer()->MediaId();
  return String();
}

template <typename original_t>
bool HTMLMediaElement<original_t>::IsPlayed() const {
  const original_t* self(static_cast<const original_t*>(this));
  bool result = self->playing_;

  if (self->played_time_ranges_ && self->played_time_ranges_->length() != 0)
    result = true;
  return result;
}

template <typename original_t>
bool HTMLMediaElement<original_t>::webosMediaFocus() const {
  DCHECK(RuntimeEnabledFeatures::AudioFocusExtensionEnabled());

  const original_t* self(static_cast<const original_t*>(this));

  if (self->GetWebMediaPlayer())
    return self->GetWebMediaPlayer()->HasAudioFocus();
  return cached_audio_focus_;
}

template <typename original_t>
void HTMLMediaElement<original_t>::setWebosMediaFocus(bool focus) {
  DCHECK(RuntimeEnabledFeatures::AudioFocusExtensionEnabled());

  original_t* self(static_cast<original_t*>(this));

  if (self->GetWebMediaPlayer())
    self->GetWebMediaPlayer()->SetAudioFocus(focus);
}

template <typename original_t>
void HTMLMediaElement<original_t>::ScheduleEvent(const AtomicString& event_name,
                                                 const String& detail) {
  original_t* self(static_cast<original_t*>(this));

  LocalFrame* frame = self->GetDocument().GetFrame();
  if (!frame) {
    LOG(ERROR) << "Document has no frame";
    return;
  }

  ScriptState* script_state = ToScriptStateForMainWorld(frame);
  if (!script_state) {
    LOG(ERROR) << "ScriptState is null";
    return;
  }

  blink::CustomEvent* event = CustomEvent::Create();
  ScriptState::Scope script_scope(script_state);
  event->initCustomEvent(script_state, event_name, false, true,
                         ScriptValue::From(script_state, detail));
  event->SetTarget(self);
  self->async_event_queue_->EnqueueEvent(FROM_HERE, *event);
}

template <typename original_t>
void HTMLMediaElement<original_t>::ParseContentType(
    const ContentType& contentType) {
  DEFINE_STATIC_LOCAL(const String, codecs, ("codecs"));
  DEFINE_STATIC_LOCAL(const String, decoder, ("decoder"));
  DEFINE_STATIC_LOCAL(const String, mediaOption, ("mediaOption"));
  DEFINE_STATIC_LOCAL(const String, cameraOption, ("cameraOption"));

  m_contentMIMEType = contentType.GetType().LowerASCII();
  m_contentTypeCodecs = contentType.Parameter(codecs);
  m_contentTypeDecoder = contentType.Parameter(decoder);

  m_contentMediaOption = DecodeURLEscapeSequences(
      contentType.Parameter(mediaOption), DecodeURLMode::kUTF8OrIsomorphic);
  if (!m_contentMediaOption.IsEmpty())
    VLOG(1) << "mediaOption=[" << m_contentMediaOption.Utf8().data() << "]";
  if (m_contentMIMEType == "service/webos-camera")
    m_contentCustomOption = DecodeURLEscapeSequences(
        contentType.Parameter(cameraOption), DecodeURLMode::kUTF8OrIsomorphic);
}

template <typename original_t>
void HTMLMediaElement<original_t>::SetMaxTimeupdateEventFrequency() {
  const original_t* self(static_cast<const original_t*>(this));

  LocalFrame* frame = self->GetDocument().GetFrame();
  if (frame) {
    Settings* settings = frame->GetSettings();
    if (settings) {
      kMaxTimeupdateEventFrequency = base::TimeDelta::FromMilliseconds(
          settings->GetMaxTimeupdateEventFrequency());
    }
  }
}

template <typename original_t>
bool HTMLMediaElement<original_t>::send(const String& message) {
  const original_t* self(static_cast<const original_t*>(this));
  DCHECK(RuntimeEnabledFeatures::SendMethodEnabled());

  if (self->GetWebMediaPlayer())
    return self->GetWebMediaPlayer()->Send(std::string(message.Utf8().data()));

  return false;
}

template <typename original_t>
class HTMLMediaElementExtendingWebMediaPlayerClient
    : public blink::WebMediaPlayerClient {
 public:
  void SendCustomMessage(const blink::WebMediaPlayer::MediaEventType,
                         const blink::WebString&) override;
  WebString ContentMIMEType() const override;
  WebString ContentTypeCodecs() const override;
  WebString ContentTypeDecoder() const override;
  WebString ContentCustomOption() const override;
  WebString ContentMediaOption() const override;
  WebString Referrer() const override;
  WebString UserAgent() const override;
  WebString Cookies() const override;
  base::Optional<bool> IsAudioDisabled() const override;
  bool IsVideo() const override;
  bool IsSuppressedMediaPlay() const override;
  blink::WebMediaPlayer::LoadType LoadType() const override;
  WebRect ScreenRect() override;
  WebMediaPlayer::RenderMode RenderMode() const override;
  WebRect WebWidgetViewRect() override;

  // Neva audio focus extension
  void OnAudioFocusChanged() override;
};

template <typename original_t>
WebRect
HTMLMediaElementExtendingWebMediaPlayerClient<original_t>::WebWidgetViewRect() {
  const original_t* self(static_cast<original_t*>(this));
  return self->WidgetViewRect();
}

template <typename original_t>
void HTMLMediaElementExtendingWebMediaPlayerClient<
    original_t>::SendCustomMessage(const blink::WebMediaPlayer::MediaEventType
                                       media_event_type,
                                   const blink::WebString& detail) {
  original_t* self(static_cast<original_t*>(this));
  switch (media_event_type) {
    case blink::WebMediaPlayer::kMediaEventUpdateUMSMediaInfo:
      if (RuntimeEnabledFeatures::UMSExtensionEnabled())
        self->ScheduleEvent(event_type_names::kUmsmediainfo, detail);
      break;
    case blink::WebMediaPlayer::kMediaEventBroadcastErrorMsg:
      if (RuntimeEnabledFeatures::CustomEventExtensionEnabled())
        self->ScheduleEvent(event_type_names::kBroadcasterrormsg, detail);
      break;
    case blink::WebMediaPlayer::kMediaEventDvrErrorMsg:
      if (RuntimeEnabledFeatures::CustomEventExtensionEnabled())
        self->ScheduleEvent(event_type_names::kDvrerrormsg, detail);
      break;
    case blink::WebMediaPlayer::kMediaEventUpdateCameraState:
      if (RuntimeEnabledFeatures::CustomEventExtensionEnabled())
        self->ScheduleEvent(event_type_names::kUpdatecamerastate, detail);
      break;
    case blink::WebMediaPlayer::kMediaEventPipelineStarted:
      if (RuntimeEnabledFeatures::CustomEventExtensionEnabled())
        self->ScheduleEvent(event_type_names::kPipelinestarted, detail);
      break;
    default:
      break;
  }
}

template <typename original_t>
WebString HTMLMediaElementExtendingWebMediaPlayerClient<
    original_t>::ContentMIMEType() const {
  const original_t* self(static_cast<const original_t*>(this));

  return self->m_contentMIMEType;
}

template <typename original_t>
WebString HTMLMediaElementExtendingWebMediaPlayerClient<
    original_t>::ContentTypeCodecs() const {
  const original_t* self(static_cast<const original_t*>(this));

  return self->m_contentTypeCodecs;
}

template <typename original_t>
WebString HTMLMediaElementExtendingWebMediaPlayerClient<
    original_t>::ContentTypeDecoder() const {
  const original_t* self(static_cast<const original_t*>(this));

  return self->m_contentTypeDecoder;
}

template <typename original_t>
WebString HTMLMediaElementExtendingWebMediaPlayerClient<
    original_t>::ContentCustomOption() const {
  const original_t* self(static_cast<const original_t*>(this));

  return self->m_contentCustomOption;
}

template <typename original_t>
WebString HTMLMediaElementExtendingWebMediaPlayerClient<
    original_t>::ContentMediaOption() const {
  const original_t* self(static_cast<const original_t*>(this));

  return self->m_contentMediaOption;
}

template <typename original_t>
WebString HTMLMediaElementExtendingWebMediaPlayerClient<original_t>::Referrer()
    const {
  const original_t* self(static_cast<const original_t*>(this));

  ExecutionContext* context = self->GetDocument().GetExecutionContext();

  return SecurityPolicy::GenerateReferrer(context->GetReferrerPolicy(),
                                          self->currentSrc(),
                                          context->OutgoingReferrer())
      .referrer;
}

template <typename original_t>
WebString HTMLMediaElementExtendingWebMediaPlayerClient<original_t>::UserAgent()
    const {
  const original_t* self(static_cast<const original_t*>(this));

  LocalFrame* frame = self->GetDocument().GetFrame();
  if (!frame)
    return String();

  return frame->Loader().UserAgent();
}

template <typename original_t>
WebString HTMLMediaElementExtendingWebMediaPlayerClient<original_t>::Cookies()
    const {
  const original_t* self(static_cast<const original_t*>(this));

  Document* doc = self->ownerDocument();

  if (!doc || !doc->GetFrame() || !doc->TopFrameOrigin())
    return String();

  if (&doc->GetFrame()->GetBrowserInterfaceBroker() ==
      &GetEmptyBrowserInterfaceBroker()) {
    LOG(ERROR) << __func__ << " empty broker";
    return String();
  }

  mojo::Remote<network::mojom::blink::RestrictedCookieManager> backend;
  doc->GetFrame()->GetBrowserInterfaceBroker().GetInterface(
      backend.BindNewPipeAndPassReceiver(
          doc->GetTaskRunner(TaskType::kMiscPlatformAPI)));

  const SecurityOrigin* security_origin =
      doc->GetFrame()->GetSecurityContext()->GetSecurityOrigin();
  if (security_origin &&
      !security_origin->IsSameOriginWith(
          SecurityOrigin::Create(self->currentSrc()).get())) {
    return String();
  }

  String value;
  backend->GetCookiesString(self->currentSrc(), doc->SiteForCookies(),
                            doc->TopFrameOrigin(), &value);

  return value;
}

template <typename original_t>
base::Optional<bool> HTMLMediaElementExtendingWebMediaPlayerClient<
    original_t>::IsAudioDisabled() const {
  const original_t* self(static_cast<const original_t*>(this));

  return self->has_noaudio_attr_;
}

template <typename original_t>
bool HTMLMediaElementExtendingWebMediaPlayerClient<original_t>::IsVideo()
    const {
  const original_t* self(static_cast<const original_t*>(this));

  return self->IsHTMLVideoElement();
}

template <typename original_t>
bool HTMLMediaElementExtendingWebMediaPlayerClient<
    original_t>::IsSuppressedMediaPlay() const {
  const original_t* self(static_cast<const original_t*>(this));
  LocalFrame* frame = self->GetDocument().GetFrame();
  if (frame) {
    WebLocalFrameImpl* weblocalframeimpl = WebLocalFrameImpl::FromFrame(frame);
    if (weblocalframeimpl)
      return weblocalframeimpl->IsSuppressedMediaPlay();
  }

  return false;
}

template <typename original_t>
blink::WebMediaPlayer::LoadType
HTMLMediaElementExtendingWebMediaPlayerClient<original_t>::LoadType() const {
  const original_t* self(static_cast<const original_t*>(this));

  return self->GetLoadType();
}

template <typename original_t>
WebRect
HTMLMediaElementExtendingWebMediaPlayerClient<original_t>::ScreenRect() {
  original_t* self(static_cast<original_t*>(this));
  LocalFrame* frame = self->GetDocument().GetFrame();

  if (!frame)
    return WebRect();

  // We may consider screen_info.available_rect if screen_info.rect gives
  // incorrect value.
  return WebRect(
      frame->GetPage()->GetChromeClient().GetScreenInfo(*frame).rect);
}

template <typename original_t>
WebMediaPlayer::RenderMode
HTMLMediaElementExtendingWebMediaPlayerClient<original_t>::RenderMode() const {
  const original_t* self(static_cast<const original_t*>(this));
  if (self->IsHTMLVideoElement())
    return self->m_renderMode;
  return blink::WebMediaPlayer::RenderModeDefault;
}

template <typename original_t>
void HTMLMediaElementExtendingWebMediaPlayerClient<
    original_t>::OnAudioFocusChanged() {
  original_t* self(static_cast<original_t*>(this));

  if (!RuntimeEnabledFeatures::AudioFocusExtensionEnabled())
    return;

  if (self->GetWebMediaPlayer())
    self->cached_audio_focus_ = self->GetWebMediaPlayer()->HasAudioFocus();

  self->ScheduleEvent(event_type_names::kWebosmediafocuschange);
}

}  // namespace neva
}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_HTML_MEDIA_NEVA_HTML_MEDIA_ELEMENT_H_
