// Copyright 2025 Atlas Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license.

#ifndef ATLAS_BROWSER_FEATURES_LLM_AGENT_RENDERER_DOM_EXTRACTOR_H_
#define ATLAS_BROWSER_FEATURES_LLM_AGENT_RENDERER_DOM_EXTRACTOR_H_

#include <string>
#include <vector>

#include "atlas_browser/features/llm_agent/public/mojom/agent.mojom.h"

namespace blink {
class WebDocument;
class WebElement;
class WebLocalFrame;
class WebNode;
}  // namespace blink

namespace atlas {

// DOMExtractor extracts structured information from the DOM for the LLM agent.
// It focuses on interactive elements and provides CSS selectors for targeting.
class DOMExtractor {
 public:
  DOMExtractor();
  ~DOMExtractor();

  DOMExtractor(const DOMExtractor&) = delete;
  DOMExtractor& operator=(const DOMExtractor&) = delete;

  // Configuration
  struct Config {
    int max_depth = 20;              // Maximum DOM tree depth to traverse
    int max_elements = 500;          // Maximum elements to return
    int max_text_length = 200;       // Maximum text content length per element
    bool include_hidden = false;     // Include hidden elements
    bool include_aria = true;        // Include ARIA attributes
    bool include_data_attrs = false; // Include data-* attributes
  };

  void Configure(const Config& config) { config_ = config; }

  // Get accessible DOM as JSON string (for LLM context)
  std::string GetAccessibleDOM(blink::WebLocalFrame* frame);

  // Get list of interactive elements
  std::vector<agent::mojom::ElementInfoPtr> GetInteractiveElements(
      blink::WebLocalFrame* frame);

  // Get current page state
  agent::mojom::PageStatePtr GetPageState(blink::WebLocalFrame* frame);

  // Get element info for a specific selector
  agent::mojom::ElementInfoPtr GetElementInfo(blink::WebLocalFrame* frame,
                                               const std::string& selector);

  // Generate a unique CSS selector for an element
  static std::string GenerateSelector(const blink::WebElement& element);

  // Generate XPath for an element
  static std::string GenerateXPath(const blink::WebElement& element);

 private:
  // Recursively extract elements from the DOM
  void ExtractElements(const blink::WebElement& element,
                       std::vector<agent::mojom::ElementInfoPtr>* output,
                       int depth);

  // Convert a WebElement to ElementInfo
  agent::mojom::ElementInfoPtr ElementToInfo(const blink::WebElement& element);

  // Check if element is interactive
  bool IsInteractive(const blink::WebElement& element) const;

  // Check if element is visible
  bool IsVisible(const blink::WebElement& element) const;

  // Check if element is in viewport
  bool IsInViewport(const blink::WebElement& element,
                    int viewport_width,
                    int viewport_height) const;

  // Get visible text content of an element (truncated)
  std::string GetVisibleText(const blink::WebElement& element) const;

  // Get element role (explicit or inferred from tag)
  std::string GetRole(const blink::WebElement& element) const;

  // Check if element has attached event listeners
  std::vector<std::string> GetEventListeners(
      const blink::WebElement& element) const;

  Config config_;
  int element_count_ = 0;
};

}  // namespace atlas

#endif  // ATLAS_BROWSER_FEATURES_LLM_AGENT_RENDERER_DOM_EXTRACTOR_H_
