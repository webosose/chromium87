// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_NEARBY_SHARING_PAYLOAD_TRACKER_H_
#define CHROME_BROWSER_NEARBY_SHARING_PAYLOAD_TRACKER_H_

#include "base/callback_forward.h"
#include "base/containers/flat_map.h"
#include "base/time/time.h"
#include "chrome/browser/nearby_sharing/attachment_info.h"
#include "chrome/browser/nearby_sharing/nearby_connections_manager.h"
#include "chrome/browser/nearby_sharing/share_target.h"
#include "chrome/browser/nearby_sharing/transfer_metadata.h"
#include "chrome/services/sharing/public/mojom/nearby_connections_types.mojom.h"

class PayloadTracker : public NearbyConnectionsManager::PayloadStatusListener {
 public:
  PayloadTracker(
      const ShareTarget& share_target,
      const base::flat_map<int64_t, AttachmentInfo>& attachment_info_map,
      base::RepeatingCallback<void(ShareTarget, TransferMetadata)>
          update_callback);
  ~PayloadTracker() override;

  // NearbyConnectionsManager::PayloadStatusListener:
  void OnStatusUpdate(PayloadTransferUpdatePtr update) override;

 private:
  struct State {
    explicit State(int64_t total_size) : total_size(total_size) {}
    ~State() = default;

    uint64_t amount_downloaded = 0;
    const uint64_t total_size;
    location::nearby::connections::mojom::PayloadStatus status =
        location::nearby::connections::mojom::PayloadStatus::kInProgress;
  };

  void OnTransferUpdate();

  bool IsComplete();
  bool IsCancelled();
  bool HasFailed();

  double CalculateProgressPercent();

  ShareTarget share_target_;
  base::RepeatingCallback<void(ShareTarget, TransferMetadata)> update_callback_;

  // Map of payload id to state of payload.
  std::map<int64_t, State> payload_state_;

  uint64_t total_download_size_;

  int last_update_progress_ = 0;
  base::Time last_update_timestamp_;
};

#endif  // CHROME_BROWSER_NEARBY_SHARING_PAYLOAD_TRACKER_H_
