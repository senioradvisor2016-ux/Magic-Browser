// Copyright 2025 Atlas Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license.

#include "atlas_browser/features/llm_agent/renderer/agent_injector.h"

#include "base/logging.h"
#include "content/public/renderer/render_frame.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/web/web_document.h"
#include "third_party/blink/public/web/web_element.h"
#include "third_party/blink/public/web/web_input_element.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_script_source.h"
#include "third_party/blink/public/web/web_view.h"
#include "ui/events/keycodes/dom/dom_key.h"

namespace atlas {

AgentInjector::AgentInjector(content::RenderFrame* render_frame)
    : content::RenderFrameObserver(render_frame) {
  LOG(INFO) << "AgentInjector created for frame";
}

AgentInjector::~AgentInjector() = default;

void AgentInjector::BindReceiver(
    mojo::PendingAssociatedReceiver<agent::mojom::AgentRendererClient>
        receiver) {
  receiver_.Bind(std::move(receiver));
}

void AgentInjector::OnDestruct() {
  delete this;
}

void AgentInjector::DidCreateDocumentElement() {
  LOG(INFO) << "Document element created";
}

void AgentInjector::DidFinishLoad() {
  LOG(INFO) << "Page finished loading";
  NotifyPageStateChanged();
}

blink::WebLocalFrame* AgentInjector::GetWebFrame() {
  if (!render_frame())
    return nullptr;
  return render_frame()->GetWebFrame();
}

// =============================================================================
// AgentRendererClient Implementation
// =============================================================================

void AgentInjector::GetAccessibleDOM(GetAccessibleDOMCallback callback) {
  blink::WebLocalFrame* frame = GetWebFrame();
  if (!frame) {
    std::move(callback).Run("", {});
    return;
  }

  std::string dom_json = dom_extractor_.GetAccessibleDOM(frame);
  auto elements = dom_extractor_.GetInteractiveElements(frame);

  std::move(callback).Run(dom_json, std::move(elements));
}

void AgentInjector::GetInteractiveElements(
    GetInteractiveElementsCallback callback) {
  blink::WebLocalFrame* frame = GetWebFrame();
  if (!frame) {
    std::move(callback).Run({});
    return;
  }

  auto elements = dom_extractor_.GetInteractiveElements(frame);
  std::move(callback).Run(std::move(elements));
}

void AgentInjector::GetPageState(GetPageStateCallback callback) {
  blink::WebLocalFrame* frame = GetWebFrame();
  if (!frame) {
    std::move(callback).Run(agent::mojom::PageState::New());
    return;
  }

  auto state = dom_extractor_.GetPageState(frame);
  std::move(callback).Run(std::move(state));
}

void AgentInjector::PerformAction(agent::mojom::AgentActionPtr action,
                                  PerformActionCallback callback) {
  agent::mojom::ActionResultPtr result;

  switch (action->type) {
    case agent::mojom::ActionType::kClick:
      result = ExecuteClick(*action);
      break;
    case agent::mojom::ActionType::kType:
      result = ExecuteType(*action);
      break;
    case agent::mojom::ActionType::kScroll:
      result = ExecuteScroll(*action);
      break;
    case agent::mojom::ActionType::kHover:
      result = ExecuteHover(*action);
      break;
    case agent::mojom::ActionType::kPressKey:
      result = ExecuteKeyPress(*action);
      break;
    case agent::mojom::ActionType::kWait:
      result = ExecuteWait(*action);
      break;
    case agent::mojom::ActionType::kNavigate:
      // Navigation is handled by browser process
      result = agent::mojom::ActionResult::New();
      result->success = false;
      result->error_message = "Navigate action should be handled by browser";
      break;
    default:
      result = agent::mojom::ActionResult::New();
      result->success = false;
      result->error_message = "Unknown action type";
  }

  // Get new page state
  blink::WebLocalFrame* frame = GetWebFrame();
  if (frame) {
    result->new_state = dom_extractor_.GetPageState(frame);
  }

  std::move(callback).Run(std::move(result));
}

void AgentInjector::HighlightElement(const std::string& selector, bool show) {
  if (show) {
    UpdateHighlight(selector);
    highlighted_selector_ = selector;
  } else {
    RemoveHighlight();
    highlighted_selector_.clear();
  }
}

void AgentInjector::ShowElementLabels(bool show,
                                      ShowElementLabelsCallback callback) {
  if (show && !labels_shown_) {
    CreateElementLabels();
    labels_shown_ = true;
  } else if (!show && labels_shown_) {
    RemoveElementLabels();
    labels_shown_ = false;
  }

  // Count labeled elements
  blink::WebLocalFrame* frame = GetWebFrame();
  int count = 0;
  if (frame) {
    auto elements = dom_extractor_.GetInteractiveElements(frame);
    count = elements.size();
  }

  std::move(callback).Run(count);
}

void AgentInjector::ScrollToElement(const std::string& selector,
                                    ScrollToElementCallback callback) {
  blink::WebLocalFrame* frame = GetWebFrame();
  if (!frame) {
    std::move(callback).Run(false);
    return;
  }

  blink::WebDocument doc = frame->GetDocument();
  blink::WebElement element =
      doc.QuerySelector(blink::WebString::FromUTF8(selector));

  if (element.IsNull()) {
    std::move(callback).Run(false);
    return;
  }

  element.ScrollIntoViewIfNeeded(/*center=*/true);
  std::move(callback).Run(true);
}

void AgentInjector::ExecuteScript(const std::string& script,
                                  ExecuteScriptCallback callback) {
  blink::WebLocalFrame* frame = GetWebFrame();
  if (!frame) {
    std::move(callback).Run(std::nullopt, "No frame available");
    return;
  }

  // Execute script and get result
  blink::WebScriptSource source(blink::WebString::FromUTF8(script));
  v8::Local<v8::Value> result = frame->ExecuteScriptAndReturnValue(source);

  if (result.IsEmpty()) {
    std::move(callback).Run(std::nullopt, std::nullopt);
    return;
  }

  // Convert result to string
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::String::Utf8Value utf8(isolate, result);
  if (*utf8) {
    std::move(callback).Run(std::string(*utf8), std::nullopt);
  } else {
    std::move(callback).Run("undefined", std::nullopt);
  }
}

void AgentInjector::CaptureScreenshot(CaptureScreenshotCallback callback) {
  // Screenshot capture is handled by browser process
  // This is just a stub
  std::move(callback).Run("");
}

void AgentInjector::FillForm(
    const base::flat_map<std::string, std::string>& field_values,
    FillFormCallback callback) {
  blink::WebLocalFrame* frame = GetWebFrame();
  if (!frame) {
    auto result = agent::mojom::ActionResult::New();
    result->success = false;
    result->error_message = "No frame available";
    std::move(callback).Run(std::move(result));
    return;
  }

  blink::WebDocument doc = frame->GetDocument();
  bool all_success = true;
  std::vector<std::string> failed_fields;

  for (const auto& [selector, value] : field_values) {
    blink::WebElement element =
        doc.QuerySelector(blink::WebString::FromUTF8(selector));

    if (element.IsNull()) {
      all_success = false;
      failed_fields.push_back(selector);
      continue;
    }

    // Handle different input types
    if (element.HasHTMLTagName("input") || element.HasHTMLTagName("textarea")) {
      blink::WebInputElement input = element.To<blink::WebInputElement>();
      input.SetValue(blink::WebString::FromUTF8(value));
      input.DispatchInputEvent();
    } else if (element.HasHTMLTagName("select")) {
      // For select, try to find and select the option
      // This would need more complex handling
      all_success = false;
      failed_fields.push_back(selector);
    }
  }

  auto result = agent::mojom::ActionResult::New();
  result->success = all_success;
  if (!all_success) {
    result->error_message =
        "Failed to fill fields: " + base::JoinString(failed_fields, ", ");
  }

  std::move(callback).Run(std::move(result));
}

// =============================================================================
// Action Executors
// =============================================================================

agent::mojom::ActionResultPtr AgentInjector::ExecuteClick(
    const agent::mojom::AgentAction& action) {
  auto result = agent::mojom::ActionResult::New();

  blink::WebLocalFrame* frame = GetWebFrame();
  if (!frame) {
    result->success = false;
    result->error_message = "No frame available";
    return result;
  }

  blink::WebDocument doc = frame->GetDocument();

  // Find element by selector or coordinates
  blink::WebElement element;
  if (action.selector) {
    element = doc.QuerySelector(blink::WebString::FromUTF8(*action.selector));
  } else if (action.x && action.y) {
    // Click at coordinates - would need hit testing
    result->success = false;
    result->error_message = "Coordinate-based click not implemented";
    return result;
  }

  if (element.IsNull()) {
    result->success = false;
    result->error_message = "Element not found: " +
                            (action.selector ? *action.selector : "no selector");
    return result;
  }

  // Scroll element into view
  element.ScrollIntoViewIfNeeded(/*center=*/true);

  // Simulate click
  element.SimulateClick();

  result->success = true;
  LOG(INFO) << "Clicked element: "
            << (action.selector ? *action.selector : "unknown");

  return result;
}

agent::mojom::ActionResultPtr AgentInjector::ExecuteType(
    const agent::mojom::AgentAction& action) {
  auto result = agent::mojom::ActionResult::New();

  if (!action.selector || !action.value) {
    result->success = false;
    result->error_message = "Type action requires selector and value";
    return result;
  }

  blink::WebLocalFrame* frame = GetWebFrame();
  if (!frame) {
    result->success = false;
    result->error_message = "No frame available";
    return result;
  }

  blink::WebDocument doc = frame->GetDocument();
  blink::WebElement element =
      doc.QuerySelector(blink::WebString::FromUTF8(*action.selector));

  if (element.IsNull()) {
    result->success = false;
    result->error_message = "Element not found: " + *action.selector;
    return result;
  }

  // Check if element is an input
  if (!element.HasHTMLTagName("input") && !element.HasHTMLTagName("textarea") &&
      !element.IsEditable()) {
    result->success = false;
    result->error_message = "Element is not editable";
    return result;
  }

  // Focus the element
  element.Focus();

  // Set value
  if (element.HasHTMLTagName("input") || element.HasHTMLTagName("textarea")) {
    blink::WebInputElement input = element.To<blink::WebInputElement>();
    input.SetValue(blink::WebString::FromUTF8(*action.value));
    input.DispatchInputEvent();
  } else {
    // Contenteditable
    element.SetInnerHTML(blink::WebString::FromUTF8(*action.value));
  }

  result->success = true;
  LOG(INFO) << "Typed into element: " << *action.selector;

  return result;
}

agent::mojom::ActionResultPtr AgentInjector::ExecuteScroll(
    const agent::mojom::AgentAction& action) {
  auto result = agent::mojom::ActionResult::New();

  blink::WebLocalFrame* frame = GetWebFrame();
  if (!frame) {
    result->success = false;
    result->error_message = "No frame available";
    return result;
  }

  if (action.selector) {
    // Scroll to element
    blink::WebDocument doc = frame->GetDocument();
    blink::WebElement element =
        doc.QuerySelector(blink::WebString::FromUTF8(*action.selector));

    if (element.IsNull()) {
      result->success = false;
      result->error_message = "Element not found: " + *action.selector;
      return result;
    }

    element.ScrollIntoViewIfNeeded(/*center=*/true);
    result->success = true;
  } else if (action.value) {
    // Scroll by direction or amount
    std::string direction = *action.value;
    int scroll_amount = 300;  // Default scroll amount

    blink::WebView* view = frame->View();
    if (!view) {
      result->success = false;
      result->error_message = "No view available";
      return result;
    }

    gfx::PointF current = frame->GetScrollOffset();
    gfx::PointF target = current;

    if (direction == "up") {
      target.set_y(current.y() - scroll_amount);
    } else if (direction == "down") {
      target.set_y(current.y() + scroll_amount);
    } else if (direction == "left") {
      target.set_x(current.x() - scroll_amount);
    } else if (direction == "right") {
      target.set_x(current.x() + scroll_amount);
    } else if (direction == "top") {
      target.set_y(0);
    } else if (direction == "bottom") {
      // Would need document height
      target.set_y(10000);  // Large value
    }

    frame->SetScrollOffset(gfx::Size(target.x(), target.y()));
    result->success = true;
  } else {
    result->success = false;
    result->error_message = "Scroll requires selector or direction";
  }

  return result;
}

agent::mojom::ActionResultPtr AgentInjector::ExecuteHover(
    const agent::mojom::AgentAction& action) {
  auto result = agent::mojom::ActionResult::New();

  if (!action.selector) {
    result->success = false;
    result->error_message = "Hover action requires selector";
    return result;
  }

  blink::WebLocalFrame* frame = GetWebFrame();
  if (!frame) {
    result->success = false;
    result->error_message = "No frame available";
    return result;
  }

  blink::WebDocument doc = frame->GetDocument();
  blink::WebElement element =
      doc.QuerySelector(blink::WebString::FromUTF8(*action.selector));

  if (element.IsNull()) {
    result->success = false;
    result->error_message = "Element not found: " + *action.selector;
    return result;
  }

  // Scroll into view
  element.ScrollIntoViewIfNeeded(/*center=*/true);

  // Dispatch mouseover event
  // Note: Full hover simulation would require more complex event handling
  element.DispatchMouseEvent(
      blink::WebInputEvent::Type::kMouseEnter,
      element.BoundsInWidget().CenterPoint(),
      /*click_count=*/0);

  result->success = true;
  return result;
}

agent::mojom::ActionResultPtr AgentInjector::ExecuteKeyPress(
    const agent::mojom::AgentAction& action) {
  auto result = agent::mojom::ActionResult::New();

  if (!action.key) {
    result->success = false;
    result->error_message = "KeyPress action requires key";
    return result;
  }

  // Key press simulation would require WebInputEvent creation
  // This is a simplified stub
  result->success = false;
  result->error_message = "KeyPress not fully implemented";

  return result;
}

agent::mojom::ActionResultPtr AgentInjector::ExecuteWait(
    const agent::mojom::AgentAction& action) {
  auto result = agent::mojom::ActionResult::New();

  // Wait is handled asynchronously - this is a synchronous stub
  // Real implementation would use PostDelayedTask

  if (action.wait_for) {
    // Wait for element
    blink::WebLocalFrame* frame = GetWebFrame();
    if (frame) {
      blink::WebDocument doc = frame->GetDocument();
      blink::WebElement element =
          doc.QuerySelector(blink::WebString::FromUTF8(*action.wait_for));
      result->success = !element.IsNull();
      if (!result->success) {
        result->error_message = "Element not found: " + *action.wait_for;
      }
    }
  } else {
    // Simple delay - just return success
    result->success = true;
  }

  return result;
}

// =============================================================================
// Helpers
// =============================================================================

void AgentInjector::NotifyPageStateChanged() {
  auto& host = GetBrowserHost();
  if (!host)
    return;

  blink::WebLocalFrame* frame = GetWebFrame();
  if (!frame)
    return;

  auto state = dom_extractor_.GetPageState(frame);
  host->OnPageStateChanged(std::move(state));
}

mojo::Remote<agent::mojom::AgentBrowserHost>& AgentInjector::GetBrowserHost() {
  // In real implementation, this would be bound during setup
  return browser_host_;
}

void AgentInjector::CreateHighlightOverlay() {
  // Would inject CSS and overlay element
}

void AgentInjector::UpdateHighlight(const std::string& selector) {
  blink::WebLocalFrame* frame = GetWebFrame();
  if (!frame)
    return;

  // Inject highlight style via JavaScript
  std::string script = R"(
    (function() {
      var el = document.querySelector(')" + selector + R"(');
      if (el) {
        el.style.outline = '3px solid #FF6B6B';
        el.style.outlineOffset = '2px';
        el.setAttribute('data-atlas-highlighted', 'true');
      }
    })();
  )";

