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

#include "media/webrtc/neva/webos/webrtc_video_encoder_webos.h"

#pragma GCC optimize("rtti")
#include <mcil/video_encoder_api.h>
#pragma GCC reset_options

#include <memory>
#include <vector>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/threading/thread_task_runner_handle.h"
#include "media/base/bind_to_current_loop.h"
#include "media/base/video_bitrate_allocation.h"
#include "media/base/video_frame.h"
#include "media/video/h264_parser.h"
#include "third_party/blink/renderer/platform/scheduler/public/post_cross_thread_task.h"
#include "third_party/blink/renderer/platform/webrtc/webrtc_video_frame_adapter.h"
#include "third_party/blink/renderer/platform/wtf/cross_thread_functional.h"
#include "third_party/webrtc/api/video/i420_buffer.h"
#include "third_party/webrtc/common_video/libyuv/include/webrtc_libyuv.h"
#include "third_party/webrtc/modules/video_coding/include/video_codec_interface.h"
#include "third_party/webrtc/rtc_base/time_utils.h"

namespace media {

namespace {

MCIL_VIDEO_CODEC emc_codecs[] = {
  MCIL_VIDEO_CODEC_NONE,
  MCIL_VIDEO_CODEC_VP8,
  MCIL_VIDEO_CODEC_VP9,
  MCIL_VIDEO_CODEC_NONE,
  MCIL_VIDEO_CODEC_H264,
  MCIL_VIDEO_CODEC_MAX,
};

// Size are aligned for efficiency (required by HXENC).
constexpr int kSizeAlignment = 4;

static inline size_t RoundUp(size_t value, size_t alignment) {
  return ((value + (alignment - 1)) & ~(alignment - 1));
}

const char* EncoderCbTypeToString(gint type) {
#define STRINGIFY_ENCODER_CB_TYPE_CASE(type) \
  case type:                             \
    return #type

  switch (static_cast<ENCODER_CB_TYPE_T>(type)) {
    STRINGIFY_ENCODER_CB_TYPE_CASE(ENCODER_CB_LOAD_COMPLETE);
    STRINGIFY_ENCODER_CB_TYPE_CASE(ENCODER_CB_NOTIFY_PLAYING);
    STRINGIFY_ENCODER_CB_TYPE_CASE(ENCODER_CB_NOTIFY_PAUSED);
    STRINGIFY_ENCODER_CB_TYPE_CASE(ENCODER_CB_BUFFER_ENCODED);
    STRINGIFY_ENCODER_CB_TYPE_CASE(ENCODER_CB_NOTIFY_EOS);
    STRINGIFY_ENCODER_CB_TYPE_CASE(ENCODER_CB_NOTIFY_ERROR);
    STRINGIFY_ENCODER_CB_TYPE_CASE(ENCODER_CB_UNLOAD_COMPLETE);
    default:
      return "Unknown CB type";
  }
}

}

// static
WebRtcVideoEncoder* WebRtcVideoEncoder::Create(
    scoped_refptr<base::SingleThreadTaskRunner> main_task_runner,
    scoped_refptr<base::SingleThreadTaskRunner> encode_task_runner,
    webrtc::VideoCodecType video_codec_type,
    webrtc::VideoContentType video_content_type) {
  return new WebRtcVideoEncoderWebOS(
      main_task_runner, encode_task_runner, video_codec_type,
      video_content_type);
}

// static
bool WebRtcVideoEncoder::IsCodecTypeSupported(
    webrtc::VideoCodecType type) {
  return mcil::encoder::VideoEncoderAPI::IsCodecSupported(emc_codecs[type]);
}

// static
const char* WebRtcVideoEncoder::ImplementationName() {
  return "WebRtcVideoEncoderWebOS";
}

WebRtcVideoEncoderWebOS::WebRtcVideoEncoderWebOS(
    scoped_refptr<base::SingleThreadTaskRunner> main_task_runner,
    scoped_refptr<base::SingleThreadTaskRunner> encode_task_runner,
    webrtc::VideoCodecType video_codec_type,
    webrtc::VideoContentType video_content_type)
    : WebRtcVideoEncoder(encode_task_runner, video_codec_type,
                         video_content_type),
      main_task_runner_(main_task_runner) {
  LOG(INFO) << __func__ << " codecType=" << video_codec_type;
  weak_this_ = weak_this_factory_.GetWeakPtr();
}

WebRtcVideoEncoderWebOS::~WebRtcVideoEncoderWebOS() {
  DCHECK(!video_encoder_api_);
}

void WebRtcVideoEncoderWebOS::CreateAndInitialize(
    const gfx::Size& input_visible_size,
    uint32_t bitrate,
    uint32_t framerate,
    base::WaitableEvent* async_waiter,
    int32_t* async_retval) {
  LOG(INFO) << __func__
            << ", input_size=" << input_visible_size.ToString()
            << ", framerate=" << framerate << ", bitrate=" << bitrate;

  DCHECK(encode_task_runner_->BelongsToCurrentThread());

  SetStatus(WEBRTC_VIDEO_CODEC_UNINITIALIZED);
  RegisterAsyncWaiter(async_waiter, async_retval);

  // Check for overflow converting bitrate (kilobits/sec) to bits/sec.
  if (IsBitrateTooHigh(bitrate)) {
    LOG(ERROR) << __func__ << " Bitrate is too high";
    SignalAsyncWaiter(WEBRTC_VIDEO_CODEC_UNINITIALIZED);
    return;
  }

  i420_buffer_size_ = webrtc::CalcBufferSize(webrtc::VideoType::kI420,
                                             input_visible_size.width(),
                                             input_visible_size.height());
  i420_buffer_.reset(new uint8_t[i420_buffer_size_]);
  if (!i420_buffer_) {
    LOG(ERROR) << __func__ << " Failed to allocate buffer";
    SignalAsyncWaiter(WEBRTC_VIDEO_CODEC_UNINITIALIZED);
    return;
  }

  video_encoder_api_.reset(new mcil::encoder::VideoEncoderAPI());
  if (!video_encoder_api_) {
    LOG(ERROR) << __func__ << " Failed to create encoder instance";
    SignalAsyncWaiter(WEBRTC_VIDEO_CODEC_ERROR);
    return;
  }

  // Size are aligned for efficiency (required by HXENC).
  int codec_width = RoundUp(input_visible_size.width(), kSizeAlignment);
  int codec_height = RoundUp(input_visible_size.height(), kSizeAlignment);

  input_visible_size_ = gfx::Size(codec_width, codec_height);
  size_t encoded_buf_size = webrtc::CalcBufferSize(webrtc::VideoType::kI420,
                                                   codec_width, codec_height);

  ENCODER_INIT_DATA_T init_data;
  init_data.pixelFormat = MCIL_PIXEL_I420;
  init_data.codecType = emc_codecs[video_codec_type_];
  init_data.width = codec_width;
  init_data.height = codec_height;
  init_data.bitRate = bitrate;
  init_data.frameRate = framerate;
  init_data.bufferSize = encoded_buf_size;

  if (!video_encoder_api_->Init(&init_data,
          std::bind(&WebRtcVideoEncoderWebOS::OnEncodedData,
                    weak_this_, std::placeholders::_1, std::placeholders::_2,
                    std::placeholders::_3, std::placeholders::_4))) {
    LOG(ERROR) << __func__ << " Error initializing encoder.";
    video_encoder_api_.reset();
    SignalAsyncWaiter(WEBRTC_VIDEO_CODEC_ERROR);
    return;
  }

  load_completed_ = true;

  SetStatus(WEBRTC_VIDEO_CODEC_OK);
  SignalAsyncWaiter(WEBRTC_VIDEO_CODEC_OK);
}

void WebRtcVideoEncoderWebOS::EncodeFrame(
    const webrtc::VideoFrame* new_frame,
    bool force_keyframe,
    base::WaitableEvent* async_waiter,
    int32_t* async_retval) {
  DVLOG(3) << __func__;
  DCHECK(encode_task_runner_->BelongsToCurrentThread());

  RegisterAsyncWaiter(async_waiter, async_retval);
  int32_t retval = GetStatus();
  if (retval != WEBRTC_VIDEO_CODEC_OK) {
    SignalAsyncWaiter(retval);
    return;
  }

  if (!video_encoder_api_) {
    LOG(ERROR) << __func__ << " Error encoder client not created.";
    SignalAsyncWaiter(WEBRTC_VIDEO_CODEC_UNINITIALIZED);
    return;
  }

  const webrtc::VideoFrame* next_frame = new_frame;
  if (!new_frame->video_frame_buffer()) {
    LOG(ERROR) << __func__ << " Error extracting frame buffer!";
    SignalAsyncWaiter(WEBRTC_VIDEO_CODEC_ERROR);
    return;
  }

  rtc::scoped_refptr<webrtc::VideoFrameBuffer> input_image =
      new_frame->video_frame_buffer();

  int32_t aligned_width = RoundUp(input_image->width(), kSizeAlignment);
  int32_t aligned_height = RoundUp(input_image->height(), kSizeAlignment);

  if (aligned_width != input_image->width() ||
      aligned_height != input_image->height()) {
    rtc::scoped_refptr<webrtc::I420Buffer> scaled_buffer =
        webrtc::I420Buffer::Create(aligned_width, aligned_height);
    rtc::scoped_refptr<const webrtc::I420BufferInterface> input_image_i420 =
        input_image->GetI420();
    scaled_buffer->ScaleFrom(*input_image_i420.get());
    // Use scaled buffer as input image.
    input_image = scaled_buffer;
  }

  gfx::Size new_visible_size(input_image->width(), input_image->height());
  if (input_visible_size_ != new_visible_size) {
    int32_t ret = UpdateFrameSize(new_visible_size);
    if (ret < 0) {
      SignalAsyncWaiter(WEBRTC_VIDEO_CODEC_FALLBACK_SOFTWARE);
      return;
    }
  }

  if (i420_buffer_size_ != webrtc::ExtractBuffer(input_image->ToI420(),
                                                 i420_buffer_size_,
                                                 i420_buffer_.get())) {
    LOG(ERROR) << __func__ << " Error extracting i420_buffer.";
    SignalAsyncWaiter(WEBRTC_VIDEO_CODEC_ERROR);
    return;
  }

#if defined(USE_GST_MEDIA)
  if (MCIL_MEDIA_OK != video_encoder_api_->Encode(i420_buffer_.get(),
                                                 i420_buffer_size_)) {
    LOG(ERROR) << __func__ << " Error feeding i420_buffer.";
    // Fallback to software if h/w encoder is not available.
    SignalAsyncWaiter(video_encoder_api_->IsEncoderAvailable() ?
        WEBRTC_VIDEO_CODEC_ERROR : WEBRTC_VIDEO_CODEC_FALLBACK_SOFTWARE);
    return;
  }
#else
  rtc::scoped_refptr<webrtc::PlanarYuv8Buffer> input_frame =
      input_image->ToI420();

  int y_size = input_frame->width() * input_frame->height();
  int uv_size = input_frame->ChromaWidth() * input_frame->ChromaHeight();

  if (MCIL_MEDIA_OK != video_encoder_api_->Encode(i420_buffer_.get(), y_size,
          i420_buffer_.get() + y_size, uv_size,
          i420_buffer_.get() + y_size + uv_size, uv_size,
          new_frame->timestamp(), force_keyframe)) {
    LOG(ERROR) << __func__ << " Error feeding buffer.";
    // Fallback to software if h/w encoder is not available.
    SignalAsyncWaiter(video_encoder_api_->IsEncoderAvailable() ?
        WEBRTC_VIDEO_CODEC_ERROR : WEBRTC_VIDEO_CODEC_FALLBACK_SOFTWARE);
    return;
  }
#endif

  if (!failed_timestamp_match_) {
    const base::TimeDelta timestamp =
        base::TimeDelta::FromMilliseconds(new_frame->ntp_time_ms());
    pending_timestamps_.emplace_back(timestamp, new_frame->timestamp(),
                                     new_frame->render_time_ms());
  }

  SignalAsyncWaiter(WEBRTC_VIDEO_CODEC_OK);
}

void WebRtcVideoEncoderWebOS::RequestEncodingParametersChange(
    const webrtc::VideoEncoder::RateControlParameters& parameters) {
  DVLOG(3) << __func__ << " bitrate=" << parameters.bitrate.ToString()
           << ", framerate=" << parameters.framerate_fps;
  DCHECK(encode_task_runner_->BelongsToCurrentThread());

  // This is a workaround to zero being temporarily provided, as part of the
  // initial setup, by WebRTC.
  if (video_encoder_api_) {
    media::VideoBitrateAllocation allocation;
    if (parameters.bitrate.get_sum_bps() == 0) {
      allocation.SetBitrate(0, 0, 1);
    }
    uint32_t framerate =
        std::max(1u, static_cast<uint32_t>(parameters.framerate_fps + 0.5));
    for (size_t spatial_id = 0;
         spatial_id < media::VideoBitrateAllocation::kMaxSpatialLayers;
         ++spatial_id) {
      for (size_t temporal_id = 0;
           temporal_id < media::VideoBitrateAllocation::kMaxTemporalLayers;
           ++temporal_id) {
        // TODO(sprang): Clean this up if/when webrtc struct moves to int.
        uint32_t layer_bitrate =
            parameters.bitrate.GetBitrate(spatial_id, temporal_id);
        CHECK_LE(layer_bitrate,
                 static_cast<uint32_t>(std::numeric_limits<int>::max()));
        if (!allocation.SetBitrate(spatial_id, temporal_id, layer_bitrate)) {
          LOG(WARNING) << "Overflow in bitrate allocation: "
                       << parameters.bitrate.ToString();
          break;
        }
      }
    }

    ENCODING_PARAMS_T params;
    params.bitRate = allocation.GetSumBps();
    params.frameRate = framerate;
    video_encoder_api_->UpdateEncodingParams(&params);
  }
}

void WebRtcVideoEncoderWebOS::Destroy(base::WaitableEvent* async_waiter) {
  DVLOG(3) << __func__;
  DCHECK(encode_task_runner_->BelongsToCurrentThread());

  if (i420_buffer_ != nullptr) {
    i420_buffer_.reset();
  }

  if (video_encoder_api_) {
    video_encoder_api_.reset();
    SetStatus(WEBRTC_VIDEO_CODEC_UNINITIALIZED);
  }

  async_waiter->Signal();
}

bool WebRtcVideoEncoderWebOS::IsBitrateTooHigh(uint32_t bitrate) {
  if (base::IsValueInRangeForNumericType<uint32_t>(bitrate * UINT64_C(1000)))
    return false;
  return true;
}

void WebRtcVideoEncoderWebOS::OnEncodedData(const uint8_t* buffer,
                                               uint32_t buffer_size,
                                               uint64_t time_stamp,
                                               bool is_key_frame) {
  if (!buffer || buffer_size <= 0) {
    LOG(ERROR) << __func__ << " Invalid data info";
    return;
  }

  DVLOG(3) << __func__ << ", buffer_size= " << buffer_size
                                   << ", is_key_frame= " << is_key_frame
                                   << ", time_stamp= " << time_stamp;

  const base::TimeDelta media_timestamp =
      base::TimeDelta::FromMilliseconds(time_stamp);
  base::Optional<uint32_t> rtp_timestamp;
  base::Optional<int64_t> capture_timestamp_ms;
  if (!failed_timestamp_match_) {
    // Pop timestamps until we have a match.
    while (!pending_timestamps_.empty()) {
      const auto& front_timestamps = pending_timestamps_.front();
      if (front_timestamps.media_timestamp_ == media_timestamp) {
        rtp_timestamp = front_timestamps.rtp_timestamp;
        capture_timestamp_ms = front_timestamps.capture_time_ms;
        pending_timestamps_.pop_front();
        break;
      }
      pending_timestamps_.pop_front();
    }
  }

  if (!rtp_timestamp.has_value() || !capture_timestamp_ms.has_value()) {
    failed_timestamp_match_ = true;
    pending_timestamps_.clear();
    const int64_t current_time_ms =
        rtc::TimeMicros() / base::Time::kMicrosecondsPerMillisecond;
    // RTP timestamp can wrap around. Get the lower 32 bits.
    rtp_timestamp = static_cast<uint32_t>(current_time_ms * 90);
    capture_timestamp_ms = current_time_ms;
  }

  webrtc::EncodedImage encoded_image;
  encoded_image.SetEncodedData(
      webrtc::EncodedImageBuffer::Create(buffer, buffer_size));
  encoded_image.SetTimestamp(rtp_timestamp.value());
  encoded_image._encodedWidth = input_visible_size_.width();
  encoded_image._encodedHeight = input_visible_size_.height();
  encoded_image.capture_time_ms_ = capture_timestamp_ms.value();
  encoded_image._frameType =
      (is_key_frame ? webrtc::VideoFrameType::kVideoFrameKey
                    : webrtc::VideoFrameType::kVideoFrameDelta);
  encoded_image.content_type_ = video_content_type_;
  encoded_image._completeFrame = true;

  main_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&WebRtcVideoEncoderWebOS::ReturnEncodedImage,
                     weak_this_, encoded_image));
}

