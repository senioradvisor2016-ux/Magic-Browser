// Copyright 2025 Atlas Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license.

#ifndef ATLAS_BROWSER_BROWSER_ATLAS_MAIN_DELEGATE_H_
#define ATLAS_BROWSER_BROWSER_ATLAS_MAIN_DELEGATE_H_

#include <memory>

#include "chrome/app/chrome_main_delegate.h"

namespace atlas {

class AtlasContentBrowserClient;
class AtlasContentRendererClient;

// AtlasMainDelegate is the main entry point for Atlas Browser.
// It inherits from ChromeMainDelegate to leverage Chrome's infrastructure
// while adding our own customizations for the AI agent and other features.
class AtlasMainDelegate : public ChromeMainDelegate {
 public:
  AtlasMainDelegate();
  ~AtlasMainDelegate() override;

  AtlasMainDelegate(const AtlasMainDelegate&) = delete;
  AtlasMainDelegate& operator=(const AtlasMainDelegate&) = delete;

  // ContentMainDelegate overrides:
  std::optional<int> BasicStartupComplete() override;
  void PreSandboxStartup() override;
  std::optional<int> PreBrowserMain() override;
  content::ContentBrowserClient* CreateContentBrowserClient() override;
  content::ContentRendererClient* CreateContentRendererClient() override;

 private:
  // Initialize Atlas-specific resources and features
  void InitializeAtlasFeatures();
  
  // Load Atlas resource packs
  void LoadAtlasResources();

  std::unique_ptr<AtlasContentBrowserClient> browser_client_;
  std::unique_ptr<AtlasContentRendererClient> renderer_client_;
};

}  // namespace atlas

#endif  // ATLAS_BROWSER_BROWSER_ATLAS_MAIN_DELEGATE_H_
