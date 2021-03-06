// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/download/android/mixed_content_download_infobar_delegate.h"

#include <memory>
#include <utility>

#include "base/memory/ptr_util.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/android/android_theme_resources.h"
#include "chrome/browser/infobars/infobar_service.h"
#include "chrome/grit/generated_resources.h"
#include "components/download/public/common/download_item.h"
#include "components/infobars/core/infobar.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/strings/grit/ui_strings.h"

using MixedContentStatus = download::DownloadItem::MixedContentStatus;

// static
void MixedContentDownloadInfoBarDelegate::Create(
    InfoBarService* infobar_service,
    const base::FilePath& basename,
    download::DownloadItem::MixedContentStatus mixed_content_status,
    ResultCallback callback) {
  infobar_service->AddInfoBar(infobar_service->CreateConfirmInfoBar(
      base::WrapUnique(new MixedContentDownloadInfoBarDelegate(
          basename, mixed_content_status, std::move(callback)))));
}

MixedContentDownloadInfoBarDelegate::MixedContentDownloadInfoBarDelegate(
    const base::FilePath& basename,
    download::DownloadItem::MixedContentStatus mixed_content_status,
    ResultCallback callback)
    : mixed_content_status_(mixed_content_status),
      callback_(std::move(callback)) {
  message_text_ =
      l10n_util::GetStringFUTF16(IDS_PROMPT_CONFIRM_MIXED_CONTENT_DOWNLOAD,
                                 base::UTF8ToUTF16(basename.value()));
}

MixedContentDownloadInfoBarDelegate::~MixedContentDownloadInfoBarDelegate() {}

infobars::InfoBarDelegate::InfoBarIdentifier
MixedContentDownloadInfoBarDelegate::GetIdentifier() const {
  return MIXED_CONTENT_DOWNLOAD_INFOBAR_DELEGATE_ANDROID;
}

int MixedContentDownloadInfoBarDelegate::GetIconId() const {
  return IDR_ANDROID_INFOBAR_WARNING;
}

bool MixedContentDownloadInfoBarDelegate::ShouldExpire(
    const NavigationDetails& details) const {
  return false;
}

void MixedContentDownloadInfoBarDelegate::InfoBarDismissed() {
  PostReply(false);
}

base::string16 MixedContentDownloadInfoBarDelegate::GetMessageText() const {
  return message_text_;
}

base::string16 MixedContentDownloadInfoBarDelegate::GetButtonLabel(
    InfoBarButton button) const {
  if (mixed_content_status_ == MixedContentStatus::WARN) {
    return l10n_util::GetStringUTF16(
        button == BUTTON_OK ? IDS_CONFIRM_DOWNLOAD : IDS_DISCARD_DOWNLOAD);
  }

  DCHECK_EQ(mixed_content_status_, MixedContentStatus::BLOCK);
  // Default button is Discard when blocking.
  return l10n_util::GetStringUTF16(button == BUTTON_OK ? IDS_DISCARD_DOWNLOAD
                                                       : IDS_CONFIRM_DOWNLOAD);
}

bool MixedContentDownloadInfoBarDelegate::Accept() {
  if (mixed_content_status_ == MixedContentStatus::WARN) {
    PostReply(true);
    return true;
  }

  DCHECK_EQ(mixed_content_status_, MixedContentStatus::BLOCK);
  // Default button is Discard when blocking.
  PostReply(false);
  return true;
}

bool MixedContentDownloadInfoBarDelegate::Cancel() {
  if (mixed_content_status_ == MixedContentStatus::WARN) {
    PostReply(false);
    return true;
  }

  DCHECK_EQ(mixed_content_status_, MixedContentStatus::BLOCK);
  // Cancel button is Keep when blocking.
  PostReply(true);
  return true;
}

void MixedContentDownloadInfoBarDelegate::PostReply(bool should_download) {
  DCHECK(callback_);
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback_), should_download));
}
