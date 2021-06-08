// Copyright 2014-2020 LG Electronics, Inc.
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

#ifndef MEDIA_BLINK_NEVA_WEBMEDIAPLAYER_NEVA_H_
#define MEDIA_BLINK_NEVA_WEBMEDIAPLAYER_NEVA_H_

#include <memory>
#include <string>
#include <vector>

#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/thread.h"
#include "base/time/default_tick_clock.h"
#include "base/timer/timer.h"
#include "cc/layers/video_frame_provider.h"
#include "media/base/audio_renderer_sink.h"
#include "media/base/pipeline.h"
#include "media/base/time_delta_interpolator.h"
#include "media/base/video_frame.h"
#include "media/blink/media_blink_export.h"
#include "media/blink/neva/media_info_loader.h"
#include "media/blink/neva/video_frame_provider_impl.h"
#include "media/blink/neva/webmediaplayer_params_neva.h"
#include "media/neva/media_player_neva_factory.h"
#include "media/neva/media_player_neva_interface.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "services/media_session/public/cpp/media_position.h"
#include "third_party/blink/public/platform/media/webmediaplayer_delegate.h"
#include "third_party/blink/public/platform/web_audio_source_provider.h"
#include "third_party/blink/public/platform/web_media_player.h"
#include "third_party/blink/public/platform/web_media_player_client.h"
#include "third_party/blink/public/platform/web_media_player_source.h"
#include "third_party/blink/public/platform/web_rect.h"
#include "third_party/blink/public/platform/web_size.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/platform/web_vector.h"
#include "ui/platform_window/neva/mojom/video_window_controller.mojom.h"
#include "url/gurl.h"

namespace blink {
class WebLocalFrame;
}

namespace cc_blink {
class WebLayerImpl;
}

namespace base {
class SingleThreadTaskRunner;
}

namespace cc {
class VideoLayer;
}

namespace gpu {
struct SyncToken;
}

namespace content {
class StreamTextureFactory;
class RenderThreadImpl;
}  // namespace content

