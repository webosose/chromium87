// Copyright 2019-2020 LG Electronics, Inc. All rights reserved.
// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// The implementation is based on RendererImpl in renderer_impl.cc

#include "media/renderers/neva/external_renderer.h"

#include <utility>

#include "base/bind.h"
#include "base/callback.h"
#include "base/callback_helpers.h"
#include "base/feature_list.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/string_util.h"
#include "base/time/default_tick_clock.h"
#include "base/trace_event/trace_event.h"
#include "media/base/bind_to_current_loop.h"
#include "media/base/limits.h"
#include "media/base/media_log.h"
#include "media/base/media_switches.h"
#include "media/base/null_video_sink.h"
#include "media/base/pipeline_status.h"
#include "media/base/renderer_client.h"
#include "media/base/video_frame.h"

namespace media {

namespace {

bool ShouldUseLowDelayMode(DemuxerStream* stream) {
  return base::FeatureList::IsEnabled(kLowDelayVideoRenderingOnLiveStream) &&
         stream->liveness() == DemuxerStream::LIVENESS_LIVE;
}

}  // namespace

ExternalRenderer::ExternalRenderer(
    const scoped_refptr<base::SingleThreadTaskRunner>& media_task_runner,
    const scoped_refptr<MediaPlatformAPI>& media_platform_api,
    const CreateAudioDecodersCB& create_audio_decoders_cb,
    const CreateVideoDecodersCB& create_video_decoders_cb,
    MediaLog* media_log,
    std::unique_ptr<GpuMemoryBufferVideoFramePool> gmb_pool)
    : task_runner_(media_task_runner),
      gpu_memory_buffer_pool_(std::move(gmb_pool)),
      media_log_(media_log),
      create_audio_decoders_cb_(create_audio_decoders_cb),
      create_video_decoders_cb_(create_video_decoders_cb),
      media_platform_api_(media_platform_api),
      weak_factory_(this),
      frame_callback_weak_factory_(this) {
  DCHECK(create_video_decoders_cb_);
  VLOG(1) << __func__;

  video_sink_.reset(new NullVideoSink(
      false, base::TimeDelta::FromSecondsD(1.0 / 30),
      base::Bind(&ExternalRenderer::FrameReady, base::Unretained(this)),
      task_runner_));
}

ExternalRenderer::~ExternalRenderer() {
  DCHECK(task_runner_->BelongsToCurrentThread());
  VLOG(1) << __func__;

  if (!init_cb_.is_null())
    std::move(init_cb_).Run(PIPELINE_ERROR_ABORT);

  if (!flush_cb_.is_null())
    std::move(flush_cb_).Run();

  if (sink_started_)
    StopSink();

  // This code is meaningful only when we use MojoMediaPlatformAPI.
  // MojoMediaPlatformAPI is made as RefCountedThreadSafe. So renderer main
  // thread, which runs WebMediaPlayerMSE(that created MojoMediaPlatformAPI)
  // is the last one which references MojoMediaPlatformAPI and calls destructor
  // of MojoMediaPlatformAPI, which will disconnect the mojo IPC with
  // MediaPlatformAPI. It can break thread safe, because MojoMediaPlatformAPI is
  // initialized by ExternalRenderer where uses renderer media thread.
  // This code explicitly calls media_platform_api_.reset() from here, to
  // guarantee MediaPlatformAPI will be created and destoryed by same
  // thread(renderer media thread).
  media_platform_api_->FinalizeMediaPlatformAPI();
}

void ExternalRenderer::Initialize(MediaResource* media_resource,
                                  RendererClient* client,
                                  PipelineStatusCallback init_cb) {
  VLOG(1) << __func__;
  DCHECK(task_runner_->BelongsToCurrentThread());
  DCHECK_EQ(render_state_, RenderState::kUninitialized);
  DCHECK(init_cb);
  DCHECK(client);
  TRACE_EVENT_ASYNC_BEGIN0("media", "ExternalRenderer::Initialize", this);

  client_ = client;
  media_resource_ = media_resource;
  init_cb_ = std::move(init_cb);

  SetMediaPlatformAPICb();

  if (HasEncryptedStream() && !cdm_context_) {
    LOG(INFO) << __func__ << ": Has encrypted stream but CDM is not set.";
    render_state_ = RenderState::kInitPendingCDM;
    return;
  }

  render_state_ = RenderState::kInitializing;
  InitializeMediaPlatformAPI();
}

void ExternalRenderer::SetCdm(CdmContext* cdm_context,
                              CdmAttachedCB cdm_attached_cb) {
  DVLOG(1) << __func__;
  DCHECK(task_runner_->BelongsToCurrentThread());
  DCHECK(cdm_context);
  TRACE_EVENT0("media", "ExternalRenderer::SetCdm");

  if (cdm_context_) {
    DVLOG(1) << "Switching CDM not supported.";
    std::move(cdm_attached_cb).Run(false);
    return;
  }

  cdm_context_ = cdm_context;
  std::move(cdm_attached_cb).Run(true);

  if (cdm_context_)
    media_platform_api_->SetKeySystem(cdm_context_->get_key_system());

  if (render_state_ != RenderState::kInitPendingCDM)
    return;

  DCHECK(init_cb_);
  render_state_ = RenderState::kInitializing;
  InitializeMediaPlatformAPI();
}

void ExternalRenderer::SetLatencyHint(
    base::Optional<base::TimeDelta> latency_hint) {
  NOTIMPLEMENTED_LOG_ONCE();
}

void ExternalRenderer::Flush(base::OnceClosure flush_cb) {
  DVLOG(1) << __func__;
  DCHECK(task_runner_->BelongsToCurrentThread());

  TRACE_EVENT_ASYNC_BEGIN0("media", "ExternalRenderer::Flush", this);

  if (render_state_ == RenderState::kFlushed) {
    flush_cb_ = BindToCurrentLoop(std::move(flush_cb));
    FinishFlush();
    return;
  }

  if (render_state_ != RenderState::kPlaying) {
    DCHECK_EQ(render_state_, RenderState::kError);
    return;
  }

  if (sink_started_)
    StopSink();

  flush_cb_ = std::move(flush_cb);
  render_state_ = RenderState::kFlushing;

  ready_frames_.clear();
  last_frame_ = nullptr;

  if (buffering_state_ != BUFFERING_HAVE_NOTHING) {
    buffering_state_ = BUFFERING_HAVE_NOTHING;
    task_runner_->PostTask(
        FROM_HERE, base::Bind(&ExternalRenderer::OnBufferingStateChange,
                              weak_factory_.GetWeakPtr(), buffering_state_,
                              BUFFERING_CHANGE_REASON_UNKNOWN));
  }
  audio_received_end_of_stream_ = false;
  video_received_end_of_stream_ = false;
  rendered_end_of_stream_ = false;

  FlushAudioDecoder();
}

void ExternalRenderer::StartPlayingFrom(base::TimeDelta timestamp) {
  DVLOG(1) << __func__ << "(" << timestamp.InMicroseconds() << ")";
  DCHECK(task_runner_->BelongsToCurrentThread());
  DCHECK(ready_frames_.empty());
  DCHECK_EQ(buffering_state_, BUFFERING_HAVE_NOTHING);

  // kFlushed state means it has flushed pipeline by seek operation
  // so it needs to let platform's pipelie know the starting timestamp to play
  // It is also kFlushed state when it is first playing but this case we don't
  // need to call seek api. To know this situation start_timestamp_ is set 0 by
  // default value.
  if (render_state_ == RenderState::kFlushed && start_timestamp_ != timestamp) {
    media_platform_api_->Seek(timestamp);
  }

  render_state_ = RenderState::kPlaying;
  start_timestamp_ = timestamp;

  // TODO(neva): We need to put StartSink to right place because
  // StartPlayingFrom is not video playing but prepareing some frames
  StartSink();
  if (video_decoder_stream_)
    video_decoder_stream_->SkipPrepareUntil(start_timestamp_);
  AttemptRead();
}

void ExternalRenderer::SetPlaybackRate(double playback_rate) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  playback_rate_ = playback_rate;

