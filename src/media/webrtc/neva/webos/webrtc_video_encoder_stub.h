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

#ifndef MEDIA_WEBRTC_NEVA_WEBOS_WEBRTC_VIDEO_ENCODER_STUB_H_
#define MEDIA_WEBRTC_NEVA_WEBOS_WEBRTC_VIDEO_ENCODER_STUB_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "media/webrtc/neva/webrtc_video_encoder.h"

namespace media {

class WebRtcVideoEncoderStub : public WebRtcVideoEncoder {
 public:
  WebRtcVideoEncoderStub(
      scoped_refptr<base::SingleThreadTaskRunner> encode_task_runner,
      webrtc::VideoCodecType video_codec_type,
      webrtc::VideoContentType video_content_type);
  WebRtcVideoEncoderStub(const WebRtcVideoEncoderStub&) = delete;
  WebRtcVideoEncoderStub& operator=(const WebRtcVideoEncoderStub&) = delete;
  ~WebRtcVideoEncoderStub();

  // Implements WebRtcVideoEncoder
 void CreateAndInitialize(const gfx::Size& input_visible_size,
                           uint32_t bitrate,
                           uint32_t framerate,
                           base::WaitableEvent* async_waiter,
                           int32_t* async_retval) override;
  void EncodeFrame(const webrtc::VideoFrame* frame,
                   bool force_keyframe,
                   base::WaitableEvent* async_waiter,
                   int32_t* async_retval) override;
  void RequestEncodingParametersChange(
      const webrtc::VideoEncoder::RateControlParameters& parameters) override {}
  void Destroy(base::WaitableEvent* async_waiter) override {}
};

}  // namespace media

#endif  // MEDIA_WEBRTC_NEVA_WEBOS_WEBRTC_VIDEO_ENCODER_STUB_H_
