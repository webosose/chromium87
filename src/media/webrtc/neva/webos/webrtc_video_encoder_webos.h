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

#ifndef MEDIA_WEBRTC_NEVA_WEBOS_WEBRTC_VIDEO_ENCODER_WEBOS_H_
#define MEDIA_WEBRTC_NEVA_WEBOS_WEBRTC_VIDEO_ENCODER_WEBOS_H_

#include <glib.h>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "media/webrtc/neva/webrtc_video_encoder.h"

namespace mcil::encoder {
class VideoEncoderAPI;
}  // namespace mcil::encoder

namespace webrtc {
class EncodedImage;
}  // namespace webrtc

namespace media {

class VideoFrame;

class WebRtcVideoEncoderWebOS : public WebRtcVideoEncoder {
 public:
  WebRtcVideoEncoderWebOS(
      scoped_refptr<base::SingleThreadTaskRunner> main_task_runner,
      scoped_refptr<base::SingleThreadTaskRunner> encode_task_runner,
      webrtc::VideoCodecType video_codec_type,
      webrtc::VideoContentType video_content_type);
  WebRtcVideoEncoderWebOS(const WebRtcVideoEncoderWebOS&) = delete;
  WebRtcVideoEncoderWebOS& operator=(const WebRtcVideoEncoderWebOS&) =
      delete;
  ~WebRtcVideoEncoderWebOS();

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
      const webrtc::VideoEncoder::RateControlParameters& parameters) override;
  void Destroy(base::WaitableEvent* async_waiter) override;

 private:
  // Checks if the bitrate would overflow when passing from kbps to bps.
  bool IsBitrateTooHigh(uint32_t bitrate);

  void OnEncodedData(const uint8_t* buffer, uint32_t buffer_size,
                     uint64_t time_stamp, bool is_key_frame);

  // Return an encoded output buffer to WebRTC.
  void ReturnEncodedImage(const webrtc::EncodedImage& image);

  uint32_t UpdateFrameSize(gfx::Size new_size);

  scoped_refptr<base::SingleThreadTaskRunner> main_task_runner_;

  // The underlying perform encoding on.
  std::unique_ptr<mcil::encoder::VideoEncoderAPI> video_encoder_api_;

  // Whether load is completed or not.
  bool load_completed_ = false;

  // Frame sizes.
  gfx::Size input_visible_size_;

  // Used for extracting I420 buffers from webrtc::VideoFrame
  std::unique_ptr<uint8_t> i420_buffer_;
  size_t i420_buffer_size_ = 0;

  base::WeakPtr<WebRtcVideoEncoderWebOS> weak_this_;
  base::WeakPtrFactory<WebRtcVideoEncoderWebOS> weak_this_factory_{this};
};

}  // namespace media

#endif  // MEDIA_WEBRTC_NEVA_WEBOS_WEBRTC_VIDEO_ENCODER_WEBOS_H_