void WebRtcVideoEncoderWebOS::ReturnEncodedImage(
    const webrtc::EncodedImage& image) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(3) << __func__;

  if (!encoded_image_callback_) {
    LOG(ERROR) << __func__ << " webrtc::EncodedImageCallback is not set";
    return;
  }

  webrtc::CodecSpecificInfo info;
  info.codecType = video_codec_type_;
  if (video_codec_type_ == webrtc::kVideoCodecVP8) {
    info.codecSpecific.VP8.keyIdx = -1;
  } else if (video_codec_type_ == webrtc::kVideoCodecVP9) {
    bool key_frame = image._frameType == webrtc::VideoFrameType::kVideoFrameKey;
    info.codecSpecific.VP9.inter_pic_predicted = !key_frame;
    info.codecSpecific.VP9.flexible_mode = false;
    info.codecSpecific.VP9.ss_data_available = key_frame;
    info.codecSpecific.VP9.temporal_idx = webrtc::kNoTemporalIdx;
    info.codecSpecific.VP9.temporal_up_switch = true;
    info.codecSpecific.VP9.inter_layer_predicted = false;
    info.codecSpecific.VP9.gof_idx = 0;
    info.codecSpecific.VP9.num_spatial_layers = 1;
    info.codecSpecific.VP9.first_frame_in_picture = true;
    info.codecSpecific.VP9.end_of_picture = true;
    info.codecSpecific.VP9.spatial_layer_resolution_present = false;
    if (info.codecSpecific.VP9.ss_data_available) {
      info.codecSpecific.VP9.spatial_layer_resolution_present = true;
      info.codecSpecific.VP9.width[0] = image._encodedWidth;
      info.codecSpecific.VP9.height[0] = image._encodedHeight;
      info.codecSpecific.VP9.gof.num_frames_in_gof = 1;
      info.codecSpecific.VP9.gof.temporal_idx[0] = 0;
      info.codecSpecific.VP9.gof.temporal_up_switch[0] = false;
      info.codecSpecific.VP9.gof.num_ref_pics[0] = 1;
      info.codecSpecific.VP9.gof.pid_diff[0][0] = 1;
    }
  }

  const auto result =
      encoded_image_callback_->OnEncodedImage(image, &info);
  if (result.error != webrtc::EncodedImageCallback::Result::OK) {
    LOG(ERROR) << __func__ << " : webrtc::EncodedImageCallback::Result.error = "
               << result.error;
  }
}

uint32_t WebRtcVideoEncoderWebOS::UpdateFrameSize(gfx::Size new_size) {
  DVLOG(3) << __func__ << " new_size: " << new_size.ToString();

  input_visible_size_ = new_size;
  if (!video_encoder_api_->UpdateEncodingResolution(new_size.width(),
                                                    new_size.height())) {
    return WEBRTC_VIDEO_CODEC_FALLBACK_SOFTWARE;
  }
  return WEBRTC_VIDEO_CODEC_OK;
}
}  // namespace media
