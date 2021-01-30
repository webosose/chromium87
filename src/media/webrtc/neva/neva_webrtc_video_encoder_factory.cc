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

#include "media/webrtc/neva/neva_webrtc_video_encoder_factory.h"

#include "base/logging.h"
#include "media/video/gpu_video_accelerator_factories.h"
#include "media/webrtc/neva/webrtc_pass_through_video_encoder.h"
#include "third_party/webrtc/api/video_codecs/sdp_video_format.h"
#include "third_party/webrtc/media/base/codec.h"
#include "third_party/webrtc/media/base/h264_profile_level_id.h"
#include "third_party/webrtc/media/base/media_constants.h"

namespace media {

namespace {

// Translate from media::VideoEncodeAccelerator::SupportedProfile to
// webrtc::SdpVideoFormat, or return nothing if the profile isn't supported.
base::Optional<webrtc::SdpVideoFormat> VEAProfileToFormat(
    const media::VideoEncodeAccelerator::SupportedProfile& profile) {
  DCHECK_EQ(profile.max_framerate_denominator, 1U);

  if (profile.profile >= media::VP8PROFILE_MIN &&
      profile.profile <= media::VP8PROFILE_MAX) {
    return webrtc::SdpVideoFormat("VP8");
  } else if (profile.profile >= media::H264PROFILE_MIN &&
             profile.profile <= media::H264PROFILE_MAX) {
    const int width = profile.max_resolution.width();
    const int height = profile.max_resolution.height();
    const int fps = profile.max_framerate_numerator;
    DCHECK_EQ(1u, profile.max_framerate_denominator);
    const absl::optional<webrtc::H264::Level> h264_level =
        webrtc::H264::SupportedLevel(width * height, fps);
    const webrtc::H264::ProfileLevelId profile_level_id(
        webrtc::H264::kProfileBaseline,
        h264_level.value_or(webrtc::H264::kLevel1));

    webrtc::SdpVideoFormat format("H264");
    format.parameters = {
        {cricket::kH264FmtpProfileLevelId,
         *webrtc::H264::ProfileLevelIdToString(profile_level_id)},
        {cricket::kH264FmtpLevelAsymmetryAllowed, "1"},
        {cricket::kH264FmtpPacketizationMode, "1"}};
    return format;
  } else if (profile.profile >= media::VP9PROFILE_MIN &&
             profile.profile <= media::VP9PROFILE_MAX) {
    return webrtc::SdpVideoFormat("VP9");
  }
  return base::nullopt;
}

bool IsSameFormat(const webrtc::SdpVideoFormat& format1,
                  const webrtc::SdpVideoFormat& format2) {
  return cricket::IsSameCodec(format1.name, format2.parameters, format2.name,
                              format2.parameters);
}

}  // anonymous namespace

NevaWebRtcVideoEncoderFactory::NevaWebRtcVideoEncoderFactory(
    scoped_refptr<base::SingleThreadTaskRunner> main_task_runner)
    : main_task_runner_(main_task_runner) {
  const media::VideoEncodeAccelerator::SupportedProfiles& profiles =
      WebRtcPassThroughVideoEncoder::GetSupportedEncodeProfiles();
  for (const auto& profile : profiles) {
    base::Optional<webrtc::SdpVideoFormat> format = VEAProfileToFormat(profile);
    if (format) {
      supported_formats_.emplace_back(std::move(*format));
      profiles_.emplace_back(profile.profile);
    }
  }
}

NevaWebRtcVideoEncoderFactory::~NevaWebRtcVideoEncoderFactory() = default;

std::vector<webrtc::SdpVideoFormat>
NevaWebRtcVideoEncoderFactory::GetSupportedFormats() const {
  return supported_formats_;
}

webrtc::VideoEncoderFactory::CodecInfo
NevaWebRtcVideoEncoderFactory::QueryVideoEncoder(
    const webrtc::SdpVideoFormat& format) const {
  CodecInfo info;
  info.has_internal_source = false;
  return info;
}

std::unique_ptr<webrtc::VideoEncoder>
NevaWebRtcVideoEncoderFactory::CreateVideoEncoder(
    const webrtc::SdpVideoFormat& format) {
  for (size_t i = 0; i < supported_formats_.size(); ++i) {
    if (IsSameFormat(format, supported_formats_[i])) {
      LOG(INFO) << __func__ << " create encoder for : " << format.name;
      return std::make_unique<WebRtcPassThroughVideoEncoder>(main_task_runner_);
    }
  }
  return nullptr;
}

}  // namespace media
