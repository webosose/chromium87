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

#include "media/webrtc/neva/webos/webrtc_video_encoder_stub.h"

namespace media {

// static
WebRtcVideoEncoder* WebRtcVideoEncoder::Create(
    scoped_refptr<base::SingleThreadTaskRunner> main_task_runner,
    scoped_refptr<base::SingleThreadTaskRunner> encode_task_runner,
    webrtc::VideoCodecType video_codec_type,
    webrtc::VideoContentType video_content_type) {
  return new WebRtcVideoEncoderStub(
      encode_task_runner, video_codec_type, video_content_type);
}

// static
bool WebRtcVideoEncoder::IsCodecTypeSupported(
    webrtc::VideoCodecType type) {
  return false;
}

// static
const char* WebRtcVideoEncoder::ImplementationName() {
  return "WebOSEncoderStub";
}

WebRtcVideoEncoderStub::WebRtcVideoEncoderStub(
    scoped_refptr<base::SingleThreadTaskRunner> encode_task_runner,
    webrtc::VideoCodecType video_codec_type,
    webrtc::VideoContentType video_content_type)
    : WebRtcVideoEncoder(encode_task_runner, video_codec_type,
                         video_content_type) {}

WebRtcVideoEncoderStub::~WebRtcVideoEncoderStub() = default;

void WebRtcVideoEncoderStub::CreateAndInitialize(
    const gfx::Size& input_visible_size,
    uint32_t bitrate,
    uint32_t framerate,
    base::WaitableEvent* async_waiter,
    int32_t* async_retval) {
  RegisterAsyncWaiter(async_waiter, async_retval);
  SetStatus(WEBRTC_VIDEO_CODEC_OK);
  SignalAsyncWaiter(WEBRTC_VIDEO_CODEC_OK);
}

void WebRtcVideoEncoderStub::EncodeFrame(const webrtc::VideoFrame* frame,
                                         bool force_keyframe,
                                         base::WaitableEvent* async_waiter,
                                         int32_t* async_retval) {
  RegisterAsyncWaiter(async_waiter, async_retval);
  SignalAsyncWaiter(WEBRTC_VIDEO_CODEC_OK);
}

}  // namespace media
