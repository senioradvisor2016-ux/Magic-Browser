// Copyright 2025 Atlas Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license.

#include "atlas_browser/features/privacy/tracker_blocker.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace atlas {
namespace {

class TrackerBlockerTest : public testing::Test {
 protected:
  void SetUp() override {
    blocker_ = TrackerBlocker::GetInstance();
    blocker_->Initialize();
    blocker_->ResetStatistics();
  }

  TrackerBlocker* blocker_;
};

// =============================================================================
// Default Block List Tests
// =============================================================================

TEST_F(TrackerBlockerTest, BlocksGoogleAnalytics) {
  EXPECT_TRUE(blocker_->IsTracker(
      GURL("https://www.google-analytics.com/analytics.js")));
}

TEST_F(TrackerBlockerTest, BlocksGoogleTagManager) {
  EXPECT_TRUE(blocker_->IsTracker(
      GURL("https://www.googletagmanager.com/gtag/js")));
}

TEST_F(TrackerBlockerTest, BlocksDoubleClick) {
  EXPECT_TRUE(blocker_->IsTracker(
      GURL("https://ad.doubleclick.net/pixel")));
}

TEST_F(TrackerBlockerTest, BlocksFacebookPixel) {
  EXPECT_TRUE(blocker_->IsTracker(
      GURL("https://connect.facebook.net/en_US/fbevents.js")));
}

TEST_F(TrackerBlockerTest, BlocksTwitterAnalytics) {
  EXPECT_TRUE(blocker_->IsTracker(
      GURL("https://analytics.twitter.com/i/adsct")));
}

// =============================================================================
// Non-Tracker Tests
// =============================================================================

TEST_F(TrackerBlockerTest, AllowsNormalWebsites) {
  EXPECT_FALSE(blocker_->IsTracker(GURL("https://www.example.com/")));
  EXPECT_FALSE(blocker_->IsTracker(GURL("https://www.wikipedia.org/")));
  EXPECT_FALSE(blocker_->IsTracker(GURL("https://www.github.com/")));
}

TEST_F(TrackerBlockerTest, AllowsFirstPartyResources) {
  EXPECT_FALSE(blocker_->IsTracker(
      GURL("https://www.example.com/scripts/main.js")));
  EXPECT_FALSE(blocker_->IsTracker(
      GURL("https://cdn.example.com/images/logo.png")));
}

TEST_F(TrackerBlockerTest, AllowsAPIEndpoints) {
  EXPECT_FALSE(blocker_->IsTracker(
      GURL("https://api.example.com/v1/users")));
}

// =============================================================================
// Pattern Matching Tests
// =============================================================================

TEST_F(TrackerBlockerTest, BlocksSubdomains) {
  EXPECT_TRUE(blocker_->IsTracker(
      GURL("https://stats.google-analytics.com/collect")));
}

TEST_F(TrackerBlockerTest, BlocksTrackingPaths) {
  EXPECT_TRUE(blocker_->IsTracker(
      GURL("https://example.com/pixel/track")));
  EXPECT_TRUE(blocker_->IsTracker(
      GURL("https://example.com/beacon")));
  EXPECT_TRUE(blocker_->IsTracker(
      GURL("https://example.com/analytics/collect")));
}

// =============================================================================
// Custom Domain Tests
// =============================================================================

TEST_F(TrackerBlockerTest, AddCustomBlockedDomain) {
  GURL test_url("https://custom-tracker.example.com/track.js");
  
  // Not blocked initially
  EXPECT_FALSE(blocker_->IsTracker(test_url));
  
  // Add to block list
  blocker_->AddBlockedDomain("custom-tracker.example.com");
  
  // Now blocked
  EXPECT_TRUE(blocker_->IsTracker(test_url));
}

TEST_F(TrackerBlockerTest, RemoveBlockedDomain) {
  // Add and then remove
  blocker_->AddBlockedDomain("temp-tracker.com");
  EXPECT_TRUE(blocker_->IsTracker(GURL("https://temp-tracker.com/track")));
  
  blocker_->RemoveBlockedDomain("temp-tracker.com");
  EXPECT_FALSE(blocker_->IsTracker(GURL("https://temp-tracker.com/track")));
}

// =============================================================================
// Statistics Tests
// =============================================================================

TEST_F(TrackerBlockerTest, CountsBlockedTrackers) {
  EXPECT_EQ(blocker_->GetBlockedCount(), 0);
  
  blocker_->ShouldBlock(GURL("https://www.google-analytics.com/analytics.js"));
  EXPECT_EQ(blocker_->GetBlockedCount(), 1);
  
  blocker_->ShouldBlock(GURL("https://ad.doubleclick.net/pixel"));
  EXPECT_EQ(blocker_->GetBlockedCount(), 2);
}

TEST_F(TrackerBlockerTest, DoesNotCountNonTrackers) {
  blocker_->ShouldBlock(GURL("https://www.example.com/"));
  EXPECT_EQ(blocker_->GetBlockedCount(), 0);
}

TEST_F(TrackerBlockerTest, ResetStatistics) {
  blocker_->ShouldBlock(GURL("https://www.google-analytics.com/analytics.js"));
  EXPECT_GT(blocker_->GetBlockedCount(), 0);
  
  blocker_->ResetStatistics();
  EXPECT_EQ(blocker_->GetBlockedCount(), 0);
}

// =============================================================================
// Edge Cases
// =============================================================================

TEST_F(TrackerBlockerTest, HandlesInvalidUrl) {
  EXPECT_FALSE(blocker_->ShouldBlock(GURL()));
  EXPECT_FALSE(blocker_->ShouldBlock(GURL("not-a-valid-url")));
}

TEST_F(TrackerBlockerTest, HandlesEmptyHost) {
  EXPECT_FALSE(blocker_->ShouldBlock(GURL("file:///path/to/file")));
}

TEST_F(TrackerBlockerTest, CaseInsensitiveMatching) {
  EXPECT_TRUE(blocker_->IsTracker(
      GURL("https://WWW.GOOGLE-ANALYTICS.COM/analytics.js")));
}

// =============================================================================
// Common Ad Networks
// =============================================================================

TEST_F(TrackerBlockerTest, BlocksCommonAdNetworks) {
  EXPECT_TRUE(blocker_->IsTracker(GURL("https://adnxs.com/pixel")));
  EXPECT_TRUE(blocker_->IsTracker(GURL("https://criteo.com/event")));
  EXPECT_TRUE(blocker_->IsTracker(GURL("https://taboola.com/track")));
  EXPECT_TRUE(blocker_->IsTracker(GURL("https://outbrain.com/widget")));
}

// =============================================================================
// Common Analytics Providers
// =============================================================================

TEST_F(TrackerBlockerTest, BlocksAnalyticsProviders) {
  EXPECT_TRUE(blocker_->IsTracker(GURL("https://hotjar.com/api")));
  EXPECT_TRUE(blocker_->IsTracker(GURL("https://mixpanel.com/track")));
  EXPECT_TRUE(blocker_->IsTracker(GURL("https://segment.io/v1/track")));
  EXPECT_TRUE(blocker_->IsTracker(GURL("https://amplitude.com/api")));
}

}  // namespace
}  // namespace atlas
