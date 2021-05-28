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

#ifndef CONTENT_PUBLIC_BROWSER_DATA_DELETER_H_
#define CONTENT_PUBLIC_BROWSER_DATA_DELETER_H_

#include <set>

#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "content/common/content_export.h"
#include "url/origin.h"

namespace content {

class DataDeleter {
 public:
  DataDeleter() = default;
  DataDeleter(const DataDeleter&) = delete;
  DataDeleter& operator=(const DataDeleter&) = delete;

  typedef std::set<GURL> Origins;
  using CompletionCallback = base::OnceCallback<void()>;

  virtual void StartDeleting(const GURL& origin,
                             bool delete_cookies,
                             CompletionCallback callback) = 0;

  struct DeletionContext : public base::RefCounted<DeletionContext> {
    DeletionContext(const Origins& origins, CompletionCallback& callback);
    CompletionCallback callback_;
    std::set<GURL> origins_;
  };

  virtual void OnDeleteCompleted(const GURL& origin,
                                 scoped_refptr<DeletionContext> context) = 0;
};

CONTENT_EXPORT void SetDataDeleter(DataDeleter*);
CONTENT_EXPORT DataDeleter* GetDataDeleter();

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_DATA_DELETER_H_
