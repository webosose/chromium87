// Copyright (c) 2019 LG Electronics, Inc.
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

#ifndef BASE_LOGGING_PMLOG_PROVIDER_H_
#define BASE_LOGGING_PMLOG_PROVIDER_H_

#include <PmLogLib.h>
#include <string>

#include "base/base_export.h"
#include "base/logging.h"

namespace base {
template <typename Type>
struct StaticMemorySingletonTraits;
}  // namespace base

namespace logging {

class BASE_EXPORT PmLogProvider {
 public:
  PmLogProvider(const PmLogProvider&) = delete;
  PmLogProvider& operator=(const PmLogProvider&) = delete;
  static PmLogProvider* GetInstance();
  static void Initialize(const char* context_name);
  static bool LogMessage(logging::LogSeverity severity,
                         const char* file,
                         int line,
                         size_t message_start,
                         const std::string& str);

  PmLogContext GetContext() const { return pmlog_context_; }

 private:
  PmLogProvider();
  void Register(const char* context_name);

  PmLogContext pmlog_context_ = 0;

  friend struct base::StaticMemorySingletonTraits<PmLogProvider>;
};

}  // namespace logging

#endif  // BASE_LOGGING_PMLOG_PROVIDER_H_
