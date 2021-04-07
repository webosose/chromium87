// Copyright 2019 LG Electronics, Inc.
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

#include "media/filters/neva/video_decoder_mock.h"

#include "base/bind.h"
#include "base/single_thread_task_runner.h"
#include "media/base/bind_to_current_loop.h"
#include "media/base/decoder_buffer.h"
#include "media/base/video_decoder_config.h"

namespace media {

VideoDecoderMock::VideoDecoderMock(
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
    const scoped_refptr<MediaPlatformAPI>& media_platform_api)
    : HoleFrameVideoDecoder(task_runner),
      media_platform_api_(media_platform_api) {}

VideoDecoderMock::~VideoDecoderMock() {}

std::string VideoDecoderMock::GetDisplayName() const {
  return "VideoDecoderMock";
}

void VideoDecoderMock::Initialize(
    const VideoDecoderConfig& config,
    bool low_delay,
    CdmContext* cdm_context,
    InitCB init_cb,
    const OutputCB& output_cb,
    const WaitingCB& waiting_for_decryption_key_cb) {
  DCHECK(task_runner_->BelongsToCurrentThread());

  config_ = config;
  output_cb_ = BindToCurrentLoop(output_cb);
  state_ = kNormal;
  InitCB initialize_cb = BindToCurrentLoop(std::move(init_cb));
  std::move(initialize_cb).Run(OkStatus());
}

bool VideoDecoderMock::FeedForPlatformMediaVideoDecoder(
    const scoped_refptr<DecoderBuffer>& buffer) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  DCHECK(buffer);
  DCHECK(media_platform_api_);

  return media_platform_api_->Feed(buffer, FeedType::kVideo);
}

}  // namespace media
