// Copyright 2025 Atlas Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license.

#ifndef ATLAS_BROWSER_FEATURES_PRIVACY_TRACKER_BLOCKER_H_
#define ATLAS_BROWSER_FEATURES_PRIVACY_TRACKER_BLOCKER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/containers/flat_set.h"
#include "base/memory/singleton.h"

class GURL;

namespace atlas {

// TrackerBlocker blocks known tracking domains and scripts
class TrackerBlocker {
 public:
  static TrackerBlocker* GetInstance();

  TrackerBlocker(const TrackerBlocker&) = delete;
  TrackerBlocker& operator=(const TrackerBlocker&) = delete;

  // Initialize the blocker with default lists
  void Initialize();

  // Check if a URL should be blocked
  bool ShouldBlock(const GURL& url) const;

  // Check if a URL is a known tracker
  bool IsTracker(const GURL& url) const;

  // Add a domain to the block list
  void AddBlockedDomain(const std::string& domain);

  // Remove a domain from the block list
  void RemoveBlockedDomain(const std::string& domain);

  // Get blocked count for statistics
  int GetBlockedCount() const { return blocked_count_; }

  // Reset statistics
  void ResetStatistics() { blocked_count_ = 0; }

 private:
  friend struct base::DefaultSingletonTraits<TrackerBlocker>;

  TrackerBlocker();
  ~TrackerBlocker();

  // Load default tracking domains
  void LoadDefaultBlockList();

  // Blocked domains (hashed for fast lookup)
  base::flat_set<std::string> blocked_domains_;

  // Statistics
  mutable int blocked_count_ = 0;
};

}  // namespace atlas

#endif  // ATLAS_BROWSER_FEATURES_PRIVACY_TRACKER_BLOCKER_H_