namespace media {

class MediaLog;
class VideoHoleGeometryUpdateHelper;
class WebAudioSourceProviderImpl;
class WebMediaPlayerParams;

class MEDIA_BLINK_EXPORT WebMediaPlayerNeva
    : public blink::WebMediaPlayer,
      public blink::WebMediaPlayerDelegate::Observer,
      public ui::mojom::VideoWindowClient,
      public media::MediaPlayerNevaClient {
 public:
  enum StatusOnSuspended {
    UnknownStatus = 0,
    PlayingStatus,
    PausedStatus,
  };

  using MediaTrackId = std::pair<WebMediaPlayer::TrackId, std::string>;

  static bool CanSupportMediaType(const std::string& mime);
  static blink::WebMediaPlayer* Create(
      blink::WebLocalFrame* frame,
      blink::WebMediaPlayerClient* client,
      blink::WebMediaPlayerDelegate* delegate,
      const StreamTextureFactoryCreateCB& stream_texture_factory_create_cb,
      std::unique_ptr<WebMediaPlayerParams> params,
      std::unique_ptr<WebMediaPlayerParamsNeva> params_neva);

  // This class implements blink::WebMediaPlayer by keeping the private media
  // player api which is supported by target platform
  WebMediaPlayerNeva(
      blink::WebLocalFrame* frame,
      blink::WebMediaPlayerClient* client,
      blink::WebMediaPlayerDelegate* delegate,
      const StreamTextureFactoryCreateCB& stream_texture_factory_create_cb,
      const media::MediaPlayerType media_player_type,
      std::unique_ptr<WebMediaPlayerParams> params,
      std::unique_ptr<WebMediaPlayerParamsNeva> params_neva);
  ~WebMediaPlayerNeva() override;

  blink::WebMediaPlayer::LoadTiming Load(LoadType load_type,
                                         const blink::WebMediaPlayerSource& src,
                                         CorsMode cors_mode) override;

  // Playback controls.
  void Play() override;
  void Pause() override;
  void Seek(double seconds) override;
  void SetRate(double rate) override;
  void SetVolume(double volume) override;
  void SetLatencyHint(double seconds) override;
  void SetPreservesPitch(bool preserves_pitch) override;
  void OnRequestPictureInPicture() override {}
  void OnTimeUpdate() override;
  void OnPictureInPictureAvailabilityChanged(bool available) override;

  blink::WebTimeRanges Buffered() const override;
  blink::WebTimeRanges Seekable() const override;

  // Attempts to switch the audio output device.
  // Implementations of SetSinkId take ownership of the WebSetSinkCallbacks
  // object.
  // Note also that SetSinkId implementations must make sure that all
  // methods of the WebSetSinkCallbacks object, including constructors and
  // destructors, run in the same thread where the object is created
  // (i.e., the blink thread).
  void SetSinkId(const blink::WebString& sing_id,
                 blink::WebSetSinkIdCompleteCallback) override {}

  // Methods for painting.
  // FIXME: This path "only works" on Android. It is a workaround for the
  // issue that Skia could not handle Android's GL_TEXTURE_EXTERNAL_OES texture
  // internally. It should be removed and replaced by the normal paint path.
  // https://code.google.com/p/skia/issues/detail?id=1189
  void Paint(
      cc::PaintCanvas*,
      const blink::WebRect&,
      cc::PaintFlags&,
      int already_uploaded_id,
      blink::WebMediaPlayer::VideoFrameUploadMetadata* out_metadata) override;

  // True if the loaded media has a playable video/audio track.
  bool HasVideo() const override;
  bool HasAudio() const override;

  bool SupportsFullscreen() const;
  void SetPreload(Preload) override;

  // Dimensions of the video.
  gfx::Size NaturalSize() const override;

  gfx::Size VisibleSize() const override;

  // Getters of playback state.
  bool Paused() const override;
  bool Seeking() const override;
  double Duration() const override;
  double CurrentTime() const override;
  bool IsEnded() const override;

  // Internal states of loading and network.
  blink::WebMediaPlayer::NetworkState GetNetworkState() const override;
  blink::WebMediaPlayer::ReadyState GetReadyState() const override;

  blink::WebMediaPlayer::SurfaceLayerMode GetVideoSurfaceLayerMode()
      const override;

  blink::WebString GetErrorMessage() const override;
  bool DidLoadingProgress() override;
  bool WouldTaintOrigin() const override;

  double MediaTimeForTimeValue(double timeValue) const override;

  unsigned DecodedFrameCount() const override;
  unsigned DroppedFrameCount() const override;
  uint64_t AudioDecodedByteCount() const override;
  uint64_t VideoDecodedByteCount() const override;

  // Returns true if the player has a frame available for presentation. Usually
  // this just means the first frame has been delivered.
  bool HasAvailableVideoFrame() const override;

  // media::MediaPlayerNevaClinet callback implementation
  void OnMediaMetadataChanged(base::TimeDelta duration,
                              int width,
                              int height,
                              bool success) override;
  void OnLoadComplete() override;
  void OnPlaybackComplete() override;
  void OnBufferingUpdate(int percentage) override;
  // void OnBufferingUpdate(double begin, double end) override;
  void OnSeekComplete(const base::TimeDelta& current_time) override;
  void OnMediaError(int error_type) override;
  void OnVideoSizeChanged(int width, int height) override;
  void OnAudioTracksUpdated(
      const std::vector<MediaTrackInfo>& audio_track_info) override;

  // Called to update the current time.
  void OnTimeUpdate(base::TimeDelta current_timestamp,
                    base::TimeTicks current_time_ticks) override;

  // Functions called when media player status changes.
  void OnMediaPlayerPlay()
      override;  // TODO(wanchang): need to check if it is required
  void OnMediaPlayerPause()
      override;  // TODO(wanchang): need to check if it is required

  void OnCustomMessage(const media::MediaEventType,
                       const std::string& detail) override;

  // Function called when audio focus is actually changed.
  void OnAudioFocusChanged() override;

  bool CopyVideoTextureToPlatformTexture(
      gpu::gles2::GLES2Interface* web_graphics_context,
      unsigned int target,
      unsigned int texture,
      unsigned internal_format,
      unsigned format,
      unsigned type,
      int level,
      bool premultiply_alpha,
      bool flip_y,
      int already_uploaded_id,
      VideoFrameUploadMetadata* out_metadata) override;
  base::WeakPtr<WebMediaPlayer> AsWeakPtr() override;

  // WebMediaPlayerDelegate::Observer interface stubs
  void OnFrameHidden() override;
  void OnFrameClosed() override {}
  void OnFrameShown() override;
  void OnIdleTimeout() override {}
  void OnPlay() override;
  void OnPause() override;
  // TODO(neva) : This method has been added during chromium72 upgrade.
  // Need to revisit and confirm that does not cause any regression
  void OnMuted(bool muted) override {}
  void OnSeekForward(double seconds) override {}
  void OnSeekBackward(double seconds) override {}
  void OnEnterPictureInPicture() override;
  void OnExitPictureInPicture() override;
  void OnSetAudioSink(const std::string& sink_id) override;

  void OnVolumeMultiplierUpdate(double multiplier) override {}
  void OnBecamePersistentVideo(bool value) override {}
  void OnSuspend() override;
  void OnMediaActivationPermitted() override;

  void OnResume();
  void OnLoadPermitted();

  scoped_refptr<blink::WebAudioSourceProviderImpl> GetAudioSourceProvider()
      override;

  void OnActiveRegionChanged(const gfx::Rect&) override;

  void Repaint();

  void SetOpaque(bool opaque);

  bool UseVideoTexture();

  // neva::WebMediaPlayer implementaions
  bool UsesIntrinsicSize() const override;
  blink::WebString MediaId() const override;
  bool HasAudioFocus() const override;
  void SetAudioFocus(bool focus) override;
  void SetRenderMode(blink::WebMediaPlayer::RenderMode mode) override;
  void SetDisableAudio(bool) override;

  void EnabledAudioTracksChanged(
      const blink::WebVector<TrackId>& enabledTrackIds) override;

  bool RenderTexture() {
    return render_mode_ == blink::WebMediaPlayer::RenderModeTexture;
  }
  bool Send(const std::string& message) override;

 private:
  // void OnPipelinePlaybackStateChanged(bool playing);
  void UpdatePlayingState(bool is_playing);

  // Helpers that set the network/ready state and notifies the client if
  // they've changed.
  void UpdateNetworkState(blink::WebMediaPlayer::NetworkState state);
  void UpdateReadyState(blink::WebMediaPlayer::ReadyState state);

  bool IsHLSStream() const;

  void DoLoad(LoadType load_type, const blink::WebURL& url, CorsMode cors_mode);
  void DidLoadMediaInfo(bool ok, const GURL& redirected_url);
  void LoadMedia();

  // Called after asynchronous initialization of a data source completed.
  void DataSourceInitialized(const GURL& gurl, bool success);

  // Called when the data source is downloading or paused.
  void NotifyDownloading(bool is_downloading);

#if defined(NEVA_VIDEO_HOLE)
  void SetDisplayWindow(const gfx::Rect& disp_rect,
                        const gfx::Rect& src_rect,
                        bool fullscreen);
  void SetVisibility(bool visibility);

  void EnteredFullscreen() override;
  void ExitedFullscreen() override;
#endif  // defined(NEVA_VIDEO_HOLE)

  // Getter method to |client_|.
  blink::WebMediaPlayerClient* GetClient();

  // Notifies blink of the video size change.
  // void OnVideoSizeChange();

  // for MSE implementation
  void OnMediaSourceOpened(blink::WebMediaSource* web_media_source);

  // Called when a decoder detects that the key needed to decrypt the stream
  // is not available.
  // void OnWaitingForDecryptionKey() override;
  // end of MSE implementation
  //-----------------------------------------------------

  // Implements ui::mojom::VideoWindowClient
  void OnVideoWindowCreated(const ui::VideoWindowInfo& info) override;
  void OnVideoWindowDestroyed() override;
  void OnVideoWindowGeometryChanged(const gfx::Rect& rect) override;
  void OnVideoWindowVisibilityChanged(bool visibility) override;
  // End of ui::mojom::VideoWindowClient

  bool EnsureVideoWindowCreated();
  void ContinuePlayerWithWindowId();

  blink::WebLocalFrame* frame_;

  // Task runner for posting tasks on Chrome's main thread. Also used
  // for DCHECKs so methods calls won't execute in the wrong thread.
  const scoped_refptr<base::SingleThreadTaskRunner> main_task_runner_;

  blink::WebMediaPlayerClient* client_;

  // WebMediaPlayer notifies the |delegate_| of playback state changes using
  // |delegate_id_|; an id provided after registering with the delegate.  The
  // WebMediaPlayer may also receive directives (play, pause) from the delegate
  // via the WebMediaPlayerDelegate::Observer interface after registration.
  blink::WebMediaPlayerDelegate* delegate_;
  int delegate_id_;

  // Callback responsible for determining if loading of media should be deferred
  // for external reasons; called during load().
  WebMediaPlayerParams::DeferLoadCB defer_load_cb_;

  // Size of the video.
  blink::WebSize natural_size_;

  // The video frame object used for rendering by the compositor.
  scoped_refptr<media::VideoFrame> current_frame_;
  base::Lock current_frame_lock_;

  // URL of the media file to be fetched.
  GURL url_;

  // URL of the media file after |media_info_loader_| resolves all the
  // redirections.
  GURL redirected_url_;

  // Media duration.
  base::TimeDelta duration_;

  double volume_ = 0.0f;

  bool is_negative_playback_rate_ = false;

  // Seek gets pending if another seek is in progress. Only last pending seek
  // will have effect.
  bool pending_seek_;
  base::TimeDelta pending_seek_time_;

  // Internal seek state.
  bool seeking_;
  base::TimeDelta seek_time_;

  // Whether loading has progressed since the last call to didLoadingProgress.
  bool did_loading_progress_;

  // Private MediaPlayer API instance
  std::unique_ptr<MediaPlayerNeva> player_api_;
  CreateMediaPlayerNevaCB create_media_player_neva_cb_;

  // TODO(hclam): get rid of these members and read from the pipeline directly.
  blink::WebMediaPlayer::NetworkState network_state_;
  blink::WebMediaPlayer::ReadyState ready_state_;

  // Whether the media player is playing.
  bool is_playing_;

  // Whether the video size info is available.
  bool has_size_info_;

  // A pointer back to the compositor to inform it about state changes. This is
  // not NULL while the compositor is actively using this webmediaplayer.
  // Accessed on main thread and on compositor thread when main thread is
  // blocked.
  cc::VideoFrameProvider::Client* video_frame_provider_client_;

  // The compositor layer for displaying the video content when using composited
  // playback.
  scoped_refptr<cc::VideoLayer> video_layer_;

#if defined(NEVA_VIDEO_HOLE)
  // A rectangle represents the geometry of video frame, when computed last
  // time.
  gfx::Rect last_computed_rect_in_view_space_;
#endif  // defined(NEVA_VIDEO_HOLE)

  std::unique_ptr<MediaLog> media_log_;

  std::unique_ptr<MediaInfoLoader> info_loader_;

  // base::TickClock used by |interpolator_|.
  base::DefaultTickClock default_tick_clock_;

  // Tracks the most recent media time update and provides interpolated values
  // as playback progresses.
  TimeDeltaInterpolator interpolator_;

  // TODO(wanchang): fix it
  // std::unique_ptr<MediaSourceDelegate> media_source_delegate_;

  // Whether OnPlaybackComplete() has been called since the last playback.
  bool playback_completed_;

  base::TimeDelta paused_time_;

  scoped_refptr<blink::WebAudioSourceProviderImpl> audio_source_provider_;

  bool is_suspended_;
  StatusOnSuspended status_on_suspended_;

  base::RepeatingTimer paintTimer_;

  std::vector<MediaTrackId> audio_track_ids_;

  const scoped_refptr<base::SingleThreadTaskRunner> compositor_task_runner_;
  std::unique_ptr<VideoFrameProviderImpl> video_frame_provider_;
  blink::WebMediaPlayer::RenderMode render_mode_;

  blink::WebRect active_video_region_;
  bool active_video_region_changed_;

  std::string app_id_;

  bool is_loading_;
  LoadType pending_load_type_ = blink::WebMediaPlayer::kLoadTypeURL;
  blink::WebMediaPlayerSource pending_source_;
  CorsMode pending_cors_mode_ = WebMediaPlayer::kCorsModeUnspecified;

  bool has_activation_permit_ = false;

  bool audio_disabled_ = false;

  bool has_first_frame_ = false;
  bool pending_load_media_ = false;
  base::Optional<Preload> pending_preload_ = base::nullopt;
  WebMediaPlayerParamsNeva::CreateVideoWindowCB create_video_window_cb_;
  base::Optional<ui::VideoWindowInfo> video_window_info_ = base::nullopt;
  mojo::Remote<ui::mojom::VideoWindow> video_window_remote_;
  mojo::Receiver<ui::mojom::VideoWindowClient> video_window_client_receiver_{
      this};

  media_session::MediaPosition media_position_state_;
  double playback_rate_;

  std::unique_ptr<media::VideoHoleGeometryUpdateHelper> geometry_update_helper_;

  base::WeakPtr<WebMediaPlayerNeva> weak_this_;
  base::WeakPtrFactory<WebMediaPlayerNeva> weak_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(WebMediaPlayerNeva);
};

}  // namespace media

#endif  // MEDIA_BLINK_NEVA_WEBMEDIAPLAYER_NEVA_H_
