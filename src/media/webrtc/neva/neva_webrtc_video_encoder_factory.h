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

#ifndef MEDIA_WEBRTC_NEVA_NEVA_WEBRTC_VIDEO_ENCODER_FACTORY_H_
#define MEDIA_WEBRTC_NEVA_NEVA_WEBRTC_VIDEO_ENCODER_FACTORY_H_

#include "base/memory/ref_counted.h"
#include "media/base/media_export.h"
#include "media/base/video_codecs.h"
#include "third_party/webrtc/api/video_codecs/video_encoder_factory.h"

namespace base {
class SingleThreadTaskRunner;
class Thread;
}

namespace webrtc {
class SdpVideoFormat;
class VideoEncoder;
}  // namespace webrtc

namespace media {

class MEDIA_EXPORT NevaWebRtcVideoEncoderFactory
    : public webrtc::VideoEncoderFactory {
 public:
  explicit NevaWebRtcVideoEncoderFactory(
      scoped_refptr<base::SingleThreadTaskRunner> main_task_runner);
  ~NevaWebRtcVideoEncoderFactory() override;

  std::vector<webrtc::SdpVideoFormat> GetSupportedFormats() const override;
  webrtc::VideoEncoderFactory::CodecInfo QueryVideoEncoder(
      const webrtc::SdpVideoFormat& format) const override;
  std::unique_ptr<webrtc::VideoEncoder> CreateVideoEncoder(
      const webrtc::SdpVideoFormat& format) override;

 private:
  scoped_refptr<base::SingleThreadTaskRunner> main_task_runner_;

  // List of supported webrtc::SdpVideoFormat. |profiles_| and
  // |supported_formats_| have the same length and the profile for
  // |supported_formats_[i]| is |profiles_[i]|.
  std::vector<media::VideoCodecProfile> profiles_;
  std::vector<webrtc::SdpVideoFormat> supported_formats_;
};

}  // namespace media

#endif  // MEDIA_WEBRTC_NEVA_NEVA_WEBRTC_VIDEO_ENCODER_FACTORY_H_
