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
#ifndef OZONE_MEDIA_PSEUDO_VIDEO_WINDOW_PROVIDER_H_
#define OZONE_MEDIA_PSEUDO_VIDEO_WINDOW_PROVIDER_H_

#include <map>
#include <string>

#include "base/cancelable_callback.h"
#include "base/unguessable_token.h"
#include "ozone/media/video_window_provider.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/platform_window/neva/mojom/video_window_controller.mojom.h"

namespace ui {
class PseudoVideoWindow;

class PseudoVideoWindowProvider : public VideoWindowProvider {
 public:
  PseudoVideoWindowProvider();
  ~PseudoVideoWindowProvider() override;
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
  friend class PseudoVideoWindow;
  void UpdateNativeVideoWindowGeometry(const base::UnguessableToken& window_id);
  PseudoVideoWindow* FindWindow(const base::UnguessableToken& window_id);

  std::map<base::UnguessableToken, std::unique_ptr<PseudoVideoWindow>>
      pseudo_windows_;
};

}  // namespace ui
#endif  // OZONE_MEDIA_PSEUDO_VIDEO_WINDOW_PROVIDER_H_
