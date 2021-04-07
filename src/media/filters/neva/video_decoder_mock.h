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

#ifndef MEDIA_FILTERS_NEVA_VIDEO_DECODER_MOCK_H_
#define MEDIA_FILTERS_NEVA_VIDEO_DECODER_MOCK_H_

#include "media/base/video_frame.h"
#include "media/filters/neva/holeframe_video_decoder.h"
#include "media/neva/media_platform_api.h"

namespace media {

class MEDIA_EXPORT VideoDecoderMock : public HoleFrameVideoDecoder {
 public:
  VideoDecoderMock(
      const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
      const scoped_refptr<MediaPlatformAPI>& media_platform_api);
  ~VideoDecoderMock() override;

  // VideoDecoder implementation.
  std::string GetDisplayName() const override;
  void Initialize(const VideoDecoderConfig& config,
                  bool low_delay,
                  CdmContext* cdm_context,
                  InitCB init_cb,
                  const OutputCB& output_cb,
                  const WaitingCB& waiting_for_decryption_key_cb) override;

  // HoleFrameVideoDecoder implementation.
  bool FeedForPlatformMediaVideoDecoder(
      const scoped_refptr<DecoderBuffer>& buffer) override;

 private:
  scoped_refptr<MediaPlatformAPI> media_platform_api_;

  DISALLOW_COPY_AND_ASSIGN(VideoDecoderMock);
};

}  // namespace media

#endif  // MEDIA_FILTERS_NEVA_VIDEO_DECODER_MOCK_H_
