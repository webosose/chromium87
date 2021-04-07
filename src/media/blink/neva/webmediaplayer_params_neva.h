// Copyright 2018-2020 LG Electronics, Inc.
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

#ifndef MEDIA_BLINK_NEVA_WEBMEDIAPLAYER_PARAMS_NEVA_H_
#define MEDIA_BLINK_NEVA_WEBMEDIAPLAYER_PARAMS_NEVA_H_

#include "media/blink/media_blink_export.h"
#include "media/neva/media_platform_api.h"
#include "media/neva/media_player_neva_factory.h"
#include "third_party/blink/public/platform/web_string.h"
#include "ui/gfx/geometry/point_f.h"
#include "ui/platform_window/neva/mojom/video_window_controller.mojom.h"

namespace media {

class MEDIA_BLINK_EXPORT WebMediaPlayerParamsNeva {
 public:
  using CreateVideoWindowCB = base::RepeatingCallback<void(
      mojo::PendingRemote<ui::mojom::VideoWindowClient>,
      mojo::PendingReceiver<ui::mojom::VideoWindow>,
      const ui::VideoWindowParams&)>;
  WebMediaPlayerParamsNeva(CreateVideoWindowCB callback,
                           gfx::PointF additional_contents_scale,
                           blink::WebString application_id);

  WebMediaPlayerParamsNeva(CreateVideoWindowCB callback);
  ~WebMediaPlayerParamsNeva();

  gfx::PointF additional_contents_scale() const {
    return additional_contents_scale_;
  }

  blink::WebString application_id() const { return application_id_; }

  bool use_unlimited_media_policy() const {
    return use_unlimited_media_policy_;
  }

  CreateMediaPlayerNevaCB override_create_media_player_neva() const {
    return override_create_media_player_neva_;
  }

  CreateMediaPlatformAPICB override_create_media_platform_api() const {
    return override_create_media_platform_api_;
  }

  void set_additional_contents_scale(
      const gfx::PointF additional_contents_scale) {
    additional_contents_scale_ = additional_contents_scale;
  }

  void set_application_id(const blink::WebString application_id) {
    application_id_ = application_id;
  }

  void set_use_unlimited_media_policy(const bool use_unlimited_media_policy) {
    use_unlimited_media_policy_ = use_unlimited_media_policy;
  }

  void set_override_create_media_player_neva(
      CreateMediaPlayerNevaCB create_callback) {
    override_create_media_player_neva_ = std::move(create_callback);
  }

  void set_override_create_media_platform_api(
      CreateMediaPlatformAPICB create_callback) {
    override_create_media_platform_api_ = std::move(create_callback);
  }

  CreateVideoWindowCB&& get_create_video_window_callback() {
    return std::move(create_video_window_cb_);
  }

 protected:
  CreateVideoWindowCB create_video_window_cb_;
  gfx::PointF additional_contents_scale_{1.0f, 1.0f};
  blink::WebString application_id_;
  bool use_unlimited_media_policy_ = false;
  CreateMediaPlayerNevaCB override_create_media_player_neva_;
  CreateMediaPlatformAPICB override_create_media_platform_api_;
};

}  // namespace media

#endif  // MEDIA_BLINK_NEVA_WEBMEDIAPLAYER_PARAMS_NEVA_H_
