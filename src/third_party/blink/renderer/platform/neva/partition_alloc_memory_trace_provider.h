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

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_NEVA_PARTITION_ALLOC_MEMORY_TRACE_PROVIDER_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_NEVA_PARTITION_ALLOC_MEMORY_TRACE_PROVIDER_H_

#include "base/trace_event/neva/memory_trace/memory_trace_provider.h"
#include "third_party/blink/public/platform/web_common.h"
#include "third_party/blink/renderer/platform/wtf/noncopyable.h"

namespace blink {
namespace neva {

class BLINK_PLATFORM_EXPORT PartitionAllocMemoryTraceProvider final
    : public base::trace_event::neva::MemoryTraceProvider {
  WTF_MAKE_NONCOPYABLE(PartitionAllocMemoryTraceProvider);

 public:
  static PartitionAllocMemoryTraceProvider* Instance();
  ~PartitionAllocMemoryTraceProvider() override {}

  // MemoryTraceProvider implementation.
  bool OnMemoryTrace() override;

  std::string GetCSVHeader() override;

 private:
    PartitionAllocMemoryTraceProvider() {}
};

}  // namespace neva
}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_NEVA_PARTITION_ALLOC_MEMORY_TRACE_PROVIDER_H_
