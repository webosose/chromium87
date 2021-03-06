// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_COMPONENTS_CDM_FACTORY_DAEMON_MOJOM_DECRYPT_CONFIG_MOJOM_TRAITS_H_
#define CHROMEOS_COMPONENTS_CDM_FACTORY_DAEMON_MOJOM_DECRYPT_CONFIG_MOJOM_TRAITS_H_

#include "base/component_export.h"
#include "media/base/decryptor.h"
#include "media/base/encryption_pattern.h"
#include "media/base/encryption_scheme.h"
#include "mojo/public/cpp/bindings/enum_traits.h"
#include "mojo/public/cpp/bindings/struct_traits.h"

// This needs to be included last so it can pick up the media::Decryptor::Status
// declaration. It's not possible to forward declare an inner class.
#include "chromeos/components/cdm_factory_daemon/mojom/content_decryption_module.mojom.h"

namespace mojo {

template <>
struct COMPONENT_EXPORT(CHROMEOS_CDM_MOJOM)
    EnumTraits<chromeos::cdm::mojom::DecryptStatus,
               ::media::Decryptor::Status> {
  static chromeos::cdm::mojom::DecryptStatus ToMojom(
      ::media::Decryptor::Status input);

  static bool FromMojom(chromeos::cdm::mojom::DecryptStatus input,
                        ::media::Decryptor::Status* output);
};

template <>
struct COMPONENT_EXPORT(CHROMEOS_CDM_MOJOM)
    EnumTraits<chromeos::cdm::mojom::EncryptionScheme,
               ::media::EncryptionScheme> {
  static chromeos::cdm::mojom::EncryptionScheme ToMojom(
      ::media::EncryptionScheme input);

  static bool FromMojom(chromeos::cdm::mojom::EncryptionScheme input,
                        ::media::EncryptionScheme* output);
};

template <>
struct COMPONENT_EXPORT(CHROMEOS_CDM_MOJOM)
    StructTraits<chromeos::cdm::mojom::EncryptionPatternDataView,
                 ::media::EncryptionPattern> {
  static uint32_t crypt_byte_block(const ::media::EncryptionPattern& input) {
    return input.crypt_byte_block();
  }

  static uint32_t skip_byte_block(const ::media::EncryptionPattern& input) {
    return input.skip_byte_block();
  }

  static bool Read(chromeos::cdm::mojom::EncryptionPatternDataView input,
                   ::media::EncryptionPattern* output);
};

}  // namespace mojo

#endif  // CHROMEOS_COMPONENTS_CDM_FACTORY_DAEMON_MOJOM_DECRYPT_CONFIG_MOJOM_TRAITS_H_