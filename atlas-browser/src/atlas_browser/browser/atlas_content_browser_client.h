// Copyright 2025 Atlas Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license.

#ifndef ATLAS_BROWSER_BROWSER_ATLAS_CONTENT_BROWSER_CLIENT_H_
#define ATLAS_BROWSER_BROWSER_ATLAS_CONTENT_BROWSER_CLIENT_H_

#include "chrome/browser/chrome_content_browser_client.h"

namespace atlas {

// AtlasContentBrowserClient extends Chrome's browser client to add
// Atlas-specific functionality, particularly for the AI agent system.
class AtlasContentBrowserClient : public ChromeContentBrowserClient {
 public:
  AtlasContentBrowserClient();
  ~AtlasContentBrowserClient() override;

  AtlasContentBrowserClient(const AtlasContentBrowserClient&) = delete;
  AtlasContentBrowserClient& operator=(const AtlasContentBrowserClient&) = delete;

  // ContentBrowserClient overrides:
  void ExposeInterfacesToRenderer(
      service_manager::BinderRegistry* registry,
      blink::AssociatedInterfaceRegistry* associated_registry,
      content::RenderProcessHost* render_process_host) override;

  void RegisterBrowserInterfaceBindersForFrame(
      content::RenderFrameHost* render_frame_host,
      mojo::BinderMapWithContext<content::RenderFrameHost*>* map) override;

  std::unique_ptr<content::BrowserMainParts> CreateBrowserMainParts(
      bool is_integration_test) override;

  std::string GetUserAgent() override;
  
  std::string GetProduct() override;

  // Atlas-specific methods
  void InitializeAgentService(content::BrowserContext* context);

 private:
  // Register Atlas-specific Mojo interfaces
  void RegisterAtlasMojoInterfaces(
      content::RenderFrameHost* render_frame_host,
      mojo::BinderMapWithContext<content::RenderFrameHost*>* map);
};

}  // namespace atlas

#endif  // ATLAS_BROWSER_BROWSER_ATLAS_CONTENT_BROWSER_CLIENT_H_
