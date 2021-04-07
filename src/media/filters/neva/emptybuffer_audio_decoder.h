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

#ifndef MEDIA_FILTERS_NEVA_EMPTYBUFFER_AUDIO_DECODER_H_
#define MEDIA_FILTERS_NEVA_EMPTYBUFFER_AUDIO_DECODER_H_

#include "base/callback.h"
#include "media/base/audio_decoder.h"
#include "media/base/audio_decoder_config.h"

namespace base {
class SingleThreadTaskRunner;
}

namespace media {

class AudioDiscardHelper;
class DecoderBuffer;

class MEDIA_EXPORT EmptyBufferAudioDecoder : public AudioDecoder {
 public:
  EmptyBufferAudioDecoder(
      const scoped_refptr<base::SingleThreadTaskRunner>& task_runner);
  ~EmptyBufferAudioDecoder() override;

  // AudioDecoder implementation.
  std::string GetDisplayName() const override;
  void Decode(scoped_refptr<DecoderBuffer> buffer, DecodeCB decode_cb) override;
  void Reset(base::OnceClosure closure) override;

  virtual bool FeedForPlatformMediaAudioDecoder(
      const scoped_refptr<DecoderBuffer>& buffer) = 0;

 protected:
  enum DecoderState { kUninitialized, kNormal, kDecodeFinished, kError };

  // Reset decoder and call |reset_cb_|.
  void DoReset();

  // Handles decoding an unencrypted encoded buffer.
  void DecodeBuffer(const scoped_refptr<DecoderBuffer>& buffer,
                    DecodeCB decode_cb);
  // Handles (re-)initializing the decoder with a (new) config.
  // Returns true if initialization was successful.
  bool ConfigureDecoder();

  // Releases resources associated with |codec_context_| and |av_frame_|
  // and resets them to NULL.
  void ResetTimestampState();

  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;

  DecoderState state_;

  OutputCB output_cb_;
  InitCB inut_cb_;

  AudioDecoderConfig config_;

  // AVSampleFormat initially requested; not Chrome's SampleFormat.
  int av_sample_format_;

  std::unique_ptr<AudioDiscardHelper> discard_helper_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(EmptyBufferAudioDecoder);
};

}  // namespace media

#endif  // MEDIA_FILTERS_NEVA_EMPTYBUFFER_AUDIO_DECODER_H_
