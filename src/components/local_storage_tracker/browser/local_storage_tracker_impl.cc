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

#include "components/local_storage_tracker/browser/local_storage_tracker_impl.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/lazy_instance.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/task/post_task.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/data_deleter.h"
#include "content/public/browser/renderer_preferences_util.h"
#include "content/public/browser/web_contents.h"
#include "net/base/net_errors.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "net/url_request/url_request.h"
#include "third_party/blink/public/mojom/renderer_preferences.mojom.h"

namespace content {

void LocalStorageTrackerImpl::Initialize(const base::FilePath& data_file_path) {
  if (init_status_ != InitializationStatus::kNone)
    return;
  init_status_ = InitializationStatus::kPending;
  scoped_refptr<base::SingleThreadTaskRunner> main_thread_runner(
      base::ThreadTaskRunnerHandle::Get());
  scoped_refptr<base::SingleThreadTaskRunner> db_thread_runner =
      base::CreateSingleThreadTaskRunner({content::BrowserThread::IO});
  store_.reset(
      new LocalStorageTrackerStore(main_thread_runner, db_thread_runner));
  store_->Initialize(
      data_file_path,
      base::Bind(&LocalStorageTrackerImpl::OnStoreInitialized, this));
}

void LocalStorageTrackerImpl::OnAppInstalled(const std::string& app_id) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (!IsInitialized()) {
    LOG(ERROR) << "LocalStorageTrackerImpl not yet initialized";
    return;
  }
  bool inserted = apps_.insert({app_id, true}).second;
  if (inserted) {
    store_->AddApplication(
        ApplicationData{app_id, true},
        base::Bind(&LocalStorageTrackerImpl::OnStoreModified, this,
                   StoreModificationOperation::kAddApplication));
  }
  VLOG(1) << "OnAppInstalled appID=" << app_id;
}

void LocalStorageTrackerImpl::OnAppRemoved(const std::string& app_id) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  VLOG(1) << "OnAppRemoved appID=" << app_id;
  if (!IsInitialized()) {
    LOG(ERROR) << "LocalStorageTrackerImpl not yet initialized";
  }
  if (apps_.erase(app_id) == 0) {
    return;
  }
  std::set<GURL> origins_to_clear;
  for (auto& origin : origins_) {
    AppsSet& apps = origin.second.apps;
    AppsSet::iterator it_app = apps.find(app_id);
    if (it_app != apps.end()) {
      apps.erase(it_app);
      GURL origin_to_clear = origin.first;
      VLOG(1) << "Origin=" << origin_to_clear << " to be cleared";
      if (apps.size() == 0) {
        origins_to_clear.insert(origin.first);
        store_->DeleteOrigin(
            origin.first,
            base::Bind(&LocalStorageTrackerImpl::OnStoreModified, this,
                       StoreModificationOperation::kDeleteOrigin));
      }
      StartDeleteOriginData(origin_to_clear);
    }
  }
  for (auto origin : origins_to_clear)
    origins_.erase(origin);
  store_->DeleteApplication(
      app_id, base::Bind(&LocalStorageTrackerImpl::OnStoreModified, this,
                         StoreModificationOperation::kDeleteApplication));
}

void LocalStorageTrackerImpl::OnDeleteCompleted(const GURL& origin) {
  VLOG(1) << "On delete completed origin=" << origin;
  OriginToAppsMap::iterator origin_it = origins_.find(origin);
  if (origin_it != origins_.end()) {
    origin_it->second.deletion_in_progress = false;
  }
  data_delete_completions_.remove_if(
      [this, &origin](const DataDeleteCompletion& n) {
        return const_cast<DataDeleteCompletion&>(n).OnDataDeleted(origin);
      });

  VLOG(1) << "data_delete_completions_ size="
          << data_delete_completions_.size();
}

base::WeakPtr<LocalStorageTracker> LocalStorageTrackerImpl::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

