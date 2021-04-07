// Copyright 2019-2020 LG Electronics, Inc. All rights reserved.
// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// The implementation is based on RendererImpl in renderer_impl.h

#ifndef MEDIA_RENDERERS_NEVA_EXTERNAL_RENDERER_H_
#define MEDIA_RENDERERS_NEVA_EXTERNAL_RENDERER_H_

#include <stddef.h>
#include <stdint.h>

#include <memory>

#include "base/callback_forward.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/timer/timer.h"
#include "media/base/cdm_context.h"
#include "media/base/decryptor.h"
#include "media/base/demuxer_stream.h"
#include "media/base/media_export.h"
#include "media/base/media_log.h"
#include "media/base/pipeline_status.h"
#include "media/base/time_source.h"
#include "media/base/video_decoder.h"
#include "media/base/video_decoder_config.h"
#include "media/base/video_frame.h"
#include "media/base/video_renderer_sink.h"
#include "media/base/wall_clock_time_source.h"
#include "media/filters/decoder_stream.h"
#include "media/filters/video_renderer_algorithm.h"
#include "media/neva/media_platform_api.h"
#include "media/renderers/default_renderer_factory.h"
#include "media/video/gpu_memory_buffer_video_frame_pool.h"

namespace base {
class SingleThreadTaskRunner;
class TickClock;
}  // namespace base

