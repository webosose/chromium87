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

#ifndef MEDIA_WEBRTC_NEVA_WEBRTC_PASS_THROUGH_VIDEO_DECODER_H_
#define MEDIA_WEBRTC_NEVA_WEBRTC_PASS_THROUGH_VIDEO_DECODER_H_

#include <atomic>
#include <memory>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/thread.h"
#include "base/time/time.h"
#include "media/base/media_export.h"
#include "media/base/video_codecs.h"
#include "media/base/video_types.h"
#include "third_party/webrtc/api/video/video_codec_type.h"
#include "third_party/webrtc/api/video_codecs/video_decoder.h"
#include "ui/gfx/geometry/size.h"

namespace webrtc {
class EncodedImage;
class SdpVideoFormat;
class VideoCodec;
}  // namespace webrtc

namespace media {

class VideoFrame;

class MEDIA_EXPORT WebRtcPassThroughVideoDecoder : public webrtc::VideoDecoder {
 public:
  // Creates and initializes an WebRtcPassThroughVideoDecoder.
  static std::unique_ptr<WebRtcPassThroughVideoDecoder> Create(
      scoped_refptr<base::SingleThreadTaskRunner> main_task_runner,
      const webrtc::SdpVideoFormat& format);

  virtual ~WebRtcPassThroughVideoDecoder();

  // Implements webrtc::VideoDecoder
  int32_t InitDecode(const webrtc::VideoCodec* codec_settings,
                     int32_t number_of_cores) override;
  int32_t Decode(const webrtc::EncodedImage& input_image,
                 bool missing_frames,
                 int64_t render_time_ms) override;
  int32_t RegisterDecodeCompleteCallback(
      webrtc::DecodedImageCallback* callback) override;
  int32_t Release() override;
  const char* ImplementationName() const override;

 private:
  // Called on the worker thread.
  WebRtcPassThroughVideoDecoder(
      scoped_refptr<base::SingleThreadTaskRunner> main_task_runner,
      media::VideoCodec video_codec);

  void DecodeOnMediaThread();
  void ReturnEncodedFrame(scoped_refptr<media::VideoFrame> encoded_frame);

  void RequestSwFallback();

  // Construction parameters.
  media::VideoCodec video_codec_;
  VideoPixelFormat video_pixel_format_;

  gfx::Size frame_size_;

  scoped_refptr<base::SingleThreadTaskRunner> main_task_runner_;
  webrtc::VideoCodecType video_codec_type_ = webrtc::kVideoCodecGeneric;
  webrtc::DecodedImageCallback* decode_complete_callback_ = nullptr;

  base::Lock lock_;
  int32_t consecutive_error_count_ = 0;

  // Requests that have not been submitted to the decoder yet.
  std::vector<scoped_refptr<media::VideoFrame>> pending_frames_;

  // Record of timestamps that have been sent to be decoded. Removing a
  // timestamp will cause the frame to be dropped when it is output.
  std::deque<base::TimeDelta> decode_timestamps_;

  bool key_frame_required_ = true;

  base::RepeatingCallback<void()> software_fallback_callback_;

  std::atomic<bool> sw_fallback_needed_{false};

  base::WeakPtr<WebRtcPassThroughVideoDecoder> weak_this_;
  base::WeakPtrFactory<WebRtcPassThroughVideoDecoder> weak_this_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(WebRtcPassThroughVideoDecoder);
};

}  // namespace media

#endif  // MEDIA_WEBRTC_NEVA_WEBRTC_PASS_THROUGH_VIDEO_DECODER_H_
