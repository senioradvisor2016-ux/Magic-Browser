// Copyright 2025 Atlas Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license.

#include "atlas_browser/browser/atlas_content_browser_client.h"

#include "atlas_browser/browser/atlas_browser_main_parts.h"
#include "atlas_browser/features/llm_agent/browser/agent_service.h"
#include "atlas_browser/features/llm_agent/public/mojom/agent.mojom.h"
#include "base/command_line.h"
#include "base/strings/stringprintf.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"

namespace atlas {

namespace {

// Atlas Browser version info
constexpr char kAtlasProductName[] = "Atlas";
constexpr char kAtlasVersionMajor[] = "1";
constexpr char kAtlasVersionMinor[] = "0";
constexpr char kAtlasVersionPatch[] = "0";

}  // namespace

AtlasContentBrowserClient::AtlasContentBrowserClient() = default;

AtlasContentBrowserClient::~AtlasContentBrowserClient() = default;

void AtlasContentBrowserClient::ExposeInterfacesToRenderer(
    service_manager::BinderRegistry* registry,
    blink::AssociatedInterfaceRegistry* associated_registry,
    content::RenderProcessHost* render_process_host) {
  // Call parent implementation first
  ChromeContentBrowserClient::ExposeInterfacesToRenderer(
      registry, associated_registry, render_process_host);

  // Atlas doesn't need process-level interfaces currently
  // Frame-level interfaces are registered in RegisterBrowserInterfaceBindersForFrame
}

void AtlasContentBrowserClient::RegisterBrowserInterfaceBindersForFrame(
    content::RenderFrameHost* render_frame_host,
    mojo::BinderMapWithContext<content::RenderFrameHost*>* map) {
  // Call parent implementation
  ChromeContentBrowserClient::RegisterBrowserInterfaceBindersForFrame(
      render_frame_host, map);

  // Register Atlas-specific interfaces
  RegisterAtlasMojoInterfaces(render_frame_host, map);
}

void AtlasContentBrowserClient::RegisterAtlasMojoInterfaces(
    content::RenderFrameHost* render_frame_host,
    mojo::BinderMapWithContext<content::RenderFrameHost*>* map) {
  
  // Register AgentBrowserHost interface
  // This allows the renderer to communicate with the browser's agent service
  map->Add<agent::mojom::AgentBrowserHost>(
      base::BindRepeating(
          [](content::RenderFrameHost* frame_host,
             mojo::PendingReceiver<agent::mojom::AgentBrowserHost> receiver) {
            auto* agent_service = AgentService::GetForBrowserContext(
                frame_host->GetBrowserContext());
            if (agent_service) {
              agent_service->BindAgentHost(frame_host, std::move(receiver));
            }
          }));
}

std::unique_ptr<content::BrowserMainParts>
AtlasContentBrowserClient::CreateBrowserMainParts(bool is_integration_test) {
  // Create Atlas-specific browser main parts
  return std::make_unique<AtlasBrowserMainParts>(is_integration_test);
}

std::string AtlasContentBrowserClient::GetUserAgent() {
  // Build Atlas user agent string
  // Format: Mozilla/5.0 (platform) AppleWebKit/537.36 (KHTML, like Gecko) 
  //         Chrome/XXX Atlas/1.0
  std::string chrome_ua = ChromeContentBrowserClient::GetUserAgent();
  
  // Append Atlas identifier
  std::string atlas_token = base::StringPrintf(
      " %s/%s.%s.%s",
      kAtlasProductName,
      kAtlasVersionMajor,
      kAtlasVersionMinor,
      kAtlasVersionPatch);
  
  return chrome_ua + atlas_token;
}

std::string AtlasContentBrowserClient::GetProduct() {
  return base::StringPrintf(
      "%s/%s.%s.%s",
      kAtlasProductName,
      kAtlasVersionMajor,
      kAtlasVersionMinor,
      kAtlasVersionPatch);
}

void AtlasContentBrowserClient::InitializeAgentService(
    content::BrowserContext* context) {
  // Initialize the agent service for this browser context
  AgentService::GetOrCreateForBrowserContext(context);
}

}  // namespace atlas
