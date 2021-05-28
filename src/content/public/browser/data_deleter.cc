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

#include "content/public/browser/data_deleter.h"

#include "base/lazy_instance.h"
#include "base/task/post_task.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/storage_partition.h"

static content::DataDeleter* g_data_deleter = nullptr;

namespace content {

DataDeleter::DeletionContext::DeletionContext(const Origins& origins,
                                              CompletionCallback& callback)
    : origins_(origins) {
  callback_ = std::move(callback);
}

void SetDataDeleter(DataDeleter* p) {
  g_data_deleter = p;
}

DataDeleter* GetDataDeleter() {
  return g_data_deleter;
}

}  // namespace content