  // If state is BUFFERING_HAVE_ENOUGH, then we already started to play
  // so we change change playback_rate. if state is BUFFERING_HAVE_NOTHING,
  // we need to wait for BUFFERING_HAVE_ENOUGH signal to start playing.
  if (buffering_state_ == BUFFERING_HAVE_ENOUGH)
    media_platform_api_->SetPlaybackRate(playback_rate_);
}

void ExternalRenderer::SetVolume(float volume) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  media_platform_api_->SetPlaybackVolume(volume);
}

base::TimeDelta ExternalRenderer::GetMediaTime() {
  // No BelongsToCurrentThread() checking because this can be called from other
  // threads.
  return media_platform_api_->GetCurrentTime();
}

void ExternalRenderer::OnSelectedVideoTracksChanged(
    const std::vector<DemuxerStream*>& enabled_tracks,
    base::OnceClosure change_completed_cb) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  NOTIMPLEMENTED();
}

void ExternalRenderer::OnEnabledAudioTracksChanged(
    const std::vector<DemuxerStream*>& enabled_tracks,
    base::OnceClosure change_completed_cb) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  NOTIMPLEMENTED();
}

scoped_refptr<VideoFrame> ExternalRenderer::Render(base::TimeTicks deadline_min,
                                                   base::TimeTicks deadline_max,
                                                   bool background_rendering) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  TRACE_EVENT1("media", "ExternalRenderer::Render", "id", media_log_->id());

  if (render_state_ != RenderState::kPlaying)
    return nullptr;

  // TODO(neva): We need to consider calling FireEndedCallback from the callback
  // from
  // media_platform_api eos received because real rendering is done by platform
  // media
  // and the eos signal comes from there.
  MaybeFireEndedCallback();

  UpdateStats();

  if (!ready_frames_.empty()) {
    scoped_refptr<VideoFrame> result = ready_frames_.front();
    last_frame_ = result;
    ready_frames_.pop_front();

    // Always post this task, it will acquire new frames if necessary and since
    // it happens on another thread, even if we don't have room in the queue
    // now, by the time it runs (may be delayed up to 50ms for complex decodes!)
    // we might.
    task_runner_->PostTask(
        FROM_HERE,
        base::Bind(&ExternalRenderer::AttemptReadAndCheckForMetadataChanges,
                   weak_factory_.GetWeakPtr(), result->format(),
                   result->natural_size()));
    return result;
  }

  AttemptRead();
  return last_frame_;
}

