// Copyright 2018-2019 LG Electronics, Inc.
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

#ifndef BASE_TRACE_EVENT_NEVA_MEMORY_TRACE_MEMORY_TRACE_PROVIDER_H_
#define BASE_TRACE_EVENT_NEVA_MEMORY_TRACE_MEMORY_TRACE_PROVIDER_H_

#include <string>

#include "base/base_export.h"
#include "base/macros.h"

namespace base {
namespace trace_event {
namespace neva {

#define ConvertKBtoMB(kb) ((kb) / 1024)

static constexpr int KB = 1024;

// The contract interface that memory trace providers must implement.
class BASE_EXPORT MemoryTraceProvider {
 public:
  virtual ~MemoryTraceProvider() {}

  virtual bool OnMemoryTrace() = 0;

  virtual std::string GetCSVHeader() = 0;

 protected:
  MemoryTraceProvider() {}

  DISALLOW_COPY_AND_ASSIGN(MemoryTraceProvider);
};

}  // namespace neva
}  // namespace trace_event
}  // namespace base

#endif  // BASE_TRACE_EVENT_NEVA_MEMORY_TRACE_MEMORY_TRACE_PROVIDER_H_
