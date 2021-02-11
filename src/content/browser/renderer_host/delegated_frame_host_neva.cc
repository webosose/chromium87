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
      weak_factory_(this) {}

DelegatedFrameHost::~DelegatedFrameHost() {}

void DelegatedFrameHost::ResumeDrawing() {
  background_cleanup_task_.Cancel();
  if (compositor_)
    compositor_->ResumeDrawing();
}

void DelegatedFrameHost::SuspendDrawing() {
  if (compositor_)
    compositor_->SuspendDrawing();

  EvictDelegatedFrame();
  background_cleanup_task_.Reset(base::BindOnce(
      &DelegatedFrameHost::DoBackgroundCleanup, weak_factory_.GetWeakPtr()));
  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE, background_cleanup_task_.callback(),
      base::TimeDelta::FromMilliseconds(kBackgroundCleanupDelayMs));
}

void DelegatedFrameHost::DoBackgroundCleanup() {
  viz::FrameEvictionManager::GetInstance()->PurgeAllUnlockedFrames();
}

}  // namespace content