void ExternalRenderer::OnFrameDropped() {
  DCHECK(task_runner_->BelongsToCurrentThread());
}

base::TimeDelta ExternalRenderer::GetPreferredRenderInterval() {
  DCHECK(task_runner_->BelongsToCurrentThread());
  return base::TimeDelta::FromMicroseconds(16666);
}

void ExternalRenderer::InitializeMediaPlatformAPI() {
  DCHECK(task_runner_->BelongsToCurrentThread());
  DCHECK(media_platform_api_);

  if (mpa_initialized_) {
    OnMediaPlatformAPIInitialized(PIPELINE_OK);
    return;
  }

  VideoDecoderConfig video_config;
  AudioDecoderConfig audio_config;

  DemuxerStream* video_stream =
      media_resource_->GetFirstStream(DemuxerStream::VIDEO);
  DemuxerStream* audio_stream =
      media_resource_->GetFirstStream(DemuxerStream::AUDIO);

  if (video_stream)
    video_config = video_stream->video_decoder_config();
  if (audio_stream)
    audio_config = audio_stream->audio_decoder_config();

  media_platform_api_->Initialize(
      audio_config, video_config,
      base::Bind(&ExternalRenderer::OnMediaPlatformAPIInitialized,
                 frame_callback_weak_factory_.GetWeakPtr()));
}

void ExternalRenderer::InitializeAudioDecoder() {
  DCHECK(task_runner_->BelongsToCurrentThread());

  DemuxerStream* audio_stream =
      media_resource_->GetFirstStream(DemuxerStream::AUDIO);

  if (audio_stream) {
    audio_decoder_stream_ = std::make_unique<AudioDecoderStream>(
        std::make_unique<AudioDecoderStream::StreamTraits>(media_log_,
                                                           CHANNEL_LAYOUT_NONE),
        task_runner_, create_audio_decoders_cb_, media_log_);
    audio_decoder_stream_->set_config_change_observer(base::Bind(
        &ExternalRenderer::OnAudioConfigChange, weak_factory_.GetWeakPtr()));

    current_audio_decoder_config_ = audio_stream->audio_decoder_config();
    DCHECK(current_audio_decoder_config_.IsValidConfig());

    audio_decoder_stream_->Initialize(
        audio_stream,
        base::Bind(&ExternalRenderer::OnAudioDecoderStreamInitialized,
                   weak_factory_.GetWeakPtr()),
        cdm_context_,
        base::Bind(&ExternalRenderer::OnStatisticsUpdate,
                   weak_factory_.GetWeakPtr()),
        base::Bind(&ExternalRenderer::OnWaiting, weak_factory_.GetWeakPtr()));
    has_audio_ = true;
  } else {
    OnAudioDecoderStreamInitialized(true);
  }
}

