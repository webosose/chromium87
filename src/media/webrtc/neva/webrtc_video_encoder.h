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

#ifndef MEDIA_WEBRTC_NEVA_WEBRTC_VIDEO_ENCODER_H_
#define MEDIA_WEBRTC_NEVA_WEBRTC_VIDEO_ENCODER_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/single_thread_task_runner.h"
#include "base/synchronization/waitable_event.h"
#include "base/threading/thread_checker.h"
#include "base/threading/thread_restrictions.h"
#include "base/time/time.h"
#include "third_party/webrtc/api/video/video_codec_type.h"
#include "third_party/webrtc/api/video/video_content_type.h"
#include "third_party/webrtc/api/video_codecs/video_encoder.h"
#include "third_party/webrtc/modules/video_coding/include/video_error_codes.h"
#include "ui/gfx/geometry/size.h"

namespace webrtc {
class VideoFrame;
}  // namespace webrtc

namespace media {

class WebRtcVideoEncoder
    : public base::RefCountedThreadSafe<media::WebRtcVideoEncoder> {
 public:
  struct WebRtcTimestamps {
    WebRtcTimestamps(const base::TimeDelta& media_timestamp,
                  int32_t rtp_timestamp,
                  int64_t capture_time_ms)
        : media_timestamp_(media_timestamp),
          rtp_timestamp(rtp_timestamp),
          capture_time_ms(capture_time_ms) {}
    const base::TimeDelta media_timestamp_;
    const int32_t rtp_timestamp;
    const int64_t capture_time_ms;
  };

  // Returns true if the codec type passed is supported by the platform
  static bool IsCodecTypeSupported(webrtc::VideoCodecType type);

  // Creates an instance of WebRtcVideoEncoder.
  static WebRtcVideoEncoder* Create(
      scoped_refptr<base::SingleThreadTaskRunner> main_task_runner,
      scoped_refptr<base::SingleThreadTaskRunner> encode_task_runner,
      webrtc::VideoCodecType video_codec_type,
      webrtc::VideoContentType video_content_type);

  static const char* ImplementationName();

  WebRtcVideoEncoder(
      scoped_refptr<base::SingleThreadTaskRunner> encode_task_runner,
      webrtc::VideoCodecType video_codec_type,
      webrtc::VideoContentType video_content_type);

  virtual ~WebRtcVideoEncoder();

  virtual void CreateAndInitialize(const gfx::Size& input_visible_size,
                                   uint32_t bitrate,
                                   uint32_t framerate,
                                   base::WaitableEvent* async_waiter,
                                   int32_t* async_retval) = 0;

  virtual void EncodeFrame(const webrtc::VideoFrame* frame,
                           bool force_keyframe,
                           base::WaitableEvent* async_waiter,
                           int32_t* async_retval) = 0;

  virtual void RequestEncodingParametersChange(
      const webrtc::VideoEncoder::RateControlParameters& parameters) = 0;

  virtual void Destroy(base::WaitableEvent* async_waiter) = 0;

  void RegisterEncodeCompleteCallback(base::WaitableEvent* async_waiter,
                                      int32_t* async_retval,
                                      webrtc::EncodedImageCallback* callback);

  int32_t GetStatus() const;
  void SetStatus(int32_t status);

  void RegisterAsyncWaiter(base::WaitableEvent* waiter, int32_t* retval);
  void SignalAsyncWaiter(int32_t retval);

 private:
  friend class base::RefCountedThreadSafe<WebRtcVideoEncoder>;
  friend class WebRtcVideoEncoderWebOS;

  // Task runner corresponding to encoding thread
  scoped_refptr<base::SingleThreadTaskRunner> encode_task_runner_;

  // The video codec type, as reported to WebRTC.
  const webrtc::VideoCodecType video_codec_type_ = webrtc::kVideoCodecGeneric;

  // The content type, as reported to WebRTC (screenshare vs realtime video).
  const webrtc::VideoContentType video_content_type_ =
      webrtc::VideoContentType::UNSPECIFIED;

  // Used to match the encoded frame timestamp with WebRTC's given RTP
  // timestamp.
  std::deque<WebRtcTimestamps> pending_timestamps_;

  // Indicates that timestamp match failed and we should no longer attempt
  // matching.
  bool failed_timestamp_match_ = false;

  base::WaitableEvent* async_waiter_ = nullptr;
  int32_t* async_retval_ = nullptr;

  // webrtc::VideoEncoder encode complete callback.
  webrtc::EncodedImageCallback* encoded_image_callback_ = nullptr;

  mutable base::Lock status_lock_;

  int32_t status_ = WEBRTC_VIDEO_CODEC_UNINITIALIZED;
};

}  // namespace media

#endif  // MEDIA_WEBRTC_NEVA_WEBRTC_VIDEO_ENCODER_H_
