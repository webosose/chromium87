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

#include "components/local_storage_manager/common/local_storage_manager_store.h"

#include "base/bind.h"
#include "base/logging.h"

namespace content {

LocalStorageManagerStore::LocalStorageManagerStore(
    scoped_refptr<base::SingleThreadTaskRunner> main_thread_runner,
    scoped_refptr<base::SingleThreadTaskRunner> db_thread_runner)
    : main_thread_runner_(main_thread_runner),
      db_thread_runner_(db_thread_runner),
      db_initialized_(false) {
  DCHECK(main_thread_runner != nullptr);
  DCHECK(db_thread_runner != nullptr);
}

void LocalStorageManagerStore::Initialize(
    const base::FilePath& data_file_path,
    const base::Callback<void(bool)>& callback) {
  VLOG(1) << "##### Store initialization; data path="
          << data_file_path.AsUTF8Unsafe();
  db_.reset(new LocalStorageManagerDatabase(data_file_path));
  RunOnDBThread(base::Bind(&LocalStorageManagerStore::InitializeOnDBThread,
                           base::Unretained(this), callback));
}

void LocalStorageManagerStore::AddAccess(
    const AccessData& access,
    const base::Callback<void(bool)>& callback) {
  RunOnDBThread(base::Bind(&LocalStorageManagerStore::AddAccessOnDBThread,
                           base::Unretained(this), access, callback));
}

void LocalStorageManagerStore::AddApplication(
    const ApplicationData& application,
    const base::Callback<void(bool)>& callback) {
  RunOnDBThread(base::Bind(&LocalStorageManagerStore::AddApplicationOnDBThread,
                           base::Unretained(this), application, callback));
}

void LocalStorageManagerStore::AddOrigin(
    const OriginData& origin,
    const base::Callback<void(bool)>& callback) {
  RunOnDBThread(base::Bind(&LocalStorageManagerStore::AddOriginOnDBThread,
                           base::Unretained(this), origin, callback));
}

void LocalStorageManagerStore::DeleteApplication(
    const std::string& app_id,
    const base::Callback<void(bool)>& callback) {
  RunOnDBThread(
      base::Bind(&LocalStorageManagerStore::DeleteApplicationOnDBThread,
                 base::Unretained(this), app_id, callback));
}

void LocalStorageManagerStore::DeleteOrigin(
    const GURL& url,
    const base::Callback<void(bool)>& callback) {
  RunOnDBThread(base::Bind(&LocalStorageManagerStore::DeleteOriginOnDBThread,
                           base::Unretained(this), url, callback));
}

void LocalStorageManagerStore::GetAccesses(
    const base::Callback<void(bool, const AccessDataList&)>& callback) {
  RunOnDBThread(base::Bind(&LocalStorageManagerStore::GetAccessesOnDBThread,
                           base::Unretained(this), callback));
}
void LocalStorageManagerStore::GetApplications(
    const base::Callback<void(bool, const ApplicationDataList&)>& callback) {
  RunOnDBThread(base::Bind(&LocalStorageManagerStore::GetApplicationsOnDBThread,
                           base::Unretained(this), callback));
}

void LocalStorageManagerStore::InitializeOnDBThread(
    const base::Callback<void(bool)>& callback) {
  db_initialized_ = db_->Init() == sql::INIT_OK;
  main_thread_runner_->PostTask(FROM_HERE,
                                base::Bind(callback, db_initialized_));
}

void LocalStorageManagerStore::AddAccessOnDBThread(
    const AccessData& access,
    const base::Callback<void(bool)>& callback) {
  bool result = false;
  if (db_initialized_) {
    result = db_->AddAccess(access);
  }
  RunOnUIThread(base::Bind(callback, result));
}

void LocalStorageManagerStore::AddApplicationOnDBThread(
    const ApplicationData& application,
    const base::Callback<void(bool)>& callback) {
  bool result = false;
  if (db_initialized_) {
    result = db_->AddApplication(application);
  }
  RunOnUIThread(base::Bind(callback, result));
}

void LocalStorageManagerStore::AddOriginOnDBThread(
    const OriginData& origin,
    const base::Callback<void(bool)>& callback) {
  bool result = false;
  if (db_initialized_) {
    result = db_->AddOrigin(origin);
  }
  RunOnUIThread(base::Bind(callback, result));
}

void LocalStorageManagerStore::DeleteApplicationOnDBThread(
    const std::string& app_id,
    const base::Callback<void(bool)>& callback) {
  bool result = false;
  if (db_initialized_) {
    result = db_->DeleteApplication(app_id);
  }
  RunOnUIThread(base::Bind(callback, result));
}

void LocalStorageManagerStore::DeleteOriginOnDBThread(
    const GURL& url,
    const base::Callback<void(bool)>& callback) {
  bool result = false;
  if (db_initialized_) {
    result = db_->DeleteOrigin(url);
  }
  RunOnUIThread(base::Bind(callback, result));
}

void LocalStorageManagerStore::GetAccessesOnDBThread(
    const base::Callback<void(bool, const AccessDataList&)>& callback) {
  AccessDataList data_list;
  bool result = false;
  if (db_initialized_) {
    result = db_->GetAccesses(&data_list);
  }
  RunOnUIThread(base::Bind(callback, result, data_list));
}

void LocalStorageManagerStore::GetApplicationsOnDBThread(
    const base::Callback<void(bool, const ApplicationDataList&)>& callback) {
  ApplicationDataList data_list;
  bool result = false;
  if (db_initialized_) {
    result = db_->GetApplications(&data_list);
  }
  RunOnUIThread(base::Bind(callback, result, data_list));
}

void LocalStorageManagerStore::RunOnDBThread(const base::Closure& task) {
  db_thread_runner_->PostTask(FROM_HERE, task);
}

void LocalStorageManagerStore::RunOnUIThread(const base::Closure& task) {
  main_thread_runner_->PostTask(FROM_HERE, task);
}

}  // namespace content
