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

#include "base/logging_pmlog_provider.h"

#include <glib.h>

#include "base/json/string_escape.h"
#include "base/memory/singleton.h"
#include "base/strings/string_piece.h"
#include "base/threading/platform_thread.h"

namespace logging {

PmLogProvider::PmLogProvider() = default;

PmLogProvider* PmLogProvider::GetInstance() {
  return base::Singleton<
      PmLogProvider, base::StaticMemorySingletonTraits<PmLogProvider>>::get();
}

void PmLogProvider::Initialize(const char* context_name) {
  PmLogProvider* provider = PmLogProvider::GetInstance();

  logging::LoggingSettings settings;
  logging::InitLogging(settings);
  logging::SetLogItems(false /* Process ID */, false /* Thread ID */,
                       false /* Timestamp */, false /* Tick count */);

  provider->Register(context_name);

  // Register our message handler with logging.
  SetLogMessageHandler(LogMessage);
}

void PmLogProvider::Register(const char* context_name) {
  CHECK(pmlog_context_ == 0);
  PmLogGetContext(context_name, &pmlog_context_);
}

bool PmLogProvider::LogMessage(logging::LogSeverity severity,
                               const char* file,
                               int line,
                               size_t message_start,
                               const std::string& message) {
  PmLogProvider* provider = GetInstance();
  if (provider == NULL)
    return false;

#define FOR_LEVELS_CALL()               \
  if (severity >= 0) {                  \
    switch (severity) {                 \
      case LOG_INFO:                    \
        PMLOG_CALL(Debug, NULL);        \
        break;                          \
      case LOG_WARNING:                 \
        PMLOG_CALL(Warning, "WARNING"); \
        break;                          \
      case LOG_ERROR:                   \
        PMLOG_CALL(Error, "ERROR");     \
        break;                          \
      case LOG_FATAL:                   \
        PMLOG_CALL(Critical, "FATAL");  \
        break;                          \
    }                                   \
  } else {                              \
    PMLOG_CALL(Debug, NULL);            \
  }

#define PMLOG_CALL(level_suffix, msgid)                                    \
  if (!PmLogIsEnabled(provider->GetContext(), kPmLogLevel_##level_suffix)) \
    return false;

  FOR_LEVELS_CALL();

#undef PMLOG_CALL

  std::string escaped_string = base::GetQuotedJSONString(
      base::StringPiece(message).substr(message_start));
  base::StringPiece file_tail(file);
  if (file_tail.size() > 20)
    file_tail.remove_prefix(file_tail.size() - 20);

  const size_t kMaxLogLength = 896;

#define PMLOG_CALL(level_suffix, msgid)                                     \
  PmLogMsg(provider->GetContext(), level_suffix, msgid, 0,                  \
           "%d:%d %d[%s:%d] %.*s", getpid(),                                \
           base::PlatformThread::CurrentId(), severity, file_tail.data(),   \
           line, std::min(escaped_string.length() - offset, kMaxLogLength), \
           escaped_string.c_str() + offset)

  for (size_t offset = 0; offset < escaped_string.length();
       offset += kMaxLogLength) {
    FOR_LEVELS_CALL();
  }

#undef PMLOG_CALL

  // We keep regular logs working too.
  return false;
}

}  // namespace logging
