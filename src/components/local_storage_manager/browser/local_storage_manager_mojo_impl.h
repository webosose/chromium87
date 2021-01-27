// Copyright (c) 2020 LG Electronics, Inc.
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

#ifndef COMPONENTS_LOCAL_STORAGE_MANAGER_BROWSER_LOCAL_STORAGE_MANAGER_MOJO_IMPL_H_
#define COMPONENTS_LOCAL_STORAGE_MANAGER_BROWSER_LOCAL_STORAGE_MANAGER_MOJO_IMPL_H_

#include "components/local_storage_manager/public/local_storage_manager.h"
#include "components/local_storage_manager/public/mojom/local_storage_manager.mojom.h"
#include "content/browser/renderer_host/render_frame_host_impl.h"

namespace content {
class LocalStorageManagerMojoImpl : local_storage::mojom::LocalStorageManager {
 public:
  explicit LocalStorageManagerMojoImpl(
      mojo::PendingReceiver<local_storage::mojom::LocalStorageManager> receiver)
      : receiver_(this, std::move(receiver)) {}

  void SaveUrl(const std::string& application_id,
               const std::string& url,
               SaveUrlCallback callback) override {
    std::move(callback).Run();
    auto lsm = content::LocalStorageManager::Create().release();
    if (lsm) {
      VLOG(1) << "LocalStorageManager  SaveUrl appID=" << application_id
              << " url=" << url;
      lsm->OnAccessOrigin(application_id, GURL(url), base::BindOnce([] {}));
    }
  }

 private:
  mojo::Receiver<local_storage::mojom::LocalStorageManager> receiver_;

  DISALLOW_COPY_AND_ASSIGN(LocalStorageManagerMojoImpl);
};
}  // namespace content

#endif  // COMPONENTS_LOCAL_STORAGE_MANAGER_BROWSER_LOCAL_STORAGE_MANAGER_MOJO_IMPL_H_