void ExternalRenderer::InitializeVideoDecoder() {
  DemuxerStream* video_stream =
      media_resource_->GetFirstStream(DemuxerStream::VIDEO);

  if (video_stream) {
    video_decoder_stream_.reset(new VideoDecoderStream(
        std::make_unique<VideoDecoderStream::StreamTraits>(media_log_),
        task_runner_, create_video_decoders_cb_, media_log_));
    video_decoder_stream_->set_config_change_observer(base::Bind(
        &ExternalRenderer::OnVideoConfigChange, weak_factory_.GetWeakPtr()));
    if (gpu_memory_buffer_pool_) {
      video_decoder_stream_->SetPrepareCB(base::BindRepeating(
          &GpuMemoryBufferVideoFramePool::MaybeCreateHardwareFrame,
          // Safe since VideoFrameStream won't issue calls after destruction.
          base::Unretained(gpu_memory_buffer_pool_.get())));
    }

    low_delay_ = ShouldUseLowDelayMode(video_stream);

    current_video_decoder_config_ = video_stream->video_decoder_config();
    DCHECK(current_video_decoder_config_.IsValidConfig());

    video_decoder_stream_->Initialize(
        video_stream,
        base::Bind(&ExternalRenderer::OnVideoDecoderStreamInitialized,
                   weak_factory_.GetWeakPtr()),
        cdm_context_,
        base::Bind(&ExternalRenderer::OnStatisticsUpdate,
                   weak_factory_.GetWeakPtr()),
        base::Bind(&ExternalRenderer::OnWaiting, weak_factory_.GetWeakPtr()));
    has_video_ = true;
  } else {
    OnVideoDecoderStreamInitialized(true);
  }

  UMA_HISTOGRAM_BOOLEAN("Media.VideoRenderer.LowDelay", low_delay_);

  if (low_delay_)
    MEDIA_LOG(DEBUG, media_log_) << "Video rendering in low delay mode.";
}

void ExternalRenderer::OnMediaPlatformAPIInitialized(PipelineStatus status) {
  if (status != PIPELINE_OK)
    return std::move(init_cb_).Run(status);
  mpa_initialized_ = true;
  InitializeAudioDecoder();
}

void ExternalRenderer::OnAudioDecoderStreamInitialized(bool success) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  DCHECK_EQ(render_state_, RenderState::kInitializing);

  if (!success) {
    FinishInitialization(DECODER_ERROR_NOT_SUPPORTED);
    return;
  }

  InitializeVideoDecoder();
}

void ExternalRenderer::OnVideoDecoderStreamInitialized(bool success) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  DCHECK_EQ(render_state_, RenderState::kInitializing);

  if (!success) {
    FinishInitialization(DECODER_ERROR_NOT_SUPPORTED);
    return;
  }

  FinishInitialization(PIPELINE_OK);
}

void ExternalRenderer::OnPlaybackError(PipelineStatus error) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  if (client_)
    client_->OnError(error);
}

void ExternalRenderer::OnPlaybackEnded() {
  DCHECK(task_runner_->BelongsToCurrentThread());
  if (client_)
    client_->OnEnded();
}

void ExternalRenderer::OnStatisticsUpdate(const PipelineStatistics& stats) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  if (client_)
    client_->OnStatisticsUpdate(stats);
}

void ExternalRenderer::OnBufferingStateChange(
    BufferingState state,
    BufferingStateChangeReason reason) {
  DCHECK(task_runner_->BelongsToCurrentThread());

  // "Underflow" is only possible when playing. This avoids noise like blaming
  // the decoder for an "underflow" that is really just a seek.
  if (render_state_ == RenderState::kPlaying &&
      state == BUFFERING_HAVE_NOTHING) {
    // TODO(neva, sync-to-87): Need to check.
    if ((video_decoder_stream_ &&
         video_decoder_stream_->is_demuxer_read_pending()) ||
        (audio_decoder_stream_ &&
         audio_decoder_stream_->is_demuxer_read_pending())) {
      reason = DEMUXER_UNDERFLOW;
    }
  }

  if (state == BUFFERING_HAVE_ENOUGH) {
    // Renderer prerolled.
    StartSink();
    media_platform_api_->SetPlaybackRate(playback_rate_);
  } else {
    // Renderer underflowed.
    StopSink();
    media_platform_api_->SetPlaybackRate(0.0f);
  }

  if (client_)
    client_->OnBufferingStateChange(state, reason);
}

