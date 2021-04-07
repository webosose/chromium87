// Copyright 2017-2018 LG Electronics, Inc.
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

#ifndef MEDIA_FILTERS_NEVA_HOLEFRAME_VIDEO_DECODER_H_
#define MEDIA_FILTERS_NEVA_HOLEFRAME_VIDEO_DECODER_H_

#include <list>

#include "base/callback.h"
#include "media/base/video_decoder.h"
#include "media/base/video_decoder_config.h"

namespace base {
class SingleThreadTaskRunner;
}

namespace media {

class DecoderBuffer;

class MEDIA_EXPORT HoleFrameVideoDecoder : public VideoDecoder {
 public:
  HoleFrameVideoDecoder(
      const scoped_refptr<base::SingleThreadTaskRunner>& task_runner);
  ~HoleFrameVideoDecoder() override;

  // VideoDecoder implementation.
  std::string GetDisplayName() const override;
  void Decode(scoped_refptr<DecoderBuffer> buffer, DecodeCB decode_cb) override;
  void Reset(base::OnceClosure closure) override;

  virtual bool FeedForPlatformMediaVideoDecoder(
      const scoped_refptr<DecoderBuffer>& buffer) = 0;

 protected:
  enum DecoderState {
    kUninitialized,
    kNormal,
    kDecodeFinished,
    kError
  };

  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;

  DecoderState state_;

  OutputCB output_cb_;

  VideoDecoderConfig config_;

  DISALLOW_COPY_AND_ASSIGN(HoleFrameVideoDecoder);
};

}  // namespace media

#endif  // MEDIA_FILTERS_NEVA_HOLEFRAME_VIDEO_DECODER_H_
