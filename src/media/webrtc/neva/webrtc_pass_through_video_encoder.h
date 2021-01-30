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

#ifndef MEDIA_WEBRTC_NEVA_WEBRTC_PASS_THROUGH_VIDEO_ENCODER_H_
#define MEDIA_WEBRTC_NEVA_WEBRTC_PASS_THROUGH_VIDEO_ENCODER_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/threading/thread.h"
#include "media/base/media_export.h"
#include "media/video/video_encode_accelerator.h"
#include "third_party/webrtc/api/video_codecs/video_encoder.h"
#include "third_party/webrtc/modules/video_coding/include/video_codec_interface.h"

namespace media {

class WebRtcVideoEncoder;

class MEDIA_EXPORT WebRtcPassThroughVideoEncoder : public webrtc::VideoEncoder {
 public:
  // Returns array of all codec profiles supported by the platform
  static std::vector<media::VideoEncodeAccelerator::SupportedProfile>
      GetSupportedEncodeProfiles();

  WebRtcPassThroughVideoEncoder(
      scoped_refptr<base::SingleThreadTaskRunner> main_task_runner);
  virtual ~WebRtcPassThroughVideoEncoder();

  // Implements webrtc::VideoEncoder
  int InitEncode(const webrtc::VideoCodec* codec,
                 const webrtc::VideoEncoder::Settings& settings) override;
  int32_t Encode(
      const webrtc::VideoFrame& input_image,
      const std::vector<webrtc::VideoFrameType>* frame_types) override;
  int32_t RegisterEncodeCompleteCallback(
      webrtc::EncodedImageCallback* callback) override;
  int32_t Release() override;
  void SetRates(
      const webrtc::VideoEncoder::RateControlParameters& parameters) override;
  EncoderInfo GetEncoderInfo() const override;

 private:
  friend class WebRtcVideoEncoder;

  // Returns a SingleThreadTaskRunner instance corresponding to the
  // encoding thread on which encoding operations should be run.
  scoped_refptr<base::SingleThreadTaskRunner> GetEncodeTaskRunner();

  // Thread for running encoding operations (e.g., video encoding).
  std::unique_ptr<base::Thread> encoding_thread_;

  scoped_refptr<base::SingleThreadTaskRunner> main_task_runner_;
  scoped_refptr<base::SingleThreadTaskRunner> encode_task_runner_;

  // The WebRtcVideoEncoder that does all the work.
  scoped_refptr<WebRtcVideoEncoder> video_encoder_;

  DISALLOW_COPY_AND_ASSIGN(WebRtcPassThroughVideoEncoder);
};

}  // namespace media

#endif  // MEDIA_WEBRTC_NEVA_WEBRTC_PASS_THROUGH_VIDEO_ENCODER_H_
