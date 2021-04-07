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

#include "extensions/shell/neva/webos_language_listener.h"

#include "base/command_line.h"
#include "base/json/json_reader.h"
#include "base/values.h"
#include "extensions/common/switches.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "neva/pal_service/webos/luna/luna_names.h"
#include "third_party/blink/public/mojom/renderer_preferences.mojom.h"

namespace {

const char kGetLanguageMethod[] =
    "palm://com.webos.settingsservice/getSystemSettings";
const char kGetLanguageRequest[] =
    R"JSON({"keys":["localeInfo"], "subscribe":true})JSON";

std::initializer_list<base::StringPiece> kUILanguagePath = {
    "settings", "localeInfo", "locales", "UI"};

}  // namespace

namespace webos {

LanguageListener::LanguageListener(content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents), weak_factory_(this) {
  pal::luna::Client::Params params;
  params.bus = pal::luna::Bus::Private;
  luna_client_ = pal::luna::GetSharedClient(params);

  if (luna_client_ && luna_client_->IsInitialized()) {
    luna_client_->Subscribe(
        std::string(kGetLanguageMethod),
        std::string(kGetLanguageRequest),
        base::BindRepeating(&LanguageListener::OnResponse,
                            weak_factory_.GetWeakPtr()));
  }
}

void LanguageListener::OnResponse(pal::luna::Client::ResponseStatus,
                                  unsigned,
                                  const std::string& json) {
  content::WebContents* contents = web_contents();
  if (!contents)
    return;

  base::Optional<base::Value> root(base::JSONReader::Read(json));
  if (!root || !root->is_dict())
    return;

  const base::Value* language = root->FindPath(kUILanguagePath);
  if (!language)
    return;

  auto* rendererPrefs(contents->GetMutableRendererPrefs());
  if (!rendererPrefs->accept_languages.compare(language->GetString()))
    return;

  rendererPrefs->accept_languages = language->GetString();
  contents->SyncRendererPrefs();
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(LanguageListener)

}  // namespace webos
