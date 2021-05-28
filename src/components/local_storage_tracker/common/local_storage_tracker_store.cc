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

#include "components/local_storage_tracker/common/local_storage_tracker_store.h"

#include "base/bind.h"
#include "base/logging.h"

namespace content {

LocalStorageTrackerStore::LocalStorageTrackerStore(
    scoped_refptr<base::SingleThreadTaskRunner> main_thread_runner,
    scoped_refptr<base::SingleThreadTaskRunner> db_thread_runner)
    : main_thread_runner_(main_thread_runner),
      db_thread_runner_(db_thread_runner),
      db_initialized_(false) {
  DCHECK(main_thread_runner != nullptr);
  DCHECK(db_thread_runner != nullptr);
}

void LocalStorageTrackerStore::Initialize(
    const base::FilePath& data_file_path,
    const base::Callback<void(bool)>& callback) {
  VLOG(1) << "##### Store initialization; data path="
          << data_file_path.AsUTF8Unsafe();
  db_.reset(new LocalStorageTrackerDatabase(data_file_path));
  RunOnDBThread(base::Bind(&LocalStorageTrackerStore::InitializeOnDBThread,
                           base::Unretained(this), callback));
}

void LocalStorageTrackerStore::AddAccess(
    const AccessData& access,
    const base::Callback<void(bool)>& callback) {
  RunOnDBThread(base::Bind(&LocalStorageTrackerStore::AddAccessOnDBThread,
                           base::Unretained(this), access, callback));
}

void LocalStorageTrackerStore::AddApplication(
    const ApplicationData& application,
    const base::Callback<void(bool)>& callback) {
  RunOnDBThread(base::Bind(&LocalStorageTrackerStore::AddApplicationOnDBThread,
                           base::Unretained(this), application, callback));
}

void LocalStorageTrackerStore::AddOrigin(
    const OriginData& origin,
    const base::Callback<void(bool)>& callback) {
  RunOnDBThread(base::Bind(&LocalStorageTrackerStore::AddOriginOnDBThread,
                           base::Unretained(this), origin, callback));
}

void LocalStorageTrackerStore::DeleteApplication(
    const std::string& app_id,
    const base::Callback<void(bool)>& callback) {
  RunOnDBThread(
      base::Bind(&LocalStorageTrackerStore::DeleteApplicationOnDBThread,
                 base::Unretained(this), app_id, callback));
}

void LocalStorageTrackerStore::DeleteOrigin(
    const GURL& url,
    const base::Callback<void(bool)>& callback) {
  RunOnDBThread(base::Bind(&LocalStorageTrackerStore::DeleteOriginOnDBThread,
                           base::Unretained(this), url, callback));
}

void LocalStorageTrackerStore::GetAccesses(
    const base::Callback<void(bool, const AccessDataList&)>& callback) {
  RunOnDBThread(base::Bind(&LocalStorageTrackerStore::GetAccessesOnDBThread,
                           base::Unretained(this), callback));
}
void LocalStorageTrackerStore::GetApplications(
    const base::Callback<void(bool, const ApplicationDataList&)>& callback) {
  RunOnDBThread(base::Bind(&LocalStorageTrackerStore::GetApplicationsOnDBThread,
                           base::Unretained(this), callback));
}

void LocalStorageTrackerStore::InitializeOnDBThread(
    const base::Callback<void(bool)>& callback) {
  db_initialized_ = db_->Init() == sql::INIT_OK;
  main_thread_runner_->PostTask(FROM_HERE,
                                base::Bind(callback, db_initialized_));
}

void LocalStorageTrackerStore::AddAccessOnDBThread(
    const AccessData& access,
    const base::Callback<void(bool)>& callback) {
  bool result = false;
  if (db_initialized_) {
    result = db_->AddAccess(access);
  }
  RunOnUIThread(base::Bind(callback, result));
}

void LocalStorageTrackerStore::AddApplicationOnDBThread(
    const ApplicationData& application,
    const base::Callback<void(bool)>& callback) {
  bool result = false;
  if (db_initialized_) {
    result = db_->AddApplication(application);
  }
  RunOnUIThread(base::Bind(callback, result));
}

void LocalStorageTrackerStore::AddOriginOnDBThread(
    const OriginData& origin,
    const base::Callback<void(bool)>& callback) {
  bool result = false;
  if (db_initialized_) {
    result = db_->AddOrigin(origin);
  }
  RunOnUIThread(base::Bind(callback, result));
}

void LocalStorageTrackerStore::DeleteApplicationOnDBThread(
    const std::string& app_id,
    const base::Callback<void(bool)>& callback) {
  bool result = false;
  if (db_initialized_) {
    result = db_->DeleteApplication(app_id);
  }
  RunOnUIThread(base::Bind(callback, result));
}

void LocalStorageTrackerStore::DeleteOriginOnDBThread(
    const GURL& url,
    const base::Callback<void(bool)>& callback) {
  bool result = false;
  if (db_initialized_) {
    result = db_->DeleteOrigin(url);
  }
  RunOnUIThread(base::Bind(callback, result));
}

void LocalStorageTrackerStore::GetAccessesOnDBThread(
    const base::Callback<void(bool, const AccessDataList&)>& callback) {
  AccessDataList data_list;
  bool result = false;
  if (db_initialized_) {
    result = db_->GetAccesses(&data_list);
  }
  RunOnUIThread(base::Bind(callback, result, data_list));
}

void LocalStorageTrackerStore::GetApplicationsOnDBThread(
    const base::Callback<void(bool, const ApplicationDataList&)>& callback) {
  ApplicationDataList data_list;
  bool result = false;
  if (db_initialized_) {
    result = db_->GetApplications(&data_list);
  }
  RunOnUIThread(base::Bind(callback, result, data_list));
}

void LocalStorageTrackerStore::RunOnDBThread(const base::Closure& task) {
  db_thread_runner_->PostTask(FROM_HERE, task);
}

void LocalStorageTrackerStore::RunOnUIThread(const base::Closure& task) {
  main_thread_runner_->PostTask(FROM_HERE, task);
}

}  // namespace content
