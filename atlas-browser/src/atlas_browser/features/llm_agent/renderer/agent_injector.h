// Copyright 2025 Atlas Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license.

#ifndef ATLAS_BROWSER_FEATURES_LLM_AGENT_RENDERER_AGENT_INJECTOR_H_
#define ATLAS_BROWSER_FEATURES_LLM_AGENT_RENDERER_AGENT_INJECTOR_H_

#include <memory>
#include <string>

#include "base/memory/weak_ptr.h"
#include "content/public/renderer/render_frame_observer.h"
#include "mojo/public/cpp/bindings/associated_receiver.h"
#include "mojo/public/cpp/bindings/pending_associated_receiver.h"
#include "atlas_browser/features/llm_agent/public/mojom/agent.mojom.h"
#include "atlas_browser/features/llm_agent/renderer/dom_extractor.h"

namespace blink {
class WebLocalFrame;
}

namespace atlas {

// AgentInjector handles the renderer-side agent functionality.
// It implements the AgentRendererClient Mojo interface to respond to
// browser commands for DOM extraction and action execution.
class AgentInjector : public content::RenderFrameObserver,
                      public agent::mojom::AgentRendererClient {
 public:
  explicit AgentInjector(content::RenderFrame* render_frame);
  ~AgentInjector() override;

  AgentInjector(const AgentInjector&) = delete;
  AgentInjector& operator=(const AgentInjector&) = delete;

  // Bind the Mojo receiver
  void BindReceiver(
      mojo::PendingAssociatedReceiver<agent::mojom::AgentRendererClient>
          receiver);

  // RenderFrameObserver implementation
  void OnDestruct() override;
  void DidCreateDocumentElement() override;
  void DidFinishLoad() override;

  // AgentRendererClient implementation
  void GetAccessibleDOM(GetAccessibleDOMCallback callback) override;
  void GetInteractiveElements(GetInteractiveElementsCallback callback) override;
  void GetPageState(GetPageStateCallback callback) override;
  void PerformAction(agent::mojom::AgentActionPtr action,
                     PerformActionCallback callback) override;
  void HighlightElement(const std::string& selector, bool show) override;
  void ShowElementLabels(bool show,
                         ShowElementLabelsCallback callback) override;
  void ScrollToElement(const std::string& selector,
                       ScrollToElementCallback callback) override;
  void ExecuteScript(const std::string& script,
                     ExecuteScriptCallback callback) override;
  void CaptureScreenshot(CaptureScreenshotCallback callback) override;
  void FillForm(const base::flat_map<std::string, std::string>& field_values,
                FillFormCallback callback) override;

 private:
  // Get the WebLocalFrame
  blink::WebLocalFrame* GetWebFrame();

  // Action executors
  agent::mojom::ActionResultPtr ExecuteClick(
      const agent::mojom::AgentAction& action);
  agent::mojom::ActionResultPtr ExecuteType(
      const agent::mojom::AgentAction& action);
  agent::mojom::ActionResultPtr ExecuteScroll(
      const agent::mojom::AgentAction& action);
  agent::mojom::ActionResultPtr ExecuteHover(
      const agent::mojom::AgentAction& action);
  agent::mojom::ActionResultPtr ExecuteKeyPress(
      const agent::mojom::AgentAction& action);
  agent::mojom::ActionResultPtr ExecuteWait(
      const agent::mojom::AgentAction& action);

  // Get the browser host interface
  mojo::Remote<agent::mojom::AgentBrowserHost>& GetBrowserHost();

  // Notify browser of events
  void NotifyPageStateChanged();

  // Highlighting
  void CreateHighlightOverlay();
  void UpdateHighlight(const std::string& selector);
  void RemoveHighlight();
  void CreateElementLabels();
  void RemoveElementLabels();

  // DOM extractor
  DOMExtractor dom_extractor_;

  // Mojo receiver
  mojo::AssociatedReceiver<agent::mojom::AgentRendererClient> receiver_{this};

  // Browser host connection
  mojo::Remote<agent::mojom::AgentBrowserHost> browser_host_;

  // Currently highlighted element selector
  std::string highlighted_selector_;

  // Whether element labels are shown
  bool labels_shown_ = false;

  base::WeakPtrFactory<AgentInjector> weak_factory_{this};
};

}  // namespace atlas

#endif  // ATLAS_BROWSER_FEATURES_LLM_AGENT_RENDERER_AGENT_INJECTOR_H_
