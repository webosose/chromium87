// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PRIVACY_BUDGET_SAMPLED_SURFACE_TRACKER_H_
#define CHROME_BROWSER_PRIVACY_BUDGET_SAMPLED_SURFACE_TRACKER_H_

#include <map>

#include "base/containers/flat_set.h"
#include "base/time/time.h"

class SampledSurfaceTracker {
 public:
  // Maximum number of surfaces that this class can track. Prevents unbounded
  // memory growth.
  static constexpr unsigned kMaxTrackedSurfaces = 1000;

  SampledSurfaceTracker();
  ~SampledSurfaceTracker();

  bool ShouldRecord(uint64_t source_id, uint64_t surface);

  void Reset();

 private:
  using HashKey = uint64_t;
  // We use std::map since it makes it fast to remove the minimum.
  std::map<HashKey, base::flat_set<uint64_t>> surfaces_;
  uint64_t seed_;
};

#endif  // CHROME_BROWSER_PRIVACY_BUDGET_SAMPLED_SURFACE_TRACKER_H_
