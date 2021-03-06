// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_SERVICES_ASSISTANT_PUBLIC_SHARED_UTILS_H_
#define CHROMEOS_SERVICES_ASSISTANT_PUBLIC_SHARED_UTILS_H_

#include <string>

#include "base/component_export.h"

namespace chromeos {
namespace assistant {

// Models status of an app.
enum class AppStatus {
  kUnknown,
  kAvailable,
  kUnavailable,
  kVersionMismatch,
  kDisabled,
};

// Models an Android app.
struct COMPONENT_EXPORT(ASSISTANT_SERVICE_SHARED) AndroidAppInfo {
  AndroidAppInfo();
  AndroidAppInfo(const AndroidAppInfo& suggestion);
  AndroidAppInfo& operator=(const AndroidAppInfo&);
  AndroidAppInfo(AndroidAppInfo&& suggestion);
  AndroidAppInfo& operator=(AndroidAppInfo&&);
  ~AndroidAppInfo();

  // Unique name to identify a specific app.
  std::string package_name;

  // Version number of the app.
  int version{0};

  // Localized app name.
  std::string localized_app_name;

  // Intent data to operate on.
  std::string intent;

  // Status of the app.
  AppStatus status{AppStatus::kUnknown};

  // The general action to be performed, such as ACTION_VIEW, ACTION_MAIN, etc.
  std::string action;
};

// Models an Interaction.
struct COMPONENT_EXPORT(ASSISTANT_SERVICE_SHARED) InteractionInfo {
  const int interaction_id;
  const std::string user_id;
};

}  // namespace assistant
}  // namespace chromeos

#endif  // CHROMEOS_SERVICES_ASSISTANT_PUBLIC_SHARED_UTILS_H_