void LocalStorageTrackerImpl::OnAccessOrigin(
    const std::string& app_id,
    const GURL& origin,
    base::OnceCallback<void()> callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (!IsInitialized()) {
    LOG(ERROR) << "LocalStorageTrackerImpl not yet initialized";
    std::move(callback).Run();
    return;
  }
  if (origin.is_empty() || !origin.is_valid()) {
    LOG(ERROR) << "LocalStorageTrackerImpl invalid origin value";
    std::move(callback).Run();
    return;
  }
  GURL origin_actual;
  if (origin.SchemeIsFile()) {
    origin_actual = GURL(std::string("file://").append(app_id));
  } else {
    origin_actual = origin;
  }

  AppToStatusMap::iterator it_app = apps_.find(app_id);
  if (it_app == apps_.end()) {
    VLOG(1) << "OnAccessOrigin: adding application, appID=" << app_id;
    it_app = apps_.insert({app_id, false}).first;
    store_->AddApplication(
        {app_id, false},
        base::Bind(&LocalStorageTrackerImpl::OnStoreModified, this,
                   StoreModificationOperation::kAddApplication));
  }

  VLOG(1) << "OnAccessOrigin: checking main domain";
  AppLinkVerifyResult verify_result =
      VerifyOriginAppLink(origin_actual, app_id);

  if (verify_result != AppLinkVerifyResult::kExist) {
    VLOG(1) << "OnAccessOrigin: checking subdomains";
    std::set<GURL> sub_origins = GetSubOrigins(origin_actual);
    for (const auto& sub_origin : sub_origins) {
      VerifyOriginAppLink(sub_origin, app_id);
    }
  }
  std::move(callback).Run();
}

std::set<GURL> LocalStorageTrackerImpl::GetSubOrigins(const GURL& origin) {
  std::set<GURL> sub_origins;
  if (!origin.SchemeIsHTTPOrHTTPS()) {
    return sub_origins;
  }
  std::string domain_registry =
      net::registry_controlled_domains::GetDomainAndRegistry(
          origin, net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);
  std::string host = origin.host();
  if (domain_registry.empty() || domain_registry == host) {
    return sub_origins;
  }
  std::size_t domain_registry_length = domain_registry.length();
  std::size_t host_length = host.length();
  if (host_length < domain_registry_length) {
    LOG(ERROR) << "Domain registry length exceeds host length";
    return sub_origins;
  }
  std::size_t max_pos = host_length - domain_registry_length;
  std::size_t pos = host.find('.', 0);
  GURL sub_origin = origin;
  while (pos != std::string::npos && pos < max_pos) {
    url::Replacements<char> replacements;
    std::size_t subdomain_pos = pos + 1;
    replacements.SetHost(
        host.c_str(),
        url::Component(subdomain_pos, host.length() - subdomain_pos));
    sub_origins.insert(sub_origin.ReplaceComponents(replacements));
    pos = host.find('.', subdomain_pos);
  }
  return sub_origins;
}

void LocalStorageTrackerImpl::OnAccessesLoaded(
    bool success,
    const AccessDataList& access_list) {
  VLOG(1) << "Accesses loaded successfully = " << success;
  if (!success) {
    init_status_ = InitializationStatus::kFailed;
    OnInitializeFailed();
    return;
  }
  for (auto& item : access_list) {
    OriginToAppsMap::iterator it = origins_.find(item.origin_);
    if (it == origins_.end()) {
      it = origins_.insert({item.origin_, {}}).first;
    }
    it->second.apps.insert(item.app_id_);
  }
  init_status_ = InitializationStatus::kSucceeded;
  OnInitializeSucceeded();
}

void LocalStorageTrackerImpl::OnApplicationsLoaded(
    bool success,
    const ApplicationDataList& apps_list) {
  VLOG(1) << "Applications loaded successfully = " << success;
  if (success) {
    store_->GetAccesses(
        base::Bind(&LocalStorageTrackerImpl::OnAccessesLoaded, this));
    for (auto& item : apps_list) {
      apps_[item.app_id_] = item.installed_;
    }
  } else {
    init_status_ = InitializationStatus::kFailed;
    OnInitializeFailed();
  }
}

void LocalStorageTrackerImpl::OnStoreModified(
    StoreModificationOperation modification_operation,
    bool success) {
  VLOG(1) << "Store modification=" << static_cast<int>(modification_operation)
          << " completed; Successfully = " << success;
}

