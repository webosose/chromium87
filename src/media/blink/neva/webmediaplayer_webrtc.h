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

#ifndef MEDIA_BLINK_NEVA_WEBMEDIAPLAYER_WEBRTC_H_
#define MEDIA_BLINK_NEVA_WEBMEDIAPLAYER_WEBRTC_H_

#include <stdint.h>

#include <memory>
#include <string>
#include <vector>

#include "media/base/media_log.h"
#include "media/blink/neva/video_frame_provider_impl.h"
#include "media/blink/neva/webmediaplayer_params_neva.h"
#include "media/neva/media_platform_api.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "third_party/blink/public/platform/web_media_player_source.h"
#include "third_party/blink/public/platform/web_rect.h"
#include "third_party/blink/public/web/modules/mediastream/webmediaplayer_ms.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"
#include "ui/gfx/geometry/point_f.h"
#include "ui/gfx/geometry/rect_f.h"
#include "ui/platform_window/neva/mojom/video_window_controller.mojom.h"

#if defined(USE_VIDEO_TEXTURE)
#include "ui/gl/neva/video_texture.h"
#endif

namespace media {

class MediaPlatformAPI;
class WebMediaPlayerImpl;
class VideoHoleGeometryUpdateHelper;

class MEDIA_BLINK_EXPORT WebMediaPlayerWebRTC
    : public ui::mojom::VideoWindowClient,
      public blink::WebMediaPlayerMS {
 public:
  // Constructs a WebMediaPlayer implementation using Chromium's media stack.
  // |delegate| may be null.
  WebMediaPlayerWebRTC(
      blink::WebLocalFrame* frame,
      blink::WebMediaPlayerClient* client,
      blink::WebMediaPlayerDelegate* delegate,
      std::unique_ptr<media::MediaLog> media_log,
      scoped_refptr<base::SingleThreadTaskRunner> main_render_task_runner,
      scoped_refptr<base::SingleThreadTaskRunner> io_task_runner,
      scoped_refptr<base::SingleThreadTaskRunner> compositor_task_runner,
      scoped_refptr<base::SingleThreadTaskRunner> media_task_runner,
      scoped_refptr<base::TaskRunner> worker_task_runner,
      media::GpuVideoAcceleratorFactories* gpu_factories,
      const blink::WebString& sink_id,
      blink::CreateSurfaceLayerBridgeCB create_bridge_callback,
      std::unique_ptr<blink::WebVideoFrameSubmitter> submitter,
      blink::WebMediaPlayer::SurfaceLayerMode surface_layer_mode,
      const StreamTextureFactoryCreateCB& stream_texture_factory_create_cb,
      std::unique_ptr<WebMediaPlayerParamsNeva> params_neva);
  ~WebMediaPlayerWebRTC() override;
  WebMediaPlayerWebRTC(const WebMediaPlayerWebRTC&) = delete;
  WebMediaPlayerWebRTC& operator=(const WebMediaPlayerWebRTC&) = delete;

  blink::WebMediaPlayer::LoadTiming Load(
      LoadType load_type,
      const blink::WebMediaPlayerSource& source,
      CorsMode cors_mode) override;

  void Play() override;
  void Pause() override;
  void SetRate(double rate) override;
  void SetVolume(double volume) override;

  void EnteredFullscreen() override;
  void ExitedFullscreen() override;

  // WebMediaPlayerDelegate::Observer interface stubs
  void OnFrameHidden() override;
  void OnFrameShown() override;
  void OnFrameClosed() override;
  void OnVolumeMultiplierUpdate(double multiplier) override {}
  void OnBecamePersistentVideo(bool value) override {}
  void OnMediaActivationPermitted() override;

  // Implements ui::mojom::VideoWindowClient
  void OnVideoWindowCreated(const ui::VideoWindowInfo& info) override;
  void OnVideoWindowDestroyed() override;
  void OnVideoWindowGeometryChanged(const gfx::Rect& rect) override;
  void OnVideoWindowVisibilityChanged(bool visibility) override;
  // End of blink::mojom::VideoWindowClient

  void SetRenderMode(blink::WebMediaPlayer::RenderMode mode) override;
  void SetDisableAudio(bool disable) override;

  // Overridden from parent blink::WebMediaPlayerMS
  bool HandleVideoFrame(
      const scoped_refptr<media::VideoFrame>& video_frame) override;

  void TriggerResize() override;
  void OnFirstFrameReceived(media::VideoRotation video_rotation,
                            bool is_opaque) override;
  void OnRotationChanged(media::VideoRotation video_rotation) override;

 private:
  enum class StatusOnSuspended {
    UnknownStatus = 0,
    PlayingStatus,
    PausedStatus,
  };

  void HandleEncodedFrame(const scoped_refptr<media::VideoFrame>& frame);

  void StartMediaPipeline(const scoped_refptr<media::VideoFrame>& input_frame);
  void InitMediaPlatformAPI(
      const scoped_refptr<media::VideoFrame>& input_frame);
  void ReleaseMediaPlatformAPI();

  void OnPipelineFeed();

  bool is_render_mode_texture() const {
    return render_mode_ == blink::WebMediaPlayer::RenderModeTexture;
  }

  void SuspendInternal();
  void ResumeInternal();

  void OnLoadPermitted();
  void OnNaturalVideoSizeChanged(const gfx::Size& natural_video_size);
  void OnResumed();
  void OnSuspended();

  bool EnsureVideoWindowCreated();
  void ContinuePlayerWithWindowId();

  void OnMediaPlatformAPIInitialized(PipelineStatus status);
  void OnPipelineError(PipelineStatus status);

  void EnqueueHoleFrame(const scoped_refptr<media::VideoFrame>& output_frame);
  VideoDecoderConfig GetVideoConfig(
      const scoped_refptr<media::VideoFrame>& video_frame);
  std::unique_ptr<VideoFrameProviderImpl> video_frame_provider_impl_;

  const gfx::PointF additional_contents_scale_;

  const scoped_refptr<base::SingleThreadTaskRunner> main_task_runner_;

  std::string app_id_;
  StatusOnSuspended status_on_suspended_ = StatusOnSuspended::UnknownStatus;

  scoped_refptr<MediaPlatformAPI> media_platform_api_;

  // This value is updated by using value from media platform api.
  gfx::Size frame_size_;
  gfx::Size natural_video_size_;

  bool is_loading_ = false;
  LoadType pending_load_type_ = blink::WebMediaPlayer::kLoadTypeMediaSource;
  CorsMode pending_cors_mode_ = WebMediaPlayer::kCorsModeUnspecified;
  blink::WebMediaStream pending_stream_;
  bool pending_load_media_ = false;

  blink::WebMediaPlayer::RenderMode render_mode_ =
      blink::WebMediaPlayer::RenderModeNone;

  bool is_suspended_ = false;

  base::TimeDelta paused_time_;
  PipelineStatus pipeline_status_ = PIPELINE_OK;

  // |frame_lock_| protects |pending_encoded_frames_|.
  base::Lock frame_lock_;
  std::vector<scoped_refptr<media::VideoFrame>> pending_encoded_frames_;

  VideoCodec codec_ = media::kUnknownVideoCodec;

  // Whether or not the pipeline is running.
  bool pipeline_running_ = false;
  bool is_destroying_ = false;

  bool handle_encoded_frames_ = false;

  bool has_activation_permit_ = false;

  CreateMediaPlatformAPICB create_media_platform_api_cb_;

  base::RepeatingCallback<void()> software_fallback_callback_;

  WebMediaPlayerParamsNeva::CreateVideoWindowCB create_video_window_cb_;
  base::Optional<ui::VideoWindowInfo> video_window_info_ = base::nullopt;
  mojo::Remote<ui::mojom::VideoWindow> video_window_remote_;
  mojo::Receiver<ui::mojom::VideoWindowClient> video_window_client_receiver_{
      this};

  std::unique_ptr<media::VideoHoleGeometryUpdateHelper> geometry_update_helper_;

  base::WeakPtr<WebMediaPlayerWebRTC> weak_ptr_this_;
  base::WeakPtrFactory<WebMediaPlayerWebRTC> weak_factory_this_;
};

}  // namespace media

#endif  // MEDIA_BLINK_NEVA_WEBMEDIAPLAYER_WEBRTC_H_
