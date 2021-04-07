// Copyright 2017 LG Electronics, Inc.
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

#ifndef NEVA_APP_RUNTIME_BROWSER_APP_RUNTIME_BROWSER_CONTEXT_ADAPTER_H_
#define NEVA_APP_RUNTIME_BROWSER_APP_RUNTIME_BROWSER_CONTEXT_ADAPTER_H_

#include <string>

namespace neva_app_runtime {

class AppRuntimeBrowserContext;

class BrowserContextAdapter {
 public:
  BrowserContextAdapter(const std::string& storage_name);
  BrowserContextAdapter(const BrowserContextAdapter&) = delete;
  BrowserContextAdapter& operator=(const BrowserContextAdapter&) = delete;
  virtual ~BrowserContextAdapter();

  static BrowserContextAdapter* GetDefaultContext();

  AppRuntimeBrowserContext* GetBrowserContext() const;

  std::string GetStorageName() const;

  void FlushCookieStore();

 private:
  std::string storage_name_;
  AppRuntimeBrowserContext* browser_context_;
};

}  // namespace neva_app_runtime

#endif  // NEVA_APP_RUNTIME_BROWSER_APP_RUNTIME_BROWSER_CONTEXT_ADAPTER_H_