void ExternalRenderer::OnWaiting(WaitingReason reason) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  DVLOG(1) << __func__;
  if (client_)
    client_->OnWaiting(reason);
}

void ExternalRenderer::OnAudioConfigChange(const AudioDecoderConfig& config) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  DCHECK(config.IsValidConfig());
  DVLOG(1) << __func__;
  if (client_)
    client_->OnAudioConfigChange(config);
}

void ExternalRenderer::OnVideoConfigChange(const VideoDecoderConfig& config) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  DCHECK(config.IsValidConfig());

  DVLOG(1) << __func__;
  // RendererClient only cares to know about config changes that differ from
  // previous configs.
  if (!current_video_decoder_config_.Matches(config)) {
    current_video_decoder_config_ = config;
    if (client_)
      client_->OnVideoConfigChange(config);
  }
}

void ExternalRenderer::AudioBufferReady(
    AudioDecoderStream::ReadStatus status,
    const scoped_refptr<AudioBuffer> frame) {
  DCHECK(task_runner_->BelongsToCurrentThread());

  if (status == AudioDecoderStream::ABORTED ||
      status == AudioDecoderStream::DEMUXER_READ_ABORTED ||
      render_state_ == RenderState::kFlushing ||
      render_state_ == RenderState::kFlushed) {
    return;
  }

  if (status == AudioDecoderStream::DECODE_ERROR) {
    DCHECK(!frame);
    task_runner_->PostTask(
        FROM_HERE,
        base::Bind(&ExternalRenderer::OnPlaybackError,
                   weak_factory_.GetWeakPtr(), PIPELINE_ERROR_DECODE));
    return;
  }

  // Can happen when demuxers are preparing for a new Seek().
  if (!frame) {
    DCHECK_EQ(status, AudioDecoderStream::DEMUXER_READ_ABORTED);
    return;
  }

  if (frame->end_of_stream()) {
    DCHECK(!audio_received_end_of_stream_);
    audio_received_end_of_stream_ = true;
  }

  // Signal buffering state if we've met our conditions.
  if (buffering_state_ == BUFFERING_HAVE_NOTHING && HaveEnoughData())
    TransitionToHaveEnough();

  AttemptRead();
}

void ExternalRenderer::VideoFrameReady(VideoDecoderStream::ReadStatus status,
                                       const scoped_refptr<VideoFrame> frame) {
  DCHECK(task_runner_->BelongsToCurrentThread());

  if (status == VideoDecoderStream::ABORTED ||
      status == VideoDecoderStream::DEMUXER_READ_ABORTED ||
      render_state_ == RenderState::kFlushing ||
      render_state_ == RenderState::kFlushed) {
    return;
  }

  if (status == VideoDecoderStream::DECODE_ERROR) {
    DCHECK(!frame);
    task_runner_->PostTask(
        FROM_HERE,
        base::Bind(&ExternalRenderer::OnPlaybackError,
                   weak_factory_.GetWeakPtr(), PIPELINE_ERROR_DECODE));
    return;
  }

  const bool is_eos = frame->metadata()->end_of_stream;
  const bool is_before_start_time =
      !is_eos && IsBeforeStartTime(frame->timestamp());
  const bool cant_read = !video_decoder_stream_->CanReadWithoutStalling();

  if (is_eos) {
    DCHECK(!video_received_end_of_stream_);
    video_received_end_of_stream_ = true;
  } else if ((low_delay_ || cant_read) && is_before_start_time) {
    stats_.video_frames_decoded++;
    // Don't accumulate frames that are earlier than the start time if we
    // won't have a chance for a better frame, otherwise we could declare
    // HAVE_ENOUGH_DATA and start playback prematurely.
    AttemptRead();
    return;
  } else {
    // Provide frame duration information so that even if we only have one frame
    // in the queue we can properly estimate duration. This allows the call to
    // RemoveFramesForUnderflowOrBackgroundRendering() below to actually expire
    // this frame if it's too far behind the current media time. Without this,
    // we may resume too soon after a track change in the low delay case.
    if (!frame->metadata()->frame_duration) {
      frame->metadata()->frame_duration =
          video_decoder_stream_->AverageDuration();
    }

    AddReadyFrame(frame);
  }
  stats_.video_frames_decoded++;

  // We may have removed all frames above and have reached end of stream.
  MaybeFireEndedCallback();

  // Update any statistics since the last call.
  UpdateStats();

  // Signal buffering state if we've met our conditions.
  if (buffering_state_ == BUFFERING_HAVE_NOTHING && HaveEnoughData())
    TransitionToHaveEnough();

  // Always request more decoded video if we have capacity.
  AttemptRead();
}

