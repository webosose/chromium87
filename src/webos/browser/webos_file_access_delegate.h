// Copyright 2016-2020 LG Electronics, Inc.
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

#ifndef WEBOS_BROWSER_FILE_ACCESS_DELEGATE_H_
#define WEBOS_BROWSER_FILE_ACCESS_DELEGATE_H_

#include <string>
#include <vector>

#include "neva/app_runtime/browser/app_runtime_file_access_delegate.h"

namespace webos {

class WebOSFileAccessDelegate
    : public neva_app_runtime::AppRuntimeFileAccessDelegate {
 public:
  WebOSFileAccessDelegate();

  // neva_app_runtime::AppRuntimeFileAccessDelegate implementation
  bool IsAccessAllowed(const base::FilePath& path,
                       int process_id,
                       int route_id,
                       int frame_tree_node_id) const override;

 private:
  void ParsePathsFromSettings(std::vector<std::string>& paths,
                              std::istringstream& stream) const;

  std::vector<std::string> allowed_target_paths_;
  std::vector<std::string> allowed_trusted_target_paths_;
  bool allow_all_access_ = true;
};

}  // namespace webos

#endif  // WEBOS_BROWSER_FILE_ACCESS_DELEGATE_H_
