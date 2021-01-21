// Copyright 2013 The Chromium Authors. All rights reserved.
// Copyright 2013 Intel Corporation. All rights reserved.
// Copyright 2016-2018 LG Electronics, Inc.
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

#include "ozone/wayland/screen.h"

#include <wayland-client.h>

#include "ozone/wayland/display.h"
#include "url/third_party/mozilla/url_parse.h"

namespace ozonewayland {
namespace {
#if defined(OS_WEBOS)
const char kDisplayIdQueryKey[] = "display_id";
const char kDisplayNameQueryKey[] = "name";

const char kNotFound[] = "*** not found ***";

const uint32_t kOutputInterfaceVersion = 2;
#else
const uint32_t kOutputInterfaceVersion = 1;
#endif
const char kDefaultDisplayId[] = "-1";
const int kNoTransform = 0;
const int k90DegressCounterClockwise = 90;
const int k180DegreesCounterClockwise = 180;
const int k270DegreesCounterClockwise = 270;
}  // namespace

WaylandScreen::WaylandScreen(wl_registry* registry, uint32_t id)
    : output_(nullptr),
#if defined(OS_WEBOS)
      pending_display_id_(kDefaultDisplayId),
      pending_display_name_(),
      pending_rect_(0, 0, 0, 0),
      pending_transform_(0),
#endif
      display_id_(kDefaultDisplayId),
      display_name_(),
      rect_(0, 0, 0, 0) {
  static const wl_output_listener kOutputListener = {
    WaylandScreen::OutputHandleGeometry,
    WaylandScreen::OutputHandleMode,
#if defined(OS_WEBOS)
    WaylandScreen::OutputDone,
    WaylandScreen::OutputScale,
#endif
  };

  output_ =
      static_cast<wl_output*>(wl_registry_bind(registry,
                                               id,
                                               &wl_output_interface,
                                               kOutputInterfaceVersion));
  wl_output_add_listener(output_, &kOutputListener, this);
  DCHECK(output_);
}

WaylandScreen::~WaylandScreen() {
  wl_output_destroy(output_);
}

int WaylandScreen::GetOutputTransformDegrees() const {
  int result = kNoTransform;
  auto transform = transform_.value_or(WL_OUTPUT_TRANSFORM_NORMAL);

  switch (transform) {
    case WL_OUTPUT_TRANSFORM_90:
      result = k90DegressCounterClockwise;
      break;
    case WL_OUTPUT_TRANSFORM_180:
      result = k180DegreesCounterClockwise;
      break;
    case WL_OUTPUT_TRANSFORM_270:
      result = k270DegreesCounterClockwise;
      break;
    default:
      break;
  }

  return result;
}

std::string WaylandScreen::GetQueryParam(const std::string& query_str,
                                         const std::string& param_name) {
  url::Component query(0, query_str.length());
  url::Component key, value;
  while (url::ExtractQueryKeyValue(query_str.c_str(), &query, &key, &value)) {
    std::string key_string(query_str.substr(key.begin, key.len));
    std::string param_text(query_str.substr(value.begin, value.len));
    if (key_string == param_name)
      return param_text;
  }
  return kNotFound;
}

// static
void WaylandScreen::OutputHandleGeometry(void *data,
                                         wl_output *output,
                                         int32_t x,
                                         int32_t y,
                                         int32_t physical_width,
                                         int32_t physical_height,
                                         int32_t subpixel,
                                         const char* make,
                                         const char* model,
                                         int32_t output_transform) {
  WaylandScreen* screen = static_cast<WaylandScreen*>(data);
#if defined(OS_WEBOS)
  std::string display_id = screen->GetQueryParam(model, kDisplayIdQueryKey);
  if (display_id != kNotFound)
    screen->pending_display_id_ = display_id;

  std::string display_name = screen->GetQueryParam(model, kDisplayNameQueryKey);
  if (display_name != kNotFound)
    screen->pending_display_name_ = display_name;

  // We don't really support other than (0,0) origin
  screen->pending_rect_.set_origin(gfx::Point(x, y));
  screen->pending_transform_ = output_transform;
#else
  screen->rect_.set_origin(gfx::Point(x, y));
#endif
}

// static
void WaylandScreen::OutputHandleMode(void* data,
                                     wl_output* wl_output,
                                     uint32_t flags,
                                     int32_t width,
                                     int32_t height,
                                     int32_t refresh) {
  WaylandScreen* screen = static_cast<WaylandScreen*>(data);
  if (flags & WL_OUTPUT_MODE_CURRENT) {
#if defined(OS_WEBOS)
    screen->pending_rect_.set_size(gfx::Size(width, height));
#else
    screen->rect_.set_size(gfx::Size(width, height));

    if (WaylandDisplay::GetInstance())
      WaylandDisplay::GetInstance()->OutputScreenChanged(
          screen->display_id_, screen->display_name_, screen->rect_.width(),
          screen->rect_.height(), screen->GetOutputTransformDegrees());
#endif
  }
}

#if defined(OS_WEBOS)
// static
void WaylandScreen::OutputDone(void* data, struct wl_output* wl_output) {
  WaylandScreen* screen = static_cast<WaylandScreen*>(data);

  if (screen->display_id_ != screen->pending_display_id_ ||
      screen->display_name_ != screen->pending_display_name_ ||
      screen->rect_ != screen->pending_rect_ ||
      screen->transform_ != screen->pending_transform_) {
    screen->display_id_ = screen->pending_display_id_;
    screen->display_name_ = screen->pending_display_name_;
    screen->rect_ = screen->pending_rect_;
    screen->transform_ = screen->pending_transform_;

    // In case of OzoneWaylandScreen::LookAheadOutputGeometry we reach this
    // point and WaylandDisplay instance is still null. That is intentional
    // because we need to ensure we have fetched geometry before continuing
    // Ozone initialization.
    if (WaylandDisplay::GetInstance())
      WaylandDisplay::GetInstance()->OutputScreenChanged(
          screen->display_id_, screen->display_name_, screen->rect_.width(),
          screen->rect_.height(), screen->GetOutputTransformDegrees());
  }
}

// static
void WaylandScreen::OutputScale(void* data, struct wl_output* wl_output, int32_t factor) {
  NOTIMPLEMENTED();
}
#endif

}  // namespace ozonewayland