void ExternalRenderer::AddReadyFrame(const scoped_refptr<VideoFrame> frame) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  DCHECK(!frame->metadata()->end_of_stream);

  if (last_frame_natural_size_ != frame->natural_size())
    ready_frames_.push_back(frame);
}

void ExternalRenderer::AttemptRead() {
  DCHECK(task_runner_->BelongsToCurrentThread());

  if (ReceivedEOSByType(ALL))
    return;

  switch (render_state_) {
    case RenderState::kPlaying:
      if (has_audio_ && !ReceivedEOSByType(Audio) &&
          media_platform_api_->AllowedFeedAudio()) {
        audio_decoder_stream_->Read(
            base::BindOnce(&ExternalRenderer::AudioBufferReady,
                           frame_callback_weak_factory_.GetWeakPtr()));
      }
      if (has_video_ && !ReceivedEOSByType(Video) &&
          media_platform_api_->AllowedFeedVideo()) {
        video_decoder_stream_->Read(
            base::BindOnce(&ExternalRenderer::VideoFrameReady,
                           frame_callback_weak_factory_.GetWeakPtr()));
      }
      return;
    case RenderState::kUninitialized:
    case RenderState::kInitPendingCDM:
    case RenderState::kInitializing:
    case RenderState::kFlushing:
    case RenderState::kFlushed:
    case RenderState::kError:
      return;
  }
}

void ExternalRenderer::FlushAudioDecoder() {
  if (has_audio_) {
    audio_decoder_stream_->Reset(
        base::Bind(&ExternalRenderer::OnAudioDecoderStreamResetDone,
                   weak_factory_.GetWeakPtr()));
  } else {
    OnAudioDecoderStreamResetDone();
  }
}

void ExternalRenderer::FlushVideoDecoder() {
  if (has_video_) {
    video_decoder_stream_->Reset(
        base::Bind(&ExternalRenderer::OnVideoDecoderStreamResetDone,
                   weak_factory_.GetWeakPtr()));
  } else {
    OnVideoDecoderStreamResetDone();
  }
}

void ExternalRenderer::FinishFlush() {
  DCHECK(task_runner_->BelongsToCurrentThread());
  // Drop any pending calls to FrameReady() and
  // FrameReadyForCopyingToGpuMemoryBuffers()
  frame_callback_weak_factory_.InvalidateWeakPtrs();
  render_state_ = RenderState::kFlushed;
  start_timestamp_ = kNoTimestamp;
  TRACE_EVENT_ASYNC_END0("media", "ExternalRenderer::Flush", this);
  std::move(flush_cb_).Run();
}

void ExternalRenderer::OnAudioDecoderStreamResetDone() {
  DCHECK(task_runner_->BelongsToCurrentThread());
  DCHECK(!sink_started_);
  DCHECK_EQ(RenderState::kFlushing, render_state_);
  DCHECK(!audio_received_end_of_stream_);
  DCHECK(!rendered_end_of_stream_);
  DCHECK_EQ(buffering_state_, BUFFERING_HAVE_NOTHING);

  FlushVideoDecoder();
}

void ExternalRenderer::OnVideoDecoderStreamResetDone() {
  DCHECK(task_runner_->BelongsToCurrentThread());
  DCHECK(!sink_started_);
  DCHECK_EQ(RenderState::kFlushing, render_state_);
  DCHECK(!video_received_end_of_stream_);
  DCHECK_EQ(buffering_state_, BUFFERING_HAVE_NOTHING);

  FinishFlush();
}