void LocalStorageTrackerImpl::OnStoreInitialized(bool success) {
  VLOG(1) << "Store initialized success=" << success;
  if (success) {
    store_->GetApplications(
        base::Bind(&LocalStorageTrackerImpl::OnApplicationsLoaded, this));
  } else {
    init_status_ = InitializationStatus::kFailed;
    OnInitializeFailed();
  }
}

void LocalStorageTrackerImpl::OnInitializeFailed() {}

void LocalStorageTrackerImpl::OnInitializeSucceeded() {}

bool LocalStorageTrackerImpl::IsInitialized() {
  return init_status_ == InitializationStatus::kSucceeded;
}

bool LocalStorageTrackerImpl::IsHTTPOrHTTPSOriginUniqueForHost(
    const std::string& host) {
  int host_origin_count = 0;
  for (auto& origin : origins_) {
    if (origin.first.has_host() && origin.first.SchemeIsHTTPOrHTTPS() &&
        origin.first.host() == host) {
      ++host_origin_count;
      if (host_origin_count > 1) {
        return false;
      }
    }
  }
  return true;
}

void LocalStorageTrackerImpl::StartDeleteOriginData(const GURL& origin,
                                                    bool delete_cookies) {
  VLOG(1) << "Start deleting origin=" << origin
          << " delete_cookies=" << delete_cookies;

  OriginToAppsMap::iterator iter = origins_.find(origin);
  base::OnceClosure callback;
  if (iter != origins_.end()) {
    iter->second.deletion_in_progress = true;
    callback = base::Bind(&LocalStorageTrackerImpl::OnDeleteCompleted,
                          weak_ptr_factory_.GetWeakPtr(), origin);
  } else {
    callback = base::BindOnce(base::DoNothing::Once());
  }

  if (GetDataDeleter())
    GetDataDeleter()->StartDeleting(origin, delete_cookies,
                                    std::move(callback));
}

LocalStorageTrackerImpl::AppLinkVerifyResult
LocalStorageTrackerImpl::VerifyOriginAppLink(const GURL& origin,
                                             const std::string& app_id) {
  OriginToAppsMap::iterator it_origin = origins_.find(origin);
  if (it_origin == origins_.end()) {
    VLOG(1) << "VerifyOriginAppLink: adding origin, origin=" << origin;
    it_origin = origins_.insert({origin, OriginData()}).first;
    store_->AddOrigin(
        {origin}, base::Bind(&LocalStorageTrackerImpl::OnStoreModified, this,
                             StoreModificationOperation::kAddOrigin));
  }
  AppLinkVerifyResult result = AppLinkVerifyResult::kExist;
  OriginData& data = it_origin->second;
  if (data.apps.find(app_id) == data.apps.end()) {
    VLOG(1) << "VerifyOriginAppLink Add appID=" << app_id
            << " to origin=" << origin;
    data.apps.insert(app_id);
    store_->AddAccess({app_id, origin},
                      base::Bind(&LocalStorageTrackerImpl::OnStoreModified,
                                 this, StoreModificationOperation::kAddAccess));
    result = data.apps.size() == 1 ? AppLinkVerifyResult::kAddedNewOriginEntry
                                   : AppLinkVerifyResult::kAdded;
  }
  if (data.deletion_in_progress) {
    result = AppLinkVerifyResult::kDeletionInProgress;
  }
  return result;
}

LocalStorageTrackerImpl::DataDeleteCompletion::DataDeleteCompletion(
    std::set<GURL> origins,
    base::OnceClosure callback)
    : origins_(origins), callback_(std::move(callback)) {}

bool LocalStorageTrackerImpl::DataDeleteCompletion::OnDataDeleted(
    const GURL& origin) {
  VLOG(1) << "OnDataDeleted, origin=" << origin;
  if (origins_.erase(origin) == 1 && origins_.empty()) {
    VLOG(1) << "origins is empty, call callback";
    std::move(callback_).Run();
  }
  return origins_.empty();
}
}  // namespace content
