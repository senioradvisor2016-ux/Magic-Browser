// Copyright 2025 Atlas Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license.

#include "atlas_browser/common/atlas_content_renderer_client.h"

#include "atlas_browser/features/llm_agent/renderer/agent_injector.h"
#include "base/logging.h"
#include "content/public/renderer/render_frame.h"

namespace atlas {

AtlasContentRendererClient::AtlasContentRendererClient() = default;

AtlasContentRendererClient::~AtlasContentRendererClient() = default;

void AtlasContentRendererClient::RenderFrameCreated(
    content::RenderFrame* render_frame) {
  // Call parent implementation
  ChromeContentRendererClient::RenderFrameCreated(render_frame);

  // Create AgentInjector for this frame
  // The AgentInjector is a RenderFrameObserver and will be automatically
  // destroyed when the frame is destroyed.
  new AgentInjector(render_frame);

  LOG(INFO) << "AtlasContentRendererClient: Frame created";
}

void AtlasContentRendererClient::RunScriptsAtDocumentStart(
    content::RenderFrame* render_frame) {
  ChromeContentRendererClient::RunScriptsAtDocumentStart(render_frame);
  
  // Atlas-specific scripts at document start (if any)
}

void AtlasContentRendererClient::RunScriptsAtDocumentEnd(
    content::RenderFrame* render_frame) {
  ChromeContentRendererClient::RunScriptsAtDocumentEnd(render_frame);
  
  // Atlas-specific scripts at document end (if any)
}

}  // namespace atlas
