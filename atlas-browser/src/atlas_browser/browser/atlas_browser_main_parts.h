// Copyright 2025 Atlas Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license.

#ifndef ATLAS_BROWSER_BROWSER_ATLAS_BROWSER_MAIN_PARTS_H_
#define ATLAS_BROWSER_BROWSER_ATLAS_BROWSER_MAIN_PARTS_H_

#include "chrome/browser/chrome_browser_main.h"

namespace atlas {

// AtlasBrowserMainParts handles Atlas-specific initialization during
// the browser startup process.
class AtlasBrowserMainParts : public ChromeBrowserMainParts {
 public:
  explicit AtlasBrowserMainParts(bool is_integration_test);
  ~AtlasBrowserMainParts() override;

  AtlasBrowserMainParts(const AtlasBrowserMainParts&) = delete;
  AtlasBrowserMainParts& operator=(const AtlasBrowserMainParts&) = delete;

  // BrowserMainParts overrides:
  int PreEarlyInitialization() override;
  void PreCreateMainMessageLoop() override;
  void PostCreateMainMessageLoop() override;
  int PreMainMessageLoopRun() override;
  void PostMainMessageLoopRun() override;

 private:
  // Initialize Atlas UI components
  void InitializeAtlasUI();
  
  // Initialize Atlas agent system
  void InitializeAgentSystem();
  
  // Initialize privacy features
  void InitializePrivacyFeatures();

  bool is_integration_test_;
};

}  // namespace atlas

#endif  // ATLAS_BROWSER_BROWSER_ATLAS_BROWSER_MAIN_PARTS_H_