bool ExternalRenderer::HaveEnoughData() const {
  DCHECK(task_runner_->BelongsToCurrentThread());
  DCHECK_EQ(render_state_, RenderState::kPlaying);

  if (ReceivedEOSByType(ALL))
    return true;

  return media_platform_api_->HaveEnoughData();
}

void ExternalRenderer::TransitionToHaveEnough() {
  DCHECK(task_runner_->BelongsToCurrentThread());
  DCHECK_EQ(buffering_state_, BUFFERING_HAVE_NOTHING);

  buffering_state_ = BUFFERING_HAVE_ENOUGH;
  task_runner_->PostTask(
      FROM_HERE, base::Bind(&ExternalRenderer::OnBufferingStateChange,
                            weak_factory_.GetWeakPtr(), buffering_state_,
                            BUFFERING_CHANGE_REASON_UNKNOWN));
}

void ExternalRenderer::TransitionToHaveNothing() {
  DCHECK(task_runner_->BelongsToCurrentThread());

  if (buffering_state_ != BUFFERING_HAVE_ENOUGH || HaveEnoughData())
    return;

  buffering_state_ = BUFFERING_HAVE_NOTHING;
  task_runner_->PostTask(
      FROM_HERE, base::Bind(&ExternalRenderer::OnBufferingStateChange,
                            weak_factory_.GetWeakPtr(), buffering_state_,
                            DECODER_UNDERFLOW));
}

void ExternalRenderer::UpdateStats() {
  DCHECK(task_runner_->BelongsToCurrentThread());

  // No need to check for `stats_.video_frames_decoded_power_efficient` because
  // if it is greater than 0, `stats_.video_frames_decoded` will too.
  if (!stats_.video_frames_decoded)
    return;

  if (stats_.video_frames_dropped) {
    TRACE_EVENT_INSTANT2("media", "VideoFramesDropped",
                         TRACE_EVENT_SCOPE_THREAD, "count",
                         stats_.video_frames_dropped, "id", media_log_->id());
  }

  OnStatisticsUpdate(stats_);

  stats_.video_frames_decoded = 0;
  stats_.video_frames_decoded_power_efficient = 0;
}

void ExternalRenderer::StartSink() {
  DCHECK(task_runner_->BelongsToCurrentThread());
  if (sink_started_)
    return;
  DVLOG(1) << __func__;
  sink_started_ = true;
  video_sink_->Start(this);
}

void ExternalRenderer::StopSink() {
  DCHECK(task_runner_->BelongsToCurrentThread());
  if (!sink_started_)
    return;
  DVLOG(1) << __func__;
  video_sink_->Stop();
  sink_started_ = false;
}

void ExternalRenderer::MaybeFireEndedCallback() {
  DCHECK(task_runner_->BelongsToCurrentThread());

  // If there's only one frame in the video or Render() was never called, the
  // algorithm will have one frame linger indefinitely.  So in cases where the
  // frame duration is unknown and we've received EOS, fire it once we get down
  // to a single frame.

  // Don't fire ended if we haven't received EOS or have already done so.
  if (!(ReceivedEOSByType(ALL)) || rendered_end_of_stream_)
    return;

  // Fire ended if we have no more effective frames or only ever had one frame.
  if (ReceivedEndOfStream() && !rendered_end_of_stream_) {
    rendered_end_of_stream_ = true;
    task_runner_->PostTask(FROM_HERE,
                           base::Bind(&ExternalRenderer::OnPlaybackEnded,
                                      weak_factory_.GetWeakPtr()));
  }
}

base::TimeTicks ExternalRenderer::ConvertMediaTimestamp(
    base::TimeDelta media_time) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  std::vector<base::TimeDelta> media_times(1, media_time);
  std::vector<base::TimeTicks> wall_clock_times;
  if (!wall_clock_time_cb_.Run(media_times, &wall_clock_times))
    return base::TimeTicks();
  return wall_clock_times[0];
}

base::TimeTicks ExternalRenderer::GetCurrentMediaTimeAsWallClockTime() {
  DCHECK(task_runner_->BelongsToCurrentThread());
  std::vector<base::TimeTicks> current_time;
  wall_clock_time_cb_.Run(std::vector<base::TimeDelta>(), &current_time);
  return current_time[0];
}

bool ExternalRenderer::IsBeforeStartTime(base::TimeDelta timestamp) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  return false;
}

