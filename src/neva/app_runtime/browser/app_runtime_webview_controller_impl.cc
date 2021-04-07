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

#include "neva/app_runtime/browser/app_runtime_webview_controller_impl.h"
#include "neva/app_runtime/public/webview_controller_delegate.h"

namespace neva_app_runtime {

AppRuntimeWebViewControllerImpl::AppRuntimeWebViewControllerImpl(
    content::WebContents* web_contents)
  : receivers_(web_contents, this) {
}

AppRuntimeWebViewControllerImpl::~AppRuntimeWebViewControllerImpl() {
}

void AppRuntimeWebViewControllerImpl::SetDelegate(
    WebViewControllerDelegate* delegate) {
  webview_controller_delegate_ = delegate;
}

void AppRuntimeWebViewControllerImpl::CallFunction(
    const std::string& name,
    const std::vector<std::string>& args,
    CallFunctionCallback callback) {
  std::string result;
  if (webview_controller_delegate_)
    result = webview_controller_delegate_->RunFunction(name, args);
  std::move(callback).Run(std::move(result));
}

void AppRuntimeWebViewControllerImpl::SendCommand(
    const std::string& name, const std::vector<std::string>& args) {
  if (webview_controller_delegate_)
    webview_controller_delegate_->RunCommand(name, args);
}

}  // namespace neva_app_runtime
