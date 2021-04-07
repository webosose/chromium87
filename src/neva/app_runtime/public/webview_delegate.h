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

#ifndef NEVA_APP_RUNTIME_PUBLIC_WEBVIEW_DELEGATE_H_
#define NEVA_APP_RUNTIME_PUBLIC_WEBVIEW_DELEGATE_H_

#include <string>

#include "neva/app_runtime/public/app_runtime_constants.h"

namespace neva_app_runtime {

class WebViewDelegate {
 public:
  virtual void OnLoadProgressChanged(double progress) = 0;
  virtual void DidFirstFrameFocused() = 0;
  virtual void TitleChanged(const std::string& title) = 0;
  virtual void NavigationHistoryChanged() = 0;
  virtual void Close() = 0;
  virtual bool DecidePolicyForResponse(bool is_main_frame,
                                       int status_code,
                                       const std::string& url,
                                       const std::string& status_text) = 0;
  virtual bool AcceptsVideoCapture() = 0;
  virtual bool AcceptsAudioCapture() = 0;
  virtual void LoadStarted() = 0;
  virtual void LoadFinished(const std::string& url) = 0;
  virtual void LoadFailed(const std::string& url,
                          int error_code,
                          const std::string& error_description) = 0;
  virtual void LoadAborted(const std::string& url) = 0;
  virtual void LoadStopped() = 0;
  virtual void RenderProcessCreated(int pid) = 0;
  virtual void RenderProcessGone() = 0;
  virtual void DocumentLoadFinished() = 0;
  virtual void DidStartNavigation(const std::string& url, bool is_main_frame) {}
  virtual void DidFinishNavigation(const std::string& url, bool is_main_frame) {}
  virtual void DidHistoryBackOnTopPage() = 0;
  virtual void DidClearWindowObject() = 0;
  virtual void DidSwapCompositorFrame() = 0;

  virtual void DidLoadingEnd() {}
  virtual void DidFirstPaint() {}
  virtual void DidFirstContentfulPaint() {}
  virtual void DidFirstImagePaint() {}
  virtual void DidFirstMeaningfulPaint() {}
  virtual void DidNonFirstMeaningfulPaint() {}
  virtual void DidLargestContentfulPaint() {}
  virtual void DidDropAllPeerConnections(
      neva_app_runtime::DropPeerConnectionReason reason) {}
  virtual void DidResumeDOM() {}

  // Pluggable delegate
  virtual void SendCookiesForHostname(const std::string& cookies) {}
};

}  // namespace neva_app_runtime

#endif  // NEVA_APP_RUNTIME_PUBLIC_WEBVIEW_DELEGATE_H_
