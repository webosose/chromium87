// Copyright (c) 2020 LG Electronics, Inc.
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

#ifndef COMPONENTS_LOCAL_STORAGE_MANAGER_COMMON_LOCAL_STORAGE_MANAGER_DATABASE_H_
#define COMPONENTS_LOCAL_STORAGE_MANAGER_COMMON_LOCAL_STORAGE_MANAGER_DATABASE_H_

#include <string>

#include "base/files/file_util.h"
#include "components/local_storage_manager/common/local_storage_manager_types.h"
#include "sql/init_status.h"
#include "sql/statement.h"
#include "sql/transaction.h"
#include "url/gurl.h"

namespace content {

class LocalStorageManagerDatabase {
 public:
  explicit LocalStorageManagerDatabase(const base::FilePath& data_file_name);

  bool AddAccess(const AccessData& access);
  bool AddApplication(const ApplicationData& application);
  bool AddOrigin(const OriginData& origin);
  bool GetAccesses(AccessDataList* accesses);
  bool GetApplications(ApplicationDataList* applications);
  bool DeleteApplication(const std::string& app_id);
  bool DeleteOrigin(const GURL& url);
  sql::InitStatus Init();

 private:
  bool CreateAppsTable();
  bool CreateLocalStorageAccessTable();
  bool CreateOriginsTable();
  bool EnsurePath(const base::FilePath& path);

  sql::Database db_;
  base::FilePath data_file_name_;
};
}  // namespace content

#endif  // COMPONENTS_LOCAL_STORAGE_MANAGER_COMMON_LOCAL_STORAGE_MANAGER_DATABASE_H_
