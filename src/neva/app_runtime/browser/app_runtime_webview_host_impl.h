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

#ifndef NEVA_APP_RUNTIME_BROWSER_APP_RUNTIME_WEBVIEW_HOST_IMPL_H_
#define NEVA_APP_RUNTIME_BROWSER_APP_RUNTIME_WEBVIEW_HOST_IMPL_H_

#include "content/public/browser/web_contents_receiver_set.h"
#include "neva/app_runtime/public/mojom/app_runtime_webview.mojom.h"
#include "third_party/blink/public/mojom/neva/app_runtime_blink_delegate.mojom.h"

namespace content {

class WebContents;

}  // namespace content

namespace neva_app_runtime {

class WebViewDelegate;

class AppRuntimeWebViewHostImpl : public mojom::AppRuntimeWebViewHost,
                                  public blink::mojom::AppRuntimeBlinkDelegate {
 public:
  AppRuntimeWebViewHostImpl(content::WebContents* web_contents);
  ~AppRuntimeWebViewHostImpl() override;

  void SetDelegate(WebViewDelegate* delegate);
  void SetBackHistoryKeyDisabled(bool disabled) {
    is_back_history_key_disabled_ = disabled;
  }

  // mojom::AppRuntimeWebViewHost implementation.
  void DidLoadingEnd() override;
  void DidFirstPaint() override;
  void DidFirstContentfulPaint() override;
  void DidFirstImagePaint() override;
  void DidFirstMeaningfulPaint() override;
  void DidClearWindowObject() override;
  void DidLargestContentfulPaint() override;
  void DidResumeDOM() override;

  // blink::mojom::AppRuntimeBlinkDelegate implementation.
  void IsBackHistoryKeyDisabled(
      IsBackHistoryKeyDisabledCallback callback) override;
  void DidNonFirstMeaningPaintAfterLoad() override;
  void DidHistoryBackOnTopPage() override;

 private:
  content::WebContentsFrameReceiverSet<mojom::AppRuntimeWebViewHost>
      webview_host_receivers_;
  content::WebContentsFrameReceiverSet<blink::mojom::AppRuntimeBlinkDelegate>
      blink_delegate_receivers_;
  WebViewDelegate* webview_delegate_ = nullptr;
  bool is_back_history_key_disabled_ = false;
};

}  // namespace neva_app_runtime

#endif  // NEVA_APP_RUNTIME_BROWSER_APP_RUNTIME_WEBVIEW_HOST_IMPL_H_
