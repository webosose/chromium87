// Copyright 2015-2018 LG Electronics, Inc.
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

#include <utility>

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "base/single_thread_task_runner.h"
#include "build/build_config.h"
#include "media/base/decoder_factory.h"
#include "media/filters/neva/webos/audio_decoder_webos.h"
#include "media/filters/neva/webos/video_decoder_webos.h"
#include "media/neva/media_platform_api.h"
#include "media/renderers/neva/external_renderer.h"
#include "media/video/gpu_video_accelerator_factories.h"

#if !defined(MEDIA_DISABLE_FFMPEG)
#include "media/filters/ffmpeg_audio_decoder.h"
#if !defined(DISABLE_FFMPEG_VIDEO_DECODERS)
#include "media/filters/ffmpeg_video_decoder.h"
#endif
#endif

#if !defined(MEDIA_DISABLE_LIBVPX)
#include "media/filters/vpx_video_decoder.h"
#endif

namespace media {

NevaMediaPlayerRendererFactory::NevaMediaPlayerRendererFactory(
    MediaLog* media_log,
    DecoderFactory* decoder_factory,
    const GetGpuFactoriesCB& get_gpu_factories_cb)
    : media_log_(media_log),
      decoder_factory_(decoder_factory),
      get_gpu_factories_cb_(get_gpu_factories_cb) {}

NevaMediaPlayerRendererFactory::~NevaMediaPlayerRendererFactory() {
}

// static
bool NevaMediaPlayerRendererFactory::Enabled() {
  return true;
}

std::vector<std::unique_ptr<AudioDecoder>>
NevaMediaPlayerRendererFactory::CreateAudioDecoders(
    const scoped_refptr<base::SingleThreadTaskRunner>& media_task_runner) {
  // Create our audio decoders and renderer.
  std::vector<std::unique_ptr<AudioDecoder>> audio_decoders;

  audio_decoders.push_back(std::make_unique<AudioDecoderWebOS>(
      media_task_runner, media_platform_api_));

  decoder_factory_->CreateAudioDecoders(media_task_runner, media_log_,
                                        &audio_decoders);

  return audio_decoders;
}

std::vector<std::unique_ptr<VideoDecoder>>
NevaMediaPlayerRendererFactory::CreateVideoDecoders(
    const scoped_refptr<base::SingleThreadTaskRunner>& media_task_runner,
    const RequestOverlayInfoCB& request_overlay_info_cb,
    const gfx::ColorSpace& target_color_space,
    GpuVideoAcceleratorFactories* gpu_factories) {
  // Create our video decoders and renderer.
  std::vector<std::unique_ptr<VideoDecoder>> video_decoders;

  // Prefer an external decoder since one will only exist if it is hardware
  // accelerated.
  video_decoders.push_back(std::make_unique<VideoDecoderWebOS>(
      media_task_runner, media_platform_api_));

  decoder_factory_->CreateVideoDecoders(media_task_runner, gpu_factories,
                                        media_log_, request_overlay_info_cb,
                                        target_color_space, &video_decoders);

  return video_decoders;
}

std::unique_ptr<Renderer> NevaMediaPlayerRendererFactory::CreateRenderer(
    const scoped_refptr<base::SingleThreadTaskRunner>& media_task_runner,
    const scoped_refptr<base::TaskRunner>& worker_task_runner,
    AudioRendererSink* audio_renderer_sink,
    VideoRendererSink* video_renderer_sink,
    RequestOverlayInfoCB request_overlay_info_cb,
    const gfx::ColorSpace& target_color_space) {
  DCHECK(audio_renderer_sink);
  DCHECK(media_platform_api_) << "WebOS Media API uninitialized";

  GpuVideoAcceleratorFactories* gpu_factories = nullptr;
  if (!get_gpu_factories_cb_.is_null())
    gpu_factories = get_gpu_factories_cb_.Run();

  // Create ExternalRenderer.
  std::unique_ptr<ExternalRenderer> renderer(new ExternalRenderer(
      media_task_runner, media_platform_api_,
      base::Bind(&NevaMediaPlayerRendererFactory::CreateAudioDecoders,
                 base::Unretained(this), media_task_runner),
      base::Bind(&NevaMediaPlayerRendererFactory::CreateVideoDecoders,
                 base::Unretained(this), media_task_runner,
                 request_overlay_info_cb, target_color_space, gpu_factories),
      media_log_, NULL));

  return std::move(renderer);
}

void NevaMediaPlayerRendererFactory::SetMediaPlatformAPI(
    const scoped_refptr<MediaPlatformAPI>& media_platform_api) {
  media_platform_api_ = media_platform_api;
}

}  // namespace media
