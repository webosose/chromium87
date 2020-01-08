// Copyright 2017-2019 LG Electronics, Inc.
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

#include "neva/app_runtime/renderer/app_runtime_page_load_timing_render_frame_observer.h"

#include "base/time/time.h"
#include "neva/app_runtime/public/mojom/app_runtime_webview.mojom.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"
#include "third_party/blink/public/web/web_performance.h"
#include "third_party/blink/public/web/web_local_frame.h"

namespace neva_app_runtime {
namespace {

base::TimeDelta ClampDelta(double event, double start) {
  if (event - start < 0)
    event = start;
  return base::Time::FromDoubleT(event) - base::Time::FromDoubleT(start);
}

}  // namespace

AppRuntimePageLoadTimingRenderFrameObserver::
    AppRuntimePageLoadTimingRenderFrameObserver(
        content::RenderFrame* render_frame)
    : content::RenderFrameObserver(render_frame) {}

AppRuntimePageLoadTimingRenderFrameObserver::
    ~AppRuntimePageLoadTimingRenderFrameObserver() {}

void AppRuntimePageLoadTimingRenderFrameObserver::DidChangePerformanceTiming() {
  // Check frame exists
  if (HasNoRenderFrame())
    return;

  if (PageLoadTimingIsLoadingEnd()) {
    mojo::AssociatedRemote<mojom::AppRuntimeWebViewHost> interface;
    render_frame()->GetRemoteAssociatedInterfaces()->GetInterface(&interface);
    if (interface)
      interface->DidLoadingEnd();
  }

  if (PageLoadTimingIsFirstPaint()) {
    mojo::AssociatedRemote<mojom::AppRuntimeWebViewHost> interface;
    render_frame()->GetRemoteAssociatedInterfaces()->GetInterface(&interface);
    if (interface)
      interface->DidFirstPaint();
  }

  if (PageLoadTimingIsFirstContentfulPaint()) {
    mojo::AssociatedRemote<mojom::AppRuntimeWebViewHost> interface;
    render_frame()->GetRemoteAssociatedInterfaces()->GetInterface(&interface);
    if (interface)
      interface->DidFirstContentfulPaint();
  }

  if (PageLoadTimingIsFirstImagePaint()) {
    mojo::AssociatedRemote<mojom::AppRuntimeWebViewHost> interface;
    render_frame()->GetRemoteAssociatedInterfaces()->GetInterface(&interface);
    if (interface)
      interface->DidFirstImagePaint();
  }

  if (PageLoadTimingIsFirstMeaningfulPaint()) {
    mojo::AssociatedRemote<mojom::AppRuntimeWebViewHost> interface;
    render_frame()->GetRemoteAssociatedInterfaces()->GetInterface(&interface);
    if (interface)
      interface->DidFirstMeaningfulPaint();
  }

  if (PageLoadTimingIsLargestContentfulPaint()) {
    mojo::AssociatedRemote<mojom::AppRuntimeWebViewHost> interface;
    render_frame()->GetRemoteAssociatedInterfaces()->GetInterface(&interface);
    if (interface)
      interface->DidLargestContentfulPaint();
  }
}

void AppRuntimePageLoadTimingRenderFrameObserver::
    DidResetStateToMarkNextPaint() {
  loading_end_ = base::nullopt;
  first_paint_ = base::nullopt;
  first_contentful_paint_ = base::nullopt;
  first_text_paint_ = base::nullopt;
  first_image_paint_ = base::nullopt;
  first_meaningful_paint_ = base::nullopt;
}

void AppRuntimePageLoadTimingRenderFrameObserver::OnDestruct() {
  delete this;
}

bool AppRuntimePageLoadTimingRenderFrameObserver::HasNoRenderFrame() const {
  bool no_frame = !render_frame() || !render_frame()->GetWebFrame();
  DCHECK(!no_frame);
  return no_frame;
}

bool AppRuntimePageLoadTimingRenderFrameObserver::
    PageLoadTimingIsLoadingEnd() {
  if (loading_end_)
    return false;

  const blink::WebPerformance& perf =
      render_frame()->GetWebFrame()->Performance();

  if (perf.LoadEventEnd() > 0.0) {
    loading_end_ =
        ClampDelta(perf.LoadEventEnd(), perf.NavigationStart());
    return true;
  }
  return false;
}

bool AppRuntimePageLoadTimingRenderFrameObserver::
    PageLoadTimingIsFirstPaint() {
  if (first_paint_)
    return false;

  const blink::WebPerformance& perf =
      render_frame()->GetWebFrame()->Performance();

  if (perf.FirstPaint() > 0.0) {
    first_paint_ =
        ClampDelta(perf.FirstPaint(), perf.NavigationStart());
    return true;
  }
  return false;
}

bool AppRuntimePageLoadTimingRenderFrameObserver::
    PageLoadTimingIsFirstContentfulPaint() {
  if (first_contentful_paint_)
    return false;

  const blink::WebPerformance& perf =
      render_frame()->GetWebFrame()->Performance();

  if (perf.FirstContentfulPaint() > 0.0) {
    first_contentful_paint_ =
        ClampDelta(perf.FirstContentfulPaint(), perf.NavigationStart());
    return true;
  }
  return false;
}

bool AppRuntimePageLoadTimingRenderFrameObserver::
    PageLoadTimingIsFirstImagePaint() {
  if (first_image_paint_)
    return false;

  const blink::WebPerformance& perf =
      render_frame()->GetWebFrame()->Performance();

  if (perf.FirstImagePaint() > 0.0) {
    first_image_paint_ =
        ClampDelta(perf.FirstImagePaint(), perf.NavigationStart());
    return true;
  }
  return false;
}

bool AppRuntimePageLoadTimingRenderFrameObserver::
    PageLoadTimingIsFirstMeaningfulPaint() {
  if (first_meaningful_paint_)
    return false;

  const blink::WebPerformance& perf =
      render_frame()->GetWebFrame()->Performance();

  if (perf.FirstMeaningfulPaint() > 0.0) {
    first_meaningful_paint_ =
        ClampDelta(perf.FirstMeaningfulPaint(), perf.NavigationStart());
    return true;
  }
  return false;
}

bool AppRuntimePageLoadTimingRenderFrameObserver::
    PageLoadTimingIsLargestContentfulPaint() {
  const blink::WebPerformance& perf =
      render_frame()->GetWebFrame()->Performance();

  if ((perf.LargestTextPaint() < 0.0) && (perf.LargestImagePaint() < 0.0))
    return false;

  if ((perf.LargestTextPaintSize() < largest_contentful_paint_size_) &&
      (perf.LargestImagePaintSize() < largest_contentful_paint_size_))
    return false;

  if (perf.LargestTextPaintSize() > perf.LargestImagePaintSize()) {
    largest_contentful_paint_ =
        ClampDelta(perf.LargestTextPaint(), perf.NavigationStart());
    largest_contentful_paint_size_ = perf.LargestTextPaintSize();
  } else {
    largest_contentful_paint_ =
        ClampDelta(perf.LargestImagePaint(), perf.NavigationStart());
    largest_contentful_paint_size_ = perf.LargestImagePaintSize();
  }
  return true;
}

}  // namespace neva_app_runtime
