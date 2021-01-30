// Copyright 2020 LG Electronics, Inc.
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

#include "media/webrtc/neva/webrtc_pass_through_video_encoder.h"

#include "base/bind.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/threading/thread_restrictions.h"
#include "media/webrtc/neva/webos/webrtc_video_encoder_webos.h"
#include "third_party/blink/renderer/platform/scheduler/public/post_cross_thread_task.h"
#include "third_party/blink/renderer/platform/wtf/cross_thread_copier.h"
#include "third_party/blink/renderer/platform/wtf/cross_thread_functional.h"
#include "third_party/webrtc/api/video/video_codec_constants.h"
#include "third_party/webrtc/api/video_codecs/sdp_video_format.h"
#include "third_party/webrtc/modules/video_coding/utility/simulcast_utility.h"
#include "third_party/webrtc/modules/video_coding/include/video_error_codes.h"

namespace WTF {
template <>
struct CrossThreadCopier<webrtc::VideoEncoder::RateControlParameters>
    : public CrossThreadCopierPassThrough<
          webrtc::VideoEncoder::RateControlParameters> {
  STATIC_ONLY(CrossThreadCopier);
};
}  // namespace WTF

namespace media {

media::VideoEncodeAccelerator::SupportedProfiles
WebRtcPassThroughVideoEncoder::GetSupportedEncodeProfiles() {
  media::VideoEncodeAccelerator::SupportedProfiles supported_profiles;

  gfx::Size max_resolution(1920, 1080);
  if (WebRtcVideoEncoder::IsCodecTypeSupported(webrtc::kVideoCodecVP8)) {
    supported_profiles.push_back(
        VideoEncodeAccelerator::SupportedProfile(media::VP8PROFILE_ANY,
                                                 max_resolution, 0u, 1u));
  }

  if (WebRtcVideoEncoder::IsCodecTypeSupported(webrtc::kVideoCodecVP9)) {
    supported_profiles.push_back(
        VideoEncodeAccelerator::SupportedProfile(media::VP9PROFILE_PROFILE0,
                                                 max_resolution, 0u, 1u));
  }

  if (WebRtcVideoEncoder::IsCodecTypeSupported(webrtc::kVideoCodecH264)) {
    supported_profiles.push_back(
        VideoEncodeAccelerator::SupportedProfile(media::H264PROFILE_BASELINE,
                                                 max_resolution, 0u, 1u));
  }
  return supported_profiles;
}

WebRtcPassThroughVideoEncoder::WebRtcPassThroughVideoEncoder(
    scoped_refptr<base::SingleThreadTaskRunner> main_task_runner)
    : main_task_runner_(main_task_runner) {
  LOG(INFO) << __func__ << "[" << this << "] ";
}

WebRtcPassThroughVideoEncoder::~WebRtcPassThroughVideoEncoder() {
  LOG(INFO) << __func__ << "[" << this << "] ";
  Release();
}

int WebRtcPassThroughVideoEncoder::InitEncode(
    const webrtc::VideoCodec* codec,
    const webrtc::VideoEncoder::Settings& settings) {
  LOG(INFO) << __func__ << " codecType=" << codec->codecType
                        << ", width=" << codec->width
                        << ", height=" << codec->height
                        << ", startBitrate=" << codec->startBitrate;

  if (!encode_task_runner_) {
    encode_task_runner_ = GetEncodeTaskRunner();

    if (!encode_task_runner_) {
      LOG(ERROR) << __func__ << " Failed to start Encoding thread.";
      return WEBRTC_VIDEO_CODEC_FALLBACK_SOFTWARE;
    }
  }

  if (video_encoder_)
    Release();

  if (codec->codecType == webrtc::kVideoCodecVP8 &&
      codec->mode == webrtc::VideoCodecMode::kScreensharing &&
      codec->VP8().numberOfTemporalLayers > 1) {
    // This is a VP8 stream with screensharing using temporal layers for
    // temporal scalability. Since this implementation does not yet implement
    // temporal layers, fall back to software codec.
    return WEBRTC_VIDEO_CODEC_FALLBACK_SOFTWARE;
  }

  if (codec->codecType == webrtc::kVideoCodecVP9 &&
      codec->VP9().numberOfSpatialLayers > 1) {
    VLOG(1)
        << "VP9 SVC not yet supported by HW codecs, falling back to sofware.";
    return WEBRTC_VIDEO_CODEC_FALLBACK_SOFTWARE;
  }

  video_encoder_ = media::WebRtcVideoEncoder::Create(
      main_task_runner_, encode_task_runner_, codec->codecType,
      (codec->mode == webrtc::VideoCodecMode::kScreensharing)
          ? webrtc::VideoContentType::SCREENSHARE
          : webrtc::VideoContentType::UNSPECIFIED);

  base::ScopedAllowBaseSyncPrimitivesOutsideBlockingScope allow_wait;
  base::WaitableEvent initialization_waiter(
      base::WaitableEvent::ResetPolicy::MANUAL,
      base::WaitableEvent::InitialState::NOT_SIGNALED);
  int32_t initialization_retval = WEBRTC_VIDEO_CODEC_UNINITIALIZED;
  blink::PostCrossThreadTask(
      *encode_task_runner_.get(), FROM_HERE,
      WTF::CrossThreadBindOnce(
          &WebRtcVideoEncoder::CreateAndInitialize,
          scoped_refptr<WebRtcVideoEncoder>(video_encoder_),
          gfx::Size(codec->width, codec->height),
          codec->startBitrate, codec->maxFramerate,
          WTF::CrossThreadUnretained(&initialization_waiter),
          WTF::CrossThreadUnretained(&initialization_retval)));

  // webrtc::VideoEncoder expects this call to be synchronous.
  initialization_waiter.Wait();
  return initialization_retval;
}

int32_t WebRtcPassThroughVideoEncoder::Encode(
    const webrtc::VideoFrame& input_frame,
    const std::vector<webrtc::VideoFrameType>* frame_types) {
  DVLOG(3) << __func__;
  if (!video_encoder_.get()) {
    LOG(ERROR) << __func__ << "Error encoder not initialized";
    return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
  }

  bool want_key_frame = false;
  if (frame_types) {
    for (const auto& frame_type : *frame_types) {
      if (frame_type == webrtc::VideoFrameType::kVideoFrameKey) {
        want_key_frame = true;
        break;
      }
    }
  }

  base::ScopedAllowBaseSyncPrimitivesOutsideBlockingScope allow_wait;
  base::WaitableEvent encode_waiter(
      base::WaitableEvent::ResetPolicy::MANUAL,
      base::WaitableEvent::InitialState::NOT_SIGNALED);
  int32_t encode_retval = WEBRTC_VIDEO_CODEC_UNINITIALIZED;
  blink::PostCrossThreadTask(
      *encode_task_runner_.get(), FROM_HERE,
      WTF::CrossThreadBindOnce(&WebRtcVideoEncoder::EncodeFrame,
          scoped_refptr<WebRtcVideoEncoder>(video_encoder_),
          WTF::CrossThreadUnretained(&input_frame), want_key_frame,
          WTF::CrossThreadUnretained(&encode_waiter),
          WTF::CrossThreadUnretained(&encode_retval)));

  // webrtc::VideoEncoder expects this call to be synchronous.
  encode_waiter.Wait();
  return encode_retval;
}

int32_t WebRtcPassThroughVideoEncoder::RegisterEncodeCompleteCallback(
    webrtc::EncodedImageCallback* callback) {
  DVLOG(3) << __func__;

  if (!video_encoder_.get()) {
    LOG(ERROR) << __func__ << "Error encoder not initialized";
    return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
  }

  base::ScopedAllowBaseSyncPrimitivesOutsideBlockingScope allow_wait;
  base::WaitableEvent register_waiter(
      base::WaitableEvent::ResetPolicy::MANUAL,
      base::WaitableEvent::InitialState::NOT_SIGNALED);
  int32_t register_retval = WEBRTC_VIDEO_CODEC_UNINITIALIZED;
  blink::PostCrossThreadTask(
      *encode_task_runner_.get(), FROM_HERE,
      WTF::CrossThreadBindOnce(
          &WebRtcVideoEncoder::RegisterEncodeCompleteCallback,
          scoped_refptr<WebRtcVideoEncoder>(video_encoder_),
          WTF::CrossThreadUnretained(&register_waiter),
          WTF::CrossThreadUnretained(&register_retval),
          WTF::CrossThreadUnretained(callback)));
  register_waiter.Wait();
  return register_retval;
}

int32_t WebRtcPassThroughVideoEncoder::Release() {
  DVLOG(3) << __func__;
  if (!video_encoder_.get()) {
    LOG(ERROR) << __func__ << "Error encoder not initialized";
    return WEBRTC_VIDEO_CODEC_OK;
  }

  base::ScopedAllowBaseSyncPrimitivesOutsideBlockingScope allow_wait;
  base::WaitableEvent release_waiter(
      base::WaitableEvent::ResetPolicy::MANUAL,
      base::WaitableEvent::InitialState::NOT_SIGNALED);
  blink::PostCrossThreadTask(
      *encode_task_runner_.get(), FROM_HERE,
      WTF::CrossThreadBindOnce(&WebRtcVideoEncoder::Destroy,
          scoped_refptr<WebRtcVideoEncoder>(video_encoder_),
          WTF::CrossThreadUnretained(&release_waiter)));

  release_waiter.Wait();
  video_encoder_ = nullptr;
  return WEBRTC_VIDEO_CODEC_OK;
}

void WebRtcPassThroughVideoEncoder::SetRates(
    const RateControlParameters& parameters) {
  DVLOG(3) << __func__ << " new_bit_rate=" << parameters.bitrate.ToString()
           << ", frame_rate=" << parameters.framerate_fps;
  if (!video_encoder_.get()) {
    LOG(ERROR) << __func__ << "Error encoder not initialized";
    return;
  }

  const int32_t retval = video_encoder_->GetStatus();
  if (retval != WEBRTC_VIDEO_CODEC_OK) {
    LOG(ERROR) << __func__ << " returning " << retval;
    return;
  }

  blink::PostCrossThreadTask(
      *encode_task_runner_.get(), FROM_HERE,
      WTF::CrossThreadBindOnce(
          &WebRtcVideoEncoder::RequestEncodingParametersChange,
          scoped_refptr<WebRtcVideoEncoder>(video_encoder_),
          parameters));
  return;
}

webrtc::VideoEncoder::EncoderInfo
WebRtcPassThroughVideoEncoder::GetEncoderInfo() const {
  EncoderInfo info;
  info.implementation_name = WebRtcVideoEncoder::ImplementationName();
  info.supports_native_handle = false;
  info.is_hardware_accelerated = true;
  info.has_internal_source = false;
  return info;
}

scoped_refptr<base::SingleThreadTaskRunner>
WebRtcPassThroughVideoEncoder::GetEncodeTaskRunner() {
  DVLOG(3) << __func__;
  if (!encoding_thread_) {
    encoding_thread_.reset(new base::Thread("Encoder"));
    // Start thread in IO mode to make it usable for encoding.
    base::Thread::Options options(base::MessagePumpType::IO, 0);
    encoding_thread_->StartWithOptions(options);
  }
  return encoding_thread_->task_runner();
}

}  // namespace media
