// Copyright 2025 Atlas Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license.

#include "atlas_browser/features/privacy/tracker_blocker.h"

#include "base/logging.h"
#include "base/strings/string_util.h"
#include "url/gurl.h"

namespace atlas {

namespace {

// Default list of common tracking domains
const char* kDefaultBlockedDomains[] = {
    // Google Analytics
    "google-analytics.com",
    "googletagmanager.com",
    "googleadservices.com",
    "googlesyndication.com",
    "doubleclick.net",

    // Facebook
    "facebook.com/tr",
    "connect.facebook.net",
    "pixel.facebook.com",

    // Twitter
    "analytics.twitter.com",
    "ads-twitter.com",

    // Microsoft
    "bat.bing.com",
    "clarity.ms",

    // Amazon
    "amazon-adsystem.com",

    // Common ad networks
    "adnxs.com",
    "adsrvr.org",
    "adform.net",
    "criteo.com",
    "criteo.net",
    "taboola.com",
    "outbrain.com",
    "pubmatic.com",
    "rubiconproject.com",
    "openx.net",
    "casalemedia.com",
    "bidswitch.net",
    "contextweb.com",

    // Analytics
    "hotjar.com",
    "mixpanel.com",
    "segment.io",
    "segment.com",
    "amplitude.com",
    "mouseflow.com",
    "fullstory.com",
    "crazyegg.com",
    "quantserve.com",
    "scorecardresearch.com",
    "newrelic.com",

    // Social trackers
    "addthis.com",
    "sharethis.com",

    // Fingerprinting
    "fingerprintjs.com",
    "cdn.jsdelivr.net/npm/@aspect/",
};

}  // namespace

TrackerBlocker* TrackerBlocker::GetInstance() {
  return base::Singleton<TrackerBlocker>::get();
}

TrackerBlocker::TrackerBlocker() = default;

TrackerBlocker::~TrackerBlocker() = default;

void TrackerBlocker::Initialize() {
  LoadDefaultBlockList();
  LOG(INFO) << "TrackerBlocker initialized with " << blocked_domains_.size()
            << " blocked domains";
}

void TrackerBlocker::LoadDefaultBlockList() {
  for (const char* domain : kDefaultBlockedDomains) {
    blocked_domains_.insert(domain);
  }
}

bool TrackerBlocker::ShouldBlock(const GURL& url) const {
  if (!url.is_valid())
    return false;

  if (IsTracker(url)) {
    blocked_count_++;
    return true;
  }

  return false;
}

bool TrackerBlocker::IsTracker(const GURL& url) const {
  if (!url.is_valid())
    return false;

  std::string host = url.host();

  // Check exact match
  if (blocked_domains_.contains(host))
    return true;

  // Check if any blocked domain is a suffix of the host
  for (const auto& blocked : blocked_domains_) {
    if (base::EndsWith(host, blocked, base::CompareCase::INSENSITIVE_ASCII))
      return true;
    if (base::EndsWith(host, "." + blocked, base::CompareCase::INSENSITIVE_ASCII))
      return true;
  }

  // Check URL path for tracking patterns
  std::string path = url.path();
  std::string path_lower = base::ToLowerASCII(path);

  // Common tracking endpoints
  if (path_lower.find("/track") != std::string::npos ||
      path_lower.find("/pixel") != std::string::npos ||
      path_lower.find("/beacon") != std::string::npos ||
      path_lower.find("/collect") != std::string::npos ||
      path_lower.find("/analytics") != std::string::npos) {
    return true;
  }

  return false;
}

void TrackerBlocker::AddBlockedDomain(const std::string& domain) {
  blocked_domains_.insert(domain);
  LOG(INFO) << "Added blocked domain: " << domain;
}

void TrackerBlocker::RemoveBlockedDomain(const std::string& domain) {
  blocked_domains_.erase(domain);
  LOG(INFO) << "Removed blocked domain: " << domain;
}

}  // namespace atlas
