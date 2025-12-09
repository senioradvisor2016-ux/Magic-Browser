// Copyright 2025 Atlas Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license.

#ifndef ATLAS_BROWSER_COMMON_ATLAS_CONTENT_RENDERER_CLIENT_H_
#define ATLAS_BROWSER_COMMON_ATLAS_CONTENT_RENDERER_CLIENT_H_

#include "chrome/renderer/chrome_content_renderer_client.h"

namespace atlas {

// AtlasContentRendererClient extends Chrome's renderer client to add
// Atlas-specific functionality in the renderer process.
class AtlasContentRendererClient : public ChromeContentRendererClient {
 public:
  AtlasContentRendererClient();
  ~AtlasContentRendererClient() override;

  AtlasContentRendererClient(const AtlasContentRendererClient&) = delete;
  AtlasContentRendererClient& operator=(const AtlasContentRendererClient&) = delete;

  // ContentRendererClient overrides:
  void RenderFrameCreated(content::RenderFrame* render_frame) override;
  void RunScriptsAtDocumentStart(content::RenderFrame* render_frame) override;
  void RunScriptsAtDocumentEnd(content::RenderFrame* render_frame) override;
};

}  // namespace atlas

#endif  // ATLAS_BROWSER_COMMON_ATLAS_CONTENT_RENDERER_CLIENT_H_
