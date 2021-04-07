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

#include "media/webrtc/neva/neva_webrtc_video_decoder_factory.h"

#include "media/webrtc/neva/webrtc_pass_through_video_decoder.h"
#include "third_party/webrtc/api/video_codecs/sdp_video_format.h"

namespace media {

NevaWebRtcVideoDecoderFactory::NevaWebRtcVideoDecoderFactory(
    scoped_refptr<base::SingleThreadTaskRunner> main_task_runner)
    : main_task_runner_(main_task_runner) {}

std::vector<webrtc::SdpVideoFormat>
NevaWebRtcVideoDecoderFactory::GetSupportedFormats() const {
  return std::vector<webrtc::SdpVideoFormat>();
}

std::unique_ptr<webrtc::VideoDecoder>
NevaWebRtcVideoDecoderFactory::CreateVideoDecoder(
    const webrtc::SdpVideoFormat& format) {
  return std::move(
      WebRtcPassThroughVideoDecoder::Create(main_task_runner_, format));
}

}  // namespace media
