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
#include "content/browser/renderer_host/delegated_frame_host_client_neva.h"
#include "content/browser/renderer_host/render_widget_host_view_aura.h"

namespace content {

DelegatedFrameHostClientNeva::DelegatedFrameHostClientNeva(
    RenderWidgetHostViewAura* render_widget_host_view)
    : DelegatedFrameHostClientAura(render_widget_host_view) {}

DelegatedFrameHostClientNeva::~DelegatedFrameHostClientNeva() = default;

bool DelegatedFrameHostClientNeva::DelegatedFrameHostIsKeepAliveWebApp() const {
  return const_cast<DelegatedFrameHostClientNeva*>(this)
      ->render_widget_host_view()
      ->IsKeepAliveWebApp();
}

}  // namespace content
