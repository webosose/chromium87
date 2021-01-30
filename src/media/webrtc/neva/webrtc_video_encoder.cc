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

#include "media/webrtc/neva/webrtc_video_encoder.h"

#include "base/logging.h"

namespace media {

WebRtcVideoEncoder::WebRtcVideoEncoder(
    scoped_refptr<base::SingleThreadTaskRunner> encode_task_runner,
    webrtc::VideoCodecType video_codec_type,
    webrtc::VideoContentType video_content_type)
    : encode_task_runner_(encode_task_runner),
      video_codec_type_(video_codec_type),
      video_content_type_(video_content_type) {
}

WebRtcVideoEncoder::~WebRtcVideoEncoder() = default;

void WebRtcVideoEncoder::RegisterEncodeCompleteCallback(
    base::WaitableEvent* async_waiter,
    int32_t* async_retval,
    webrtc::EncodedImageCallback* callback) {
  DCHECK(encode_task_runner_->BelongsToCurrentThread());
  LOG(INFO) << __PRETTY_FUNCTION__;

  DVLOG(3) << __func__;
  RegisterAsyncWaiter(async_waiter, async_retval);
  int32_t retval = GetStatus();
  if (retval == WEBRTC_VIDEO_CODEC_OK)
    encoded_image_callback_ = callback;
  SignalAsyncWaiter(retval);
}

int32_t WebRtcVideoEncoder::GetStatus() const {
  base::AutoLock lock(status_lock_);
  return status_;
}

void WebRtcVideoEncoder::SetStatus(int32_t status) {
  base::AutoLock lock(status_lock_);
  status_ = status;
}

void WebRtcVideoEncoder::RegisterAsyncWaiter(base::WaitableEvent* waiter,
                                             int32_t* retval) {
  DCHECK(encode_task_runner_->BelongsToCurrentThread());
  DCHECK(!async_waiter_);
  DCHECK(!async_retval_);
  async_waiter_ = waiter;
  async_retval_ = retval;
}

void WebRtcVideoEncoder::SignalAsyncWaiter(int32_t retval) {
  DCHECK(encode_task_runner_->BelongsToCurrentThread());
  *async_retval_ = retval;
  async_waiter_->Signal();
  async_retval_ = nullptr;
  async_waiter_ = nullptr;
}

}  // namespace media
