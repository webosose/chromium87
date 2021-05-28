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

#ifndef COMPONENTS_LOCAL_STORAGE_TRACKER_COMMON_LOCAL_STORAGE_TRACKER_TYPES_H_
#define COMPONENTS_LOCAL_STORAGE_TRACKER_COMMON_LOCAL_STORAGE_TRACKER_TYPES_H_

#include <vector>

#include "url/gurl.h"

namespace content {

class OriginData {
 public:
  OriginData() = default;
  OriginData(const OriginData&) = default;
  GURL url_;
};

typedef std::vector<OriginData> OriginDataList;

class ApplicationData {
 public:
  std::string app_id_;
  bool installed_;
};

typedef std::vector<ApplicationData> ApplicationDataList;

class AccessData {
 public:
  std::string app_id_;
  GURL origin_;
  bool operator<(const AccessData& access_data) const {
    return app_id_ < access_data.app_id_;
  };
};

typedef std::vector<AccessData> AccessDataList;

}  // namespace content

#endif  // COMPONENTS_LOCAL_STORAGE_TRACKER_COMMON_LOCAL_STORAGE_TRACKER_TYPES_H_