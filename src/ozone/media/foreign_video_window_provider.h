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

#ifndef OZONE_MEDIA_FOREIGN_VIDEO_WINDOW_PROVIDER_H_
#define OZONE_MEDIA_FOREIGN_VIDEO_WINDOW_PROVIDER_H_

#include <wayland-client.h>
#include <wayland-webos-foreign-client-protocol.h>

#include <map>
#include <string>

#include "base/cancelable_callback.h"
#include "base/single_thread_task_runner.h"
#include "base/unguessable_token.h"
#include "ozone/media/video_window_provider.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/views/widget/desktop_aura/neva/ui_constants.h"

namespace ui {

class ForeignVideoWindow;

class ForeignVideoWindowProvider : public VideoWindowProvider {
 public:
  ForeignVideoWindowProvider();
  ~ForeignVideoWindowProvider() override;

  static void HandleExportedWindowAssigned(
      void* data,
      struct wl_webos_exported* webos_exported,
      const char* window_id,
      uint32_t exported_type);
  void OnCreatedForeignWindow(struct wl_webos_exported* webos_exported,
                              const std::string native_window_id,
                              ui::ForeignWindowType type);

  base::UnguessableToken CreateNativeVideoWindow(
      gfx::AcceleratedWidget w,
      mojo::PendingRemote<ui::mojom::VideoWindowClient> client,
      mojo::PendingReceiver<ui::mojom::VideoWindow> receiver,
      const VideoWindowParams& params,
      WindowEventCb cb) override;

  void DestroyNativeVideoWindow(
      gfx::AcceleratedWidget w,
      const base::UnguessableToken& window_id) override;

  void NativeVideoWindowGeometryChanged(
      const base::UnguessableToken& window_id,
      const gfx::Rect& dst,
      const gfx::Rect& src = gfx::Rect(),
      const base::Optional<gfx::Rect>& ori = base::nullopt) override;
  void NativeVideoWindowVisibilityChanged(
      const base::UnguessableToken& window_id,
      bool visibility) override;
  void OwnerWidgetStateChanged(const base::UnguessableToken& window_id,
                               ui::WidgetState state) override;

 private:
  friend class ForeignVideoWindow;
  void UpdateNativeVideoWindowGeometry(const base::UnguessableToken& window_id);
  void NativeVideoWindowSetProperty(const base::UnguessableToken& window_id,
                                    const std::string& name,
                                    const std::string& value);
  ForeignVideoWindow* FindWindow(struct wl_webos_exported* webos_exported);
  ForeignVideoWindow* FindWindow(const base::UnguessableToken& id);

  std::map<base::UnguessableToken, std::unique_ptr<ForeignVideoWindow>>
      foreign_windows_;
  std::map<std::string, base::UnguessableToken> native_id_to_window_id_;
  const scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
};

}  // namespace ui

#endif  // OZONE_MEDIA_FOREIGN_VIDEO_WINDOW_PROVIDER_H_
