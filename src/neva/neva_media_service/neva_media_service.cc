// Copyright 2019-2020 LG Electronics, Inc.
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

#include "neva/neva_media_service/neva_media_service.h"

#include <map>
#include <memory>
#include <string>

#include "base/component_export.h"
#include "base/memory/ref_counted.h"
#include "base/no_destructor.h"
#include "base/threading/sequence_local_storage_slot.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "neva/neva_media_service/media_service_provider_impl.h"
#include "neva/neva_media_service/public/mojom/media_player.mojom.h"
#include "neva/neva_media_service/public/mojom/neva_media_service.mojom.h"
#include "neva/neva_media_service/services/mojo_media_platform_api_service.h"
#include "neva/neva_media_service/services/mojo_media_player_service.h"

namespace neva_media {

class COMPONENT_EXPORT(NEVA_MEDIA_SERVICE) NevaMediaService
    : public mojom::NevaMediaService {
 public:
  explicit NevaMediaService(
      mojo::PendingReceiver<mojom::NevaMediaService> receiver);
  ~NevaMediaService() override;

 private:
  void BindMediaServiceProvider(
      mojo::PendingReceiver<mojom::MediaServiceProvider> receiver) override;

  std::unique_ptr<MediaServiceProvider> media_player_provider_;

  mojo::Receiver<mojom::NevaMediaService> receiver_;
};

NevaMediaService::NevaMediaService(
    mojo::PendingReceiver<mojom::NevaMediaService> receiver)
    : receiver_(this, std::move(receiver)) {}

NevaMediaService::~NevaMediaService() {}

void NevaMediaService::BindMediaServiceProvider(
    mojo::PendingReceiver<mojom::MediaServiceProvider> receiver) {
  if (!media_player_provider_) {
    media_player_provider_ = std::make_unique<MediaServiceProvider>();
  }
  media_player_provider_->AddBinding(std::move(receiver));
}

namespace {

std::unique_ptr<NevaMediaService> CreateNevaMediaService(
    mojo::PendingReceiver<mojom::NevaMediaService> receiver) {
  return std::make_unique<NevaMediaService>(std::move(receiver));
}

void BindNevaMediaServiceReceiver(
    mojo::PendingReceiver<mojom::NevaMediaService> receiver) {
  // Bind the lifetime of the service instance to that of the sequence it's
  // running on.
  static base::NoDestructor<base::SequenceLocalStorageSlot<
      std::unique_ptr<neva_media::NevaMediaService>>>
      service_slot;
  auto& service = service_slot->GetOrCreateValue();

  // This function should only be called once during the lifetime of the
  // service's bound sequence.
  DCHECK(!service);

  service = CreateNevaMediaService(std::move(receiver));
}

}  // anonymous namespace

neva_media::mojom::NevaMediaService& GetNevaMediaService() {
  using namespace content;
  static base::NoDestructor<
      base::SequenceLocalStorageSlot<mojo::Remote<mojom::NevaMediaService>>>
      remote_slot;
  mojo::Remote<mojom::NevaMediaService>& remote =
      remote_slot->GetOrCreateValue();
  if (!remote) {
    // To be sure that NevaMediaService gets created after main message loop
    // has been started
    base::PostTask(FROM_HERE, {content::BrowserThread::UI},
                   base::BindOnce(&BindNevaMediaServiceReceiver,
                                  remote.BindNewPipeAndPassReceiver()));
  }
  return *remote.get();
}

}  // namespace neva_media
