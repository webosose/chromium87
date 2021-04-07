// Copyright 2016-2019 LG Electronics, Inc.
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

#include "neva/app_runtime/browser/app_runtime_webview_host_impl.h"
#include "neva/app_runtime/public/webview_delegate.h"

namespace neva_app_runtime {

AppRuntimeWebViewHostImpl::AppRuntimeWebViewHostImpl(
    content::WebContents* web_contents)
    : webview_host_receivers_(web_contents, this),
      blink_delegate_receivers_(web_contents, this) {}

AppRuntimeWebViewHostImpl::~AppRuntimeWebViewHostImpl() {}

void AppRuntimeWebViewHostImpl::SetDelegate(WebViewDelegate* delegate) {
  webview_delegate_ = delegate;
}

void AppRuntimeWebViewHostImpl::DidLoadingEnd() {
  if (webview_delegate_) {
    webview_delegate_->DidLoadingEnd();
    // FIXME(neva): This workaround temporarily fixes showing of WebApp window.
    webview_delegate_->DidSwapCompositorFrame();
  }
}

void AppRuntimeWebViewHostImpl::DidFirstPaint() {
  if (webview_delegate_) {
    webview_delegate_->DidFirstPaint();
    webview_delegate_->DidSwapCompositorFrame();
  }
}

void AppRuntimeWebViewHostImpl::DidFirstContentfulPaint() {
  if (webview_delegate_)
    webview_delegate_->DidFirstContentfulPaint();
}

void AppRuntimeWebViewHostImpl::DidFirstImagePaint() {
  if (webview_delegate_)
    webview_delegate_->DidFirstImagePaint();
}

void AppRuntimeWebViewHostImpl::DidFirstMeaningfulPaint() {
  if (webview_delegate_)
    webview_delegate_->DidFirstMeaningfulPaint();
}

void AppRuntimeWebViewHostImpl::DidClearWindowObject() {
  if (webview_delegate_)
    webview_delegate_->DidClearWindowObject();
}

void AppRuntimeWebViewHostImpl::DidHistoryBackOnTopPage() {
  if (webview_delegate_)
    webview_delegate_->DidHistoryBackOnTopPage();
}

void AppRuntimeWebViewHostImpl::IsBackHistoryKeyDisabled(
    IsBackHistoryKeyDisabledCallback callback) {
  if (webview_delegate_)
    std::move(callback).Run(is_back_history_key_disabled_);
}

void AppRuntimeWebViewHostImpl::DidNonFirstMeaningPaintAfterLoad() {
  if (webview_delegate_)
    webview_delegate_->DidNonFirstMeaningfulPaint();
}

void AppRuntimeWebViewHostImpl::DidLargestContentfulPaint() {
  if (webview_delegate_)
    webview_delegate_->DidLargestContentfulPaint();
}

}  // namespace neva_app_runtime
