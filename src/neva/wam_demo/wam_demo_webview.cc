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

#include "neva/wam_demo/wam_demo_webview.h"

#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "neva/app_runtime/webview_profile.h"
#include "neva/wam_demo/wam_demo_webview_observer.h"

namespace wam_demo {

bool WamDemoWebView::policy_ = false;

WamDemoWebView::WamDemoWebView(std::string appid,
                               WamDemoWebViewObserver* observer)
    : observer_(observer)
    , appid_(std::move(appid)) {
}

WamDemoWebView::WamDemoWebView(std::string appid,
                               int width, int height,
                               WamDemoWebViewObserver* observer,
                               neva_app_runtime::WebViewProfile* profile)
    : WebViewBase(width, height, profile)
    , observer_(observer)
    , appid_(std::move(appid)) {
}

void WamDemoWebView::OnLoadProgressChanged(double progress) {
  progress_ = base::NumberToString((int)std::floor(progress * 100 + 0.5));

  std::string progress_title = title_;
  if (progress_ != "100")
    progress_title += " " + progress_ + "%";

  if (observer_)
    observer_->OnTitleChanged(this, progress_title);

  LOG(INFO) << __func__ << "(): " << progress_title;
}

void WamDemoWebView::DidFirstFrameFocused() {
  LOG(INFO) << __func__ << "(): Did frame focused is delivered";
}

void WamDemoWebView::DidLoadingEnd() {
  LOG(INFO) << __func__ << "(): Loading end is delivered";

  if (observer_)
    observer_->OnDidLoadingEnd(this);
}

void WamDemoWebView::DidFirstPaint() {
  LOG(INFO) << __func__ << "(): First paint is delivered";

  if (observer_)
    observer_->OnDidFirstPaint(this);
}

void WamDemoWebView::DidFirstContentfulPaint() {
  LOG(INFO) << __func__ << "(): First contentful paint is delivered";

  if (observer_)
    observer_->OnDidFirstContentfulPaint(this);
}

void WamDemoWebView::DidFirstImagePaint() {
  LOG(INFO) << __func__ << "(): First image paint is delivered";

  if (observer_)
    observer_->OnDidFirstImagePaint(this);
}

void WamDemoWebView::DidFirstMeaningfulPaint() {
  LOG(INFO) << __func__ << "(): First meaningful paint is delivered";

  if (observer_)
    observer_->OnDidFirstMeaningfulPaint(this);
}

void WamDemoWebView::DidNonFirstMeaningfulPaint() {
  LOG(INFO) << __func__ << "(): Non first meaningful paint is delivered";

  if (observer_)
    observer_->OnDidNonFirstMeaningfulPaint(this);
}

void WamDemoWebView::DidLargestContentfulPaint() {
  LOG(INFO) << __func__ << "(): Largest contentful paint is delivered";

  if (observer_)
    observer_->OnDidLargestContentfulPaint(this);
}

void WamDemoWebView::TitleChanged(const std::string& title) {
  title_ = title;
  std::string progress_title = title;
  if (progress_ != "100")
    progress_title += " " + progress_ + "%";

  if (observer_)
    observer_->OnTitleChanged(this, progress_title);
}

void WamDemoWebView::NavigationHistoryChanged() {
  LOG(INFO) << __func__
            << "(): Navigation history changed notification is delivered";
}

void WamDemoWebView::Close() {
  if (observer_)
    observer_->OnClose(this);
}

bool WamDemoWebView::DecidePolicyForResponse(bool is_main_frame,
                                        int status_code,
                                        const std::string& url,
                                        const std::string& status_text) {
  return policy_;
}

void WamDemoWebView::SetDecidePolicyForResponse() {
  policy_ = true;
}

void WamDemoWebView::SetMediaCapturePermission() {
  LOG(INFO) << __func__ << "(): Set media capture permission is delivered";
  accepts_video_capture_ = accepts_audio_capture_ = true;
}

void WamDemoWebView::ClearMediaCapturePermission() {
  LOG(INFO) << __func__ << "(): Clear media capture permission is delivered";
  accepts_video_capture_ = accepts_audio_capture_ = false;
}

bool WamDemoWebView::AcceptsVideoCapture() {
  LOG(INFO) << __func__ << "(): Accepts video capture is delivered";
  return accepts_video_capture_;
}

bool WamDemoWebView::AcceptsAudioCapture() {
  LOG(INFO) << __func__ << "(): Accepts audio capture is delivered";
  return accepts_audio_capture_;
}

void WamDemoWebView::LoadStarted() {
  LOG(INFO) << __func__ << "(): Load started notification is delivered";
}

void WamDemoWebView::LoadFinished(const std::string& url) {
  LOG(INFO) << __func__
            << "(): Load finished notification is delivered"
            << " for url [" << url << "]";
}

void WamDemoWebView::LoadFailed(const std::string& url,
                           int error_code,
                           const std::string& error_description) {
  LOG(INFO) << __func__
            << "(): Load failed notification is delivered"
            << " for url [" << url << "]";
  if (observer_)
    observer_->OnLoadFailed(this);
}

void WamDemoWebView::LoadAborted(const std::string& url) {
  LOG(INFO) << __func__
            << "(): Load aborted notification is delivered"
            << " for url [" << url << "]";
  if (observer_)
    observer_->OnLoadFailed(this);
}

void WamDemoWebView::LoadStopped() {
  LOG(INFO) << __func__ << "(): Load stopped notification is delivered";
}

void WamDemoWebView::RenderProcessCreated(int pid) {
  if (observer_)
    observer_->OnRenderProcessCreated(this);
}

void WamDemoWebView::RenderProcessGone() {
  if (observer_)
    observer_->OnRenderProcessGone(this);
}

void WamDemoWebView::DocumentLoadFinished() {
  LOG(INFO) << __func__
            << "(): Document load finished notification is delivered";
  if (observer_)
    observer_->OnDocumentLoadFinished(this);
}

void WamDemoWebView::DidStartNavigation(const std::string& url, bool is_main_frame) {
  LOG(INFO) << __func__
            << "(): Did start navigation notification is delivered"
            << " for url [" << url << "]";
}

void WamDemoWebView::DidFinishNavigation(const std::string& url, bool is_main_frame) {
  LOG(INFO) << __func__
            << "(): Did finish navigation notification is delivered"
            << " for url [" << url << "]";
}

void WamDemoWebView::DidHistoryBackOnTopPage() {
  LOG(INFO) << __func__
            << "(): Did history back on top page notification is delivered";
}

void WamDemoWebView::DidClearWindowObject() {
  LOG(INFO) << __func__
            << "(): Did clear window object notification is delivered";
}

void WamDemoWebView::DidSwapCompositorFrame() {
  LOG(INFO) << __func__
            << "(): Did swap window frame notification is delivered";
}

void WamDemoWebView::DidResumeDOM() {
  LOG(INFO) << __func__ << "(): Did ResumeDOM notification is delivered";
}

void WamDemoWebView::DidDropAllPeerConnections(
    neva_app_runtime::DropPeerConnectionReason reason) {}

void WamDemoWebView::RunCommand(const std::string& name,
                           const std::vector<std::string>& arguments) {
  if (observer_)
    observer_->OnBrowserControlCommand(this, name, arguments);
}

std::string WamDemoWebView::RunFunction(const std::string& name,
                            const std::vector<std::string>& arguments) {
  return observer_ ? observer_->OnBrowserControlFunction(this, name, arguments)
                   : std::string();
}

}  // namespace wam_demo
