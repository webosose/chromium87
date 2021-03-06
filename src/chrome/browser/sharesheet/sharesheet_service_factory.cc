// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sharesheet/sharesheet_service_factory.h"

#include <memory>

#include "chrome/browser/apps/app_service/app_service_proxy_factory.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sharesheet/sharesheet_service.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

#if defined(OS_CHROMEOS)
#include "chrome/browser/chromeos/profiles/profile_helper.h"
#endif  // OS_CHROMEOS

namespace sharesheet {

// static
SharesheetService* SharesheetServiceFactory::GetForProfile(Profile* profile) {
  // TODO: decide the right behaviour in incognito (non-guest) profiles:
  //   - return nullptr (means we need to null check the service at call sites
  //     OR ensure it's never accessed from an incognito profile),
  //   - return the service attached to the Profile that the incognito profile
  //     is branched from (i.e. "inherit" the parent service),
  //   - return a temporary service just for the incognito session (probably
  //     the least sensible option).
  return static_cast<SharesheetService*>(
      SharesheetServiceFactory::GetInstance()->GetServiceForBrowserContext(
          profile, true /* create */));
}

// static
SharesheetServiceFactory* SharesheetServiceFactory::GetInstance() {
  return base::Singleton<SharesheetServiceFactory>::get();
}

SharesheetServiceFactory::SharesheetServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "SharesheetService",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(apps::AppServiceProxyFactory::GetInstance());
}

SharesheetServiceFactory::~SharesheetServiceFactory() = default;

KeyedService* SharesheetServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new SharesheetService(Profile::FromBrowserContext(context));
}

content::BrowserContext* SharesheetServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  Profile* const profile = Profile::FromBrowserContext(context);
  if (!profile || profile->IsSystemProfile()) {
    return nullptr;
  }

#if defined(OS_CHROMEOS)
  if (chromeos::ProfileHelper::IsSigninProfile(profile)) {
    return nullptr;
  }

  // We allow sharing in guest mode.
  if (profile->IsGuestSession()) {
    return chrome::GetBrowserContextOwnInstanceInIncognito(context);
  }
#endif  // OS_CHROMEOS

  return BrowserContextKeyedServiceFactory::GetBrowserContextToUse(context);
}

bool SharesheetServiceFactory::ServiceIsCreatedWithBrowserContext() const {
  return true;
}

}  // namespace sharesheet