namespace media {

class NullVideoSink;
class WallClockTimeSource;

using CreateAudioDecodersCB =
    base::RepeatingCallback<std::vector<std::unique_ptr<AudioDecoder>>()>;
using CreateVideoDecodersCB =
    base::RepeatingCallback<std::vector<std::unique_ptr<VideoDecoder>>()>;

// NOTE(neva): ExternalRenderer reads Audio/Video decoder to feed audio/video
// stream data to external platform media-pipeline. The external media-pipeline
// will render audio/video frames outside of chromium. The basic concept of
// state transition and checking states come from RendererImpl but main
// work-flow is our own implementation. When it has any conflict with upstream
// we better to refer to RendererImp.
class MEDIA_EXPORT ExternalRenderer : public Renderer,
                                      public VideoRendererSink::RenderCallback {
 public:
  // Renders audio/video streams using external media-pipeline. All methods
  // except for GetMediaTime() run on the |task_runner|.  GetMediaTime() runs on
  // the render main thread because it's part of JS sync API.
  ExternalRenderer(
      const scoped_refptr<base::SingleThreadTaskRunner>& media_task_runner,
      const scoped_refptr<MediaPlatformAPI>& media_platform_api,
      const CreateAudioDecodersCB& create_audio_decoders_cb,
      const CreateVideoDecodersCB& create_video_decoders_cb,
      MediaLog* media_log,
      std::unique_ptr<GpuMemoryBufferVideoFramePool> gmb_pool);
  ~ExternalRenderer() override;

  // Renderer implementation
  // Initializes the Renderer with |media_resource|, executing |init_cb| upon
  // completion. |media_resource| must be valid for the lifetime of the Renderer
  // object.  |init_cb| must only be run after this method has returned. Firing
  // |init_cb| may result in the immediate destruction of the caller, so it must
  // be run only prior to returning.
  void Initialize(MediaResource* media_resource,
                  RendererClient* client,
                  PipelineStatusCallback init_cb) override;

  // Associates the |cdm_context| with this Renderer for decryption (and
  // decoding) of media data, then fires |cdm_attached_cb| with the result.
  void SetCdm(CdmContext* cdm_context, CdmAttachedCB cdm_attached_cb) override;

  void SetLatencyHint(base::Optional<base::TimeDelta> latency_hint) override;

  // The following functions must be called after Initialize().

  // Discards any buffered data, executing |flush_cb| when completed.
  void Flush(base::OnceClosure flush_cb) override;

  // Starts rendering from |time|.
  void StartPlayingFrom(base::TimeDelta time) override;

  // Updates the current playback rate. The default playback rate should be 0.
  void SetPlaybackRate(double playback_rate) override;

  // Sets the output volume. The default volume should be 1.
  void SetVolume(float volume) override;

  // Returns the current media time.
  //
  // This method must be safe to call from any thread.
  base::TimeDelta GetMediaTime() override;

  // Provides a list of DemuxerStreams correlating to the tracks which should
  // be played. An empty list would mean that any playing track of the same
  // type should be flushed and disabled. Any provided Streams should be played
  // by whatever mechanism the subclass of Renderer choses for managing it's AV
  // playback.
  void OnSelectedVideoTracksChanged(
      const std::vector<DemuxerStream*>& enabled_tracks,
      base::OnceClosure change_completed_cb) override;
  void OnEnabledAudioTracksChanged(
      const std::vector<DemuxerStream*>& enabled_tracks,
      base::OnceClosure change_completed_cb) override;
  // End-Of-Renderer

  // VideoRendererSink::RenderCallback implementation.
  scoped_refptr<VideoFrame> Render(base::TimeTicks deadline_min,
                                   base::TimeTicks deadline_max,
                                   bool background_rendering) override;
  void OnFrameDropped() override;
  base::TimeDelta GetPreferredRenderInterval() override;
  // End-Of-VideoRendererSink::RenderCallback

 private:
  void FrameReady(const scoped_refptr<VideoFrame> frame) {}
  void InitializeMediaPlatformAPI();
  void InitializeAudioDecoder();
  void InitializeVideoDecoder();

  void OnMediaPlatformAPIInitialized(PipelineStatus status);
  void OnAudioDecoderStreamInitialized(bool success);
  void OnVideoDecoderStreamInitialized(bool success);

  // Functions to notify certain events to the RendererClient.
  void OnPlaybackError(PipelineStatus error);
  void OnPlaybackEnded();
  void OnStatisticsUpdate(const PipelineStatistics& stats);
  void OnBufferingStateChange(BufferingState state,
                              BufferingStateChangeReason reason);
  void OnWaiting(WaitingReason);

  // Called by the VideoDecoderStream when a config change occurs. Will notify
  // RenderClient of the new config.
  void OnAudioConfigChange(const AudioDecoderConfig& config);
  void OnVideoConfigChange(const VideoDecoderConfig& config);

  // Callback for |{audio|video}_decoder_stream_| to deliver decoded
  // {audio|video} frames and report video decoding status.
  void AudioBufferReady(AudioDecoderStream::ReadStatus status,
                        const scoped_refptr<AudioBuffer> frame);
  void VideoFrameReady(VideoDecoderStream::ReadStatus status,
                       const scoped_refptr<VideoFrame> frame);

  void AddReadyFrame(const scoped_refptr<VideoFrame> frame);

  void AttemptRead();

  void FlushAudioDecoder();
  void FlushVideoDecoder();
  void FinishFlush();
  void OnAudioDecoderStreamResetDone();
  void OnVideoDecoderStreamResetDone();

  // Returns true if the renderer has enough data for playback purposes.
  // Note that having enough data may be due to reaching end of stream.
  bool HaveEnoughData() const;
  void TransitionToHaveEnough();
  void TransitionToHaveNothing();

  // Runs |statistics_cb_| with |frames_decoded_| and |frames_dropped_|, resets
  // them to 0.
  void UpdateStats();

  // TODO(neva): Need to refactor Start/StopSink to manage feed data without
  // NullVideoSink
  void StartSink();
  void StopSink();

  // Fires |ended_cb_| if there are no remaining usable frames and
  // |received_end_of_stream_| is true.  Sets |rendered_end_of_stream_| if it
  // does so.
  void MaybeFireEndedCallback();

  // Helper method for converting a single media timestamp to wall clock time.
  base::TimeTicks ConvertMediaTimestamp(base::TimeDelta media_timestamp);

  base::TimeTicks GetCurrentMediaTimeAsWallClockTime();

  // Helper method for checking if a frame timestamp plus the frame's expected
  // duration is before |start_timestamp_|.
  bool IsBeforeStartTime(base::TimeDelta timestamp);

  // Notifies |client_| in the event of frame size or opacity changes. Must be
  // called on |task_runner_|.
  void CheckForMetadataChanges(VideoPixelFormat pixel_format,
                               const gfx::Size& natural_size);

  // Both calls AttemptRead() and CheckForMetadataChanges(). Must be
  // called on |task_runner_|.
  void AttemptReadAndCheckForMetadataChanges(VideoPixelFormat pixel_format,
                                             const gfx::Size& natural_size);

  bool ReceivedEndOfStream();

  enum Type { Audio, Video, ALL };
  bool ReceivedEOSByType(Type type) const;
  void SetMediaPlatformAPICb();

  void OnPlayerEvent(PlayerEvent e);

  void FinishInitialization(PipelineStatus status);
  bool HasEncryptedStream();

  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;

  // Sink should call Render from media_task_runner
  std::unique_ptr<NullVideoSink> video_sink_;
  bool sink_started_ = false;

  // Stores the last decoder config that was passed to
  // RendererClient::OnVideoConfigChange. Used to prevent signaling config
  // to the upper layers when when the new config is the same.
  AudioDecoderConfig current_audio_decoder_config_;
  VideoDecoderConfig current_video_decoder_config_;

  // Pool of GpuMemoryBuffers and resources used to create hardware frames.
  // Ensure this is destructed after |algorithm_| for optimal memory release
  // when a frames are still held by the compositor. Must be destructed after
  // |video_decoder_stream_| since it holds a callback to the pool.
  std::unique_ptr<GpuMemoryBufferVideoFramePool> gpu_memory_buffer_pool_;

  // Provides video frames to ExternalRenderer.
  std::unique_ptr<AudioDecoderStream> audio_decoder_stream_;
  std::unique_ptr<VideoDecoderStream> video_decoder_stream_;

  bool has_audio_ = false;
  bool has_video_ = false;

  MediaLog* media_log_;

  // Flag indicating low-delay mode.
  bool low_delay_ = false;

  // WEBOS: Queue of incoming frames yet to be painted. (refered from
  // chromium38)
  typedef std::deque<scoped_refptr<VideoFrame>> VideoFrameQueue;
  VideoFrameQueue ready_frames_;

  // Keeps track of whether we received the end of stream buffer and finished
  // rendering.
  bool audio_received_end_of_stream_ = false;
  bool video_received_end_of_stream_ = false;
  bool rendered_end_of_stream_ = false;

  // Important detail: being in kPlaying doesn't imply that video is being
  // rendered. Rather, it means that the renderer is ready to go. The actual
  // rendering of video is controlled by time advancing via |get_time_cb_|.
  // Video renderer can be reinitialized completely by calling Initialize again
  // when it is in a kFlushed state with video sink stopped.
  //
  //   kUninitialized
  //  +------> | Initialize()
  //  |        |
  //  |        V
  //  |   kInitializing
  //  |        | Decoders initialized
  //  |        |
  //  |        V            Decoders reset
  //  ---- kFlushed<------------------ kFlushing
  //           | StartPlayingFrom()         ^
  //           |                            |
  //           |                            | Flush()
  //           `---------> kPlaying--------'
  enum class RenderState {
    kUninitialized,
    kInitPendingCDM,  // Initialization is waiting for the CDM to be set.
    kInitializing,    // Initializing audio/video renderers.
    kFlushing,        // Flushing is in progress.
    kFlushed,         // After initialization or after flush completed.
    kPlaying,         // After StartPlayingFrom has been called.
    kError
  };

  RenderState render_state_ = RenderState::kUninitialized;

  MediaResource* media_resource_ = nullptr;
  RendererClient* client_ = nullptr;

  // TODO(servolk): Consider using DecoderFactory here instead of the
  // CreateVideoDecodersCB.
  CreateAudioDecodersCB create_audio_decoders_cb_;
  CreateVideoDecodersCB create_video_decoders_cb_;

  BufferingState buffering_state_ = BUFFERING_HAVE_NOTHING;

  // Playback operation callbacks.
  PipelineStatusCallback init_cb_;
  base::OnceClosure flush_cb_;
  TimeSource::WallClockTimeCB wall_clock_time_cb_;

  base::TimeDelta start_timestamp_ = base::TimeDelta::FromSeconds(0);

  PipelineStatistics stats_;

  // Indicates if a frame has been processed by CheckForMetadataChanges().
  bool have_renderered_frames_ = false;

  // Tracks last frame properties to detect and notify client of any changes.
  gfx::Size last_frame_natural_size_;
  bool last_frame_opaque_ = false;

  scoped_refptr<VideoFrame> last_frame_;
  scoped_refptr<MediaPlatformAPI> media_platform_api_;

  double playback_rate_ = 0.0;

  CdmContext* cdm_context_ = nullptr;

  bool mpa_initialized_ = false;
  // NOTE: Weak pointers must be invalidated before all other member variables.
  base::WeakPtrFactory<ExternalRenderer> weak_factory_;

  // Weak factory used to invalidate certain queued callbacks on reset().
  // This is useful when doing video frame copies asynchronously since we
  // want to discard video frames that might be received after the stream has
  // been reset.
  base::WeakPtrFactory<ExternalRenderer> frame_callback_weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ExternalRenderer);
};

}  // namespace media

#endif  // MEDIA_RENDERERS_NEVA_EXTERNAL_RENDERER_H_
