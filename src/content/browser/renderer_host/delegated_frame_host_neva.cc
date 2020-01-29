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

#include "content/browser/renderer_host/delegated_frame_host_neva.h"

#include "base/command_line.h"
#include "base/task/post_task.h"
#include "cc/base/switches.h"
#include "components/viz/service/frame_sinks/compositor_frame_sink_support.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"

namespace content {

namespace {

const int kBackgroundCleanupDelayMs = 1000;

}  // namespace

////////////////////////////////////////////////////////////////////////////////
// DelegatedFrameHost

DelegatedFrameHost::DelegatedFrameHost(const viz::FrameSinkId& frame_sink_id,
                                       DelegatedFrameHostClient* client,
                                       bool should_register_frame_sink_id)
    : neva_wrapped::DelegatedFrameHost(
          frame_sink_id,
          static_cast<neva_wrapped::DelegatedFrameHostClient*>(client),
          should_register_frame_sink_id),
      use_aggressive_release_policy_(
          base::CommandLine::ForCurrentProcess()->HasSwitch(
              cc::switches::kEnableAggressiveReleasePolicy)),
      weak_factory_(this) {}

DelegatedFrameHost::~DelegatedFrameHost() {}

void DelegatedFrameHost::WasShown(
    const viz::LocalSurfaceId& new_local_surface_id,
    const gfx::Size& new_dip_size,
    blink::mojom::RecordContentToVisibleTimeRequestPtr
        record_tab_switch_time_request) {
  // Calling origin procedure
  neva_wrapped::DelegatedFrameHost::WasShown(
      new_local_surface_id, new_dip_size,
      std::move(record_tab_switch_time_request));
  if (use_aggressive_release_policy_) {
    background_cleanup_task_.Cancel();
    if (compositor_)
      compositor_->ResumeDrawing();
  }

  DelegatedFrameHostClient* client =
      static_cast<DelegatedFrameHostClient*>(client_);
}

void DelegatedFrameHost::WasHidden(HiddenCause cause) {
  // Calling origin procedure
  neva_wrapped::DelegatedFrameHost::WasHidden(cause);

  if (use_aggressive_release_policy_) {
    if (compositor_)
      compositor_->SuspendDrawing();

    EvictDelegatedFrame();
    background_cleanup_task_.Reset(base::BindOnce(
        &DelegatedFrameHost::DoBackgroundCleanup, weak_factory_.GetWeakPtr()));
    base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
        FROM_HERE, background_cleanup_task_.callback(),
        base::TimeDelta::FromMilliseconds(kBackgroundCleanupDelayMs));
  }
}

void DelegatedFrameHost::DoBackgroundCleanup() {
  viz::FrameEvictionManager::GetInstance()->PurgeAllUnlockedFrames();
}

}  // namespace content