void ExternalRenderer::CheckForMetadataChanges(VideoPixelFormat pixel_format,
                                               const gfx::Size& natural_size) {
  DCHECK(task_runner_->BelongsToCurrentThread());

  // Notify client of size and opacity changes if this is the first frame
  // or if those have changed from the last frame.
  if (!have_renderered_frames_ || last_frame_natural_size_ != natural_size) {
    last_frame_natural_size_ = natural_size;
    if (client_)
      client_->OnVideoNaturalSizeChange(last_frame_natural_size_);
  }

  const bool is_opaque = IsOpaque(pixel_format);
  if (!have_renderered_frames_ || last_frame_opaque_ != is_opaque) {
    last_frame_opaque_ = is_opaque;
    if (client_)
      client_->OnVideoOpacityChange(last_frame_opaque_);
  }

  have_renderered_frames_ = true;
}

void ExternalRenderer::AttemptReadAndCheckForMetadataChanges(
    VideoPixelFormat pixel_format,
    const gfx::Size& natural_size) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  CheckForMetadataChanges(pixel_format, natural_size);
  AttemptRead();
}

// Returns true if Platform player played to the end
bool ExternalRenderer::ReceivedEndOfStream() {
  DCHECK(task_runner_->BelongsToCurrentThread());
  return media_platform_api_->IsEOSReceived();
}

// Returns true if one has eos buffer received from the decoder
bool ExternalRenderer::ReceivedEOSByType(Type type) const {
  DCHECK(task_runner_->BelongsToCurrentThread());
  switch (type) {
    case Audio:
      return has_audio_ ? audio_received_end_of_stream_ : true;
    case Video:
      return has_video_ ? video_received_end_of_stream_ : true;
    case ALL:
      return ReceivedEOSByType(Audio) && ReceivedEOSByType(Video);
  }
  return false;
}

void ExternalRenderer::SetMediaPlatformAPICb() {
  DCHECK(task_runner_->BelongsToCurrentThread());
  if (media_platform_api_) {
    media_platform_api_->SetPlayerEventCb(BindToCurrentLoop(base::Bind(
        &ExternalRenderer::OnPlayerEvent, weak_factory_.GetWeakPtr())));

    media_platform_api_->SetStatisticsCb(base::Bind(
        &ExternalRenderer::OnStatisticsUpdate, weak_factory_.GetWeakPtr()));
  }
}

void ExternalRenderer::OnPlayerEvent(PlayerEvent event) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  // Declare HAVE_NOTHING if we reach a state where we can't progress playback
  // any further.  We don't want to do this if we've already done so, reached
  // end of stream, or have frames available.  We also don't want to do this in
  // background rendering mode, as the frames aren't visible anyways.
  if (buffering_state_ == BUFFERING_HAVE_ENOUGH && !ReceivedEOSByType(ALL) &&
      event == PlayerEvent::kBufferLow)
    TransitionToHaveNothing();

  if (buffering_state_ == BUFFERING_HAVE_NOTHING &&
      (event == PlayerEvent::kLoadCompleted ||
       event == PlayerEvent::kSeekDone) &&
      HaveEnoughData())
    TransitionToHaveEnough();
}

void ExternalRenderer::FinishInitialization(PipelineStatus status) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  if (init_cb_.is_null())
    return;

  TRACE_EVENT_ASYNC_END1("media", "::Initialize", this, "status",
                         PipelineStatusToString(status));
  render_state_ = (status == PIPELINE_OK) ? RenderState::kFlushed
                                          : RenderState::kUninitialized;
  // We're all good!  Consider ourselves flushed. (ThreadMain() should never
  // see us in the kUninitialized state).
  // Since we had an initial Preroll(), we consider ourself flushed, because we
  // have not populated any buffers yet.
  std::move(init_cb_).Run(status);
}

bool ExternalRenderer::HasEncryptedStream() {
  DCHECK(task_runner_->BelongsToCurrentThread());
  std::vector<DemuxerStream*> demuxer_streams =
      media_resource_->GetAllStreams();

  for (auto* stream : demuxer_streams) {
    if (stream->type() == DemuxerStream::AUDIO &&
        stream->audio_decoder_config().is_encrypted())
      return true;
    if (stream->type() == DemuxerStream::VIDEO &&
        stream->video_decoder_config().is_encrypted())
      return true;
  }

  return false;
}
}  // namespace media
