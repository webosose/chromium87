// Copyright 2019 LG Electronics, Inc.
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

#ifndef EXTENSIONS_SHELL_NEVA_WEBOS_LANGUAGE_LISTENER_H_
#define EXTENSIONS_SHELL_NEVA_WEBOS_LANGUAGE_LISTENER_H_

#include "base/memory/weak_ptr.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "neva/pal_service/webos/luna/luna_client.h"

namespace webos {

class LanguageListener : public content::WebContentsUserData<LanguageListener>,
                         public content::WebContentsObserver {
 private:
  friend class content::WebContentsUserData<LanguageListener>;

  explicit LanguageListener(content::WebContents* web_contents);

  void OnResponse(pal::luna::Client::ResponseStatus,
                  unsigned token,
                  const std::string& json);

  std::shared_ptr<pal::luna::Client> luna_client_;
  base::WeakPtrFactory<LanguageListener> weak_factory_;

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace webos

#endif  // EXTENSIONS_SHELL_NEVA_WEBOS_LANGUAGE_LISTENER_H_
