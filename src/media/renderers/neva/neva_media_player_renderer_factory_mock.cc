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

#include "media/renderers/neva/neva_media_player_renderer_factory.h"

#include "base/bind.h"
#include "base/single_thread_task_runner.h"
#include "media/base/audio_decoder.h"
#include "media/base/video_decoder.h"
#include "media/filters/neva/audio_decoder_mock.h"
#include "media/filters/neva/video_decoder_mock.h"
#include "media/neva/media_platform_api.h"
#include "media/renderers/audio_renderer_impl.h"
#include "media/renderers/renderer_impl.h"
#include "media/renderers/video_renderer_impl.h"
#include "media/video/gpu_memory_buffer_video_frame_pool.h"

namespace media {

NevaMediaPlayerRendererFactory::NevaMediaPlayerRendererFactory(
    MediaLog* media_log,
    DecoderFactory* decoder_factory,
    const GetGpuFactoriesCB& get_gpu_factories_cb) {
  media_log_ = media_log;
  decoder_factory_ = decoder_factory;
}

NevaMediaPlayerRendererFactory::~NevaMediaPlayerRendererFactory() {}

bool NevaMediaPlayerRendererFactory::Enabled() {
  return true;
}

std::unique_ptr<Renderer> NevaMediaPlayerRendererFactory::CreateRenderer(
    const scoped_refptr<base::SingleThreadTaskRunner>& media_task_runner,
    const scoped_refptr<base::TaskRunner>& worker_task_runner,
    AudioRendererSink* audio_renderer_sink,
    VideoRendererSink* video_renderer_sink,
    RequestOverlayInfoCB request_overlay_info_cb,
    const gfx::ColorSpace& target_color_space) {
  std::unique_ptr<AudioRenderer> audio_renderer(new AudioRendererImpl(
      media_task_runner, audio_renderer_sink,
      base::Bind(&NevaMediaPlayerRendererFactory::CreateAudioDecoders,
                 base::Unretained(this), media_task_runner),
      media_log_, nullptr));

  std::unique_ptr<GpuMemoryBufferVideoFramePool> gmb_pool;
  std::unique_ptr<VideoRenderer> video_renderer(new VideoRendererImpl(
      media_task_runner, video_renderer_sink,
      base::Bind(&NevaMediaPlayerRendererFactory::CreateVideoDecoders,
                 base::Unretained(this), media_task_runner,
                 request_overlay_info_cb, target_color_space, nullptr),
      true, media_log_, std::move(gmb_pool)));

  return std::make_unique<RendererImpl>(
      media_task_runner, std::move(audio_renderer), std::move(video_renderer));
}

std::vector<std::unique_ptr<AudioDecoder>>
NevaMediaPlayerRendererFactory::CreateAudioDecoders(
    const scoped_refptr<base::SingleThreadTaskRunner>& media_task_runner) {
  std::vector<std::unique_ptr<AudioDecoder>> audio_decoders;
  audio_decoders.push_back(std::make_unique<AudioDecoderMock>(
      media_task_runner, media_platform_api_));
  return audio_decoders;
}

std::vector<std::unique_ptr<VideoDecoder>>
NevaMediaPlayerRendererFactory::CreateVideoDecoders(
    const scoped_refptr<base::SingleThreadTaskRunner>& media_task_runner,
    const RequestOverlayInfoCB& request_overlay_info_cb,
    const gfx::ColorSpace& target_color_space,
    GpuVideoAcceleratorFactories* gpu_factories) {
  std::vector<std::unique_ptr<VideoDecoder>> video_decoders;
  video_decoders.push_back(std::make_unique<VideoDecoderMock>(
      media_task_runner, media_platform_api_));
  return video_decoders;
}

void NevaMediaPlayerRendererFactory::SetMediaPlatformAPI(
    const scoped_refptr<MediaPlatformAPI>& media_platform_api) {
  media_platform_api_ = media_platform_api;
}

}  // namespace media