  frame->ExecuteScript(blink::WebScriptSource(blink::WebString::FromUTF8(script)));
}

void AgentInjector::RemoveHighlight() {
  blink::WebLocalFrame* frame = GetWebFrame();
  if (!frame)
    return;

  std::string script = R"(
    (function() {
      var els = document.querySelectorAll('[data-atlas-highlighted]');
      els.forEach(function(el) {
        el.style.outline = '';
        el.style.outlineOffset = '';
        el.removeAttribute('data-atlas-highlighted');
      });
    })();
  )";

  frame->ExecuteScript(blink::WebScriptSource(blink::WebString::FromUTF8(script)));
}

void AgentInjector::CreateElementLabels() {
  blink::WebLocalFrame* frame = GetWebFrame();
  if (!frame)
    return;

  // Inject labels for all interactive elements
  std::string script = R"(
    (function() {
      // Remove existing labels
      document.querySelectorAll('.atlas-element-label').forEach(l => l.remove());
      
      // Get interactive elements
      var selectors = 'a, button, input, select, textarea, [role="button"], [onclick], [tabindex]:not([tabindex="-1"])';
      var elements = document.querySelectorAll(selectors);
      
      var labelNum = 1;
      elements.forEach(function(el) {
        var rect = el.getBoundingClientRect();
        if (rect.width > 0 && rect.height > 0 && rect.top < window.innerHeight && rect.bottom > 0) {
          var label = document.createElement('div');
          label.className = 'atlas-element-label';
          label.textContent = labelNum++;
          label.style.cssText = 'position:fixed;background:#FF6B6B;color:white;font-size:10px;padding:1px 4px;border-radius:3px;z-index:999999;pointer-events:none;font-family:monospace;';
          label.style.left = rect.left + 'px';
          label.style.top = rect.top + 'px';
          document.body.appendChild(label);
        }
      });
    })();
  )";

  frame->ExecuteScript(blink::WebScriptSource(blink::WebString::FromUTF8(script)));
}

void AgentInjector::RemoveElementLabels() {
  blink::WebLocalFrame* frame = GetWebFrame();
  if (!frame)
    return;

  std::string script = R"(
    document.querySelectorAll('.atlas-element-label').forEach(l => l.remove());
  )";

  frame->ExecuteScript(blink::WebScriptSource(blink::WebString::FromUTF8(script)));
}

}  // namespace atlas
