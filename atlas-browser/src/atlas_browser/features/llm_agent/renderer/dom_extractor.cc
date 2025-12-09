// Copyright 2025 Atlas Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license.

#include "atlas_browser/features/llm_agent/renderer/dom_extractor.h"

#include "base/containers/flat_set.h"
#include "base/json/json_writer.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/web/web_document.h"
#include "third_party/blink/public/web/web_element.h"
#include "third_party/blink/public/web/web_element_collection.h"
#include "third_party/blink/public/web/web_form_element.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_node.h"
#include "third_party/blink/public/web/web_view.h"
#include "ui/gfx/geometry/rect.h"

namespace atlas {

namespace {

// Interactive HTML tags
const base::flat_set<std::string> kInteractiveTags = {
    "A",        "BUTTON",   "INPUT",    "SELECT",   "TEXTAREA",
    "DETAILS",  "SUMMARY",  "DIALOG",   "MENU",     "MENUITEM",
};

// Tags that are typically interactive when clickable
const base::flat_set<std::string> kPotentiallyInteractiveTags = {
    "DIV", "SPAN", "LI", "TD", "TH", "LABEL", "IMG",
};

// Interactive ARIA roles
const base::flat_set<std::string> kInteractiveRoles = {
    "button",       "link",       "menuitem",   "option",    "tab",
    "checkbox",     "radio",      "switch",     "textbox",   "combobox",
    "searchbox",    "slider",     "spinbutton", "listbox",   "menu",
    "menubar",      "tablist",    "tree",       "treegrid",  "grid",
    "listitem",     "treeitem",   "gridcell",   "rowheader", "columnheader",
};

// Map of tag names to default roles
const std::map<std::string, std::string> kTagToRole = {
    {"A", "link"},
    {"BUTTON", "button"},
    {"INPUT", "textbox"},
    {"SELECT", "combobox"},
    {"TEXTAREA", "textbox"},
    {"IMG", "img"},
    {"NAV", "navigation"},
    {"MAIN", "main"},
    {"HEADER", "banner"},
    {"FOOTER", "contentinfo"},
    {"ASIDE", "complementary"},
    {"ARTICLE", "article"},
    {"SECTION", "region"},
    {"FORM", "form"},
    {"TABLE", "table"},
    {"UL", "list"},
    {"OL", "list"},
    {"LI", "listitem"},
};

}  // namespace

DOMExtractor::DOMExtractor() = default;

DOMExtractor::~DOMExtractor() = default;

std::string DOMExtractor::GetAccessibleDOM(blink::WebLocalFrame* frame) {
  if (!frame)
    return "{}";

  blink::WebDocument doc = frame->GetDocument();
  if (doc.IsNull())
    return "{}";

  base::Value::Dict result;

  // Page info
  result.Set("url", doc.Url().GetString().Utf8());
  result.Set("title", doc.Title().Utf8());

  // Viewport info
  blink::WebView* view = frame->View();
  if (view) {
    base::Value::Dict viewport;
    gfx::Size size = view->Size();
    viewport.Set("width", size.width());
    viewport.Set("height", size.height());

    gfx::PointF scroll = frame->GetScrollOffset();
    viewport.Set("scrollX", static_cast<int>(scroll.x()));
    viewport.Set("scrollY", static_cast<int>(scroll.y()));

    result.Set("viewport", std::move(viewport));
  }

  // Extract interactive elements
  element_count_ = 0;
  auto elements = GetInteractiveElements(frame);

  base::Value::List elements_list;
  for (const auto& elem : elements) {
    base::Value::Dict elem_dict;
    elem_dict.Set("tag", elem->tag_name);

    if (elem->id)
      elem_dict.Set("id", *elem->id);
    if (elem->class_name)
      elem_dict.Set("class", *elem->class_name);
    if (elem->text_content)
      elem_dict.Set("text", *elem->text_content);
    if (elem->aria_label)
      elem_dict.Set("aria-label", *elem->aria_label);
    if (elem->placeholder)
      elem_dict.Set("placeholder", *elem->placeholder);
    if (elem->href)
      elem_dict.Set("href", *elem->href);
    if (elem->value)
      elem_dict.Set("value", *elem->value);
    if (elem->type)
      elem_dict.Set("type", *elem->type);

    elem_dict.Set("selector", elem->css_selector);

    // Bounding box
    base::Value::Dict bounds;
    bounds.Set("x", elem->bounding_box.x());
    bounds.Set("y", elem->bounding_box.y());
    bounds.Set("w", elem->bounding_box.width());
    bounds.Set("h", elem->bounding_box.height());
    elem_dict.Set("bounds", std::move(bounds));

    elem_dict.Set("visible", elem->is_visible);
    elem_dict.Set("clickable", elem->is_clickable);
    elem_dict.Set("editable", elem->is_editable);

    elements_list.Append(std::move(elem_dict));
  }

  result.Set("elements", std::move(elements_list));
  result.Set("elementCount", static_cast<int>(elements.size()));

  // Serialize to JSON
  std::string json;
  base::JSONWriter::WriteWithOptions(
      result, base::JSONWriter::OPTIONS_PRETTY_PRINT, &json);

  return json;
}

std::vector<agent::mojom::ElementInfoPtr> DOMExtractor::GetInteractiveElements(
    blink::WebLocalFrame* frame) {
  std::vector<agent::mojom::ElementInfoPtr> elements;

  if (!frame)
    return elements;

  blink::WebDocument doc = frame->GetDocument();
  if (doc.IsNull())
    return elements;

  blink::WebElement body = doc.Body();
  if (body.IsNull())
    return elements;

  element_count_ = 0;
  ExtractElements(body, &elements, 0);

  return elements;
}

agent::mojom::PageStatePtr DOMExtractor::GetPageState(
    blink::WebLocalFrame* frame) {
  auto state = agent::mojom::PageState::New();

  if (!frame) {
    state->is_loading = false;
    return state;
  }

  blink::WebDocument doc = frame->GetDocument();
  if (!doc.IsNull()) {
    state->url = doc.Url();
    state->title = doc.Title().Utf8();
  }

  state->is_loading = frame->IsLoading();

  blink::WebView* view = frame->View();
  if (view) {
    gfx::Size size = view->Size();
    state->viewport_width = size.width();
    state->viewport_height = size.height();
  }

  gfx::PointF scroll = frame->GetScrollOffset();
  state->scroll_x = static_cast<int>(scroll.x());
  state->scroll_y = static_cast<int>(scroll.y());

  // Document dimensions would require more complex calculation
  state->document_width = state->viewport_width;
  state->document_height = state->viewport_height;

  return state;
}

agent::mojom::ElementInfoPtr DOMExtractor::GetElementInfo(
    blink::WebLocalFrame* frame,
    const std::string& selector) {
  if (!frame)
    return nullptr;

  blink::WebDocument doc = frame->GetDocument();
  if (doc.IsNull())
    return nullptr;

  blink::WebElement element =
      doc.QuerySelector(blink::WebString::FromUTF8(selector));
  if (element.IsNull())
    return nullptr;

  return ElementToInfo(element);
}

void DOMExtractor::ExtractElements(
    const blink::WebElement& element,
    std::vector<agent::mojom::ElementInfoPtr>* output,
    int depth) {
  if (element.IsNull())
    return;

  // Check limits
  if (depth > config_.max_depth)
    return;
  if (element_count_ >= config_.max_elements)
    return;

  // Check if element should be included
  bool is_interactive = IsInteractive(element);
  bool is_visible = config_.include_hidden || IsVisible(element);

  if (is_interactive && is_visible) {
    auto info = ElementToInfo(element);
    if (info) {
      output->push_back(std::move(info));
      element_count_++;
    }
  }

  // Recurse through children
  for (blink::WebNode child = element.FirstChild(); !child.IsNull();
       child = child.NextSibling()) {
    if (child.IsElementNode()) {
      ExtractElements(child.To<blink::WebElement>(), output, depth + 1);
    }
  }
}

agent::mojom::ElementInfoPtr DOMExtractor::ElementToInfo(
    const blink::WebElement& element) {
  auto info = agent::mojom::ElementInfo::New();

  // Basic info
  info->tag_name = element.TagName().Utf8();

  // ID
  std::string id = element.GetAttribute("id").Utf8();
  if (!id.empty())
    info->id = id;

  // Class
  std::string class_name = element.GetAttribute("class").Utf8();
  if (!class_name.empty())
    info->class_name = class_name;

  // Text content
  std::string text = GetVisibleText(element);
  if (!text.empty())
    info->text_content = text;

  // ARIA attributes
  if (config_.include_aria) {
    std::string aria_label = element.GetAttribute("aria-label").Utf8();
    if (!aria_label.empty())
      info->aria_label = aria_label;

    std::string role = GetRole(element);
    if (!role.empty())
      info->aria_role = role;
  }

  // Form-related attributes
  std::string placeholder = element.GetAttribute("placeholder").Utf8();
  if (!placeholder.empty())
    info->placeholder = placeholder;

  std::string href = element.GetAttribute("href").Utf8();
  if (!href.empty())
    info->href = href;

  std::string src = element.GetAttribute("src").Utf8();
  if (!src.empty())
    info->src = src;

  std::string value = element.GetAttribute("value").Utf8();
  if (!value.empty())
    info->value = value;

  std::string name = element.GetAttribute("name").Utf8();
  if (!name.empty())
    info->name = name;

  std::string type = element.GetAttribute("type").Utf8();
  if (!type.empty())
    info->type = type;

  // Selectors
  info->css_selector = GenerateSelector(element);
  info->xpath = GenerateXPath(element);

  // Bounding box
  gfx::Rect bounds = element.BoundsInWidget();
  info->bounding_box = bounds;

  // State flags
  info->is_visible = IsVisible(element);
  info->is_clickable = IsInteractive(element);
  info->is_editable =
      (info->tag_name == "INPUT" || info->tag_name == "TEXTAREA" ||
       element.HasAttribute("contenteditable"));
  info->is_focusable = element.IsFocusable();
  info->is_disabled = element.HasAttribute("disabled");

  // Checked state for checkboxes/radios
  if (type == "checkbox" || type == "radio") {
    info->is_checked = element.HasAttribute("checked");
  }

  // Tab index
  std::string tab_index_str = element.GetAttribute("tabindex").Utf8();
  if (!tab_index_str.empty()) {
    int tab_index;
    if (base::StringToInt(tab_index_str, &tab_index)) {
      info->tab_index = tab_index;
    }
  }

  // Generate unique ID for this element
  info->unique_id = info->css_selector;

  return info;
}

bool DOMExtractor::IsInteractive(const blink::WebElement& element) const {
  std::string tag = element.TagName().Utf8();

  // Check tag
  if (kInteractiveTags.contains(tag))
    return true;

  // Check role
  std::string role = element.GetAttribute("role").Utf8();
  if (!role.empty() && kInteractiveRoles.contains(role))
    return true;

  // Check for click handlers
  if (element.HasAttribute("onclick") || element.HasAttribute("onmousedown") ||
      element.HasAttribute("onmouseup") || element.HasAttribute("ontouchstart"))
    return true;

  // Check tabindex (makes element focusable/interactive)
  if (element.HasAttribute("tabindex")) {
    std::string tabindex = element.GetAttribute("tabindex").Utf8();
    if (tabindex != "-1")
      return true;
  }

  // Check for contenteditable
  if (element.HasAttribute("contenteditable") &&
      element.GetAttribute("contenteditable").Utf8() != "false")
    return true;

  // Check potentially interactive elements
  if (kPotentiallyInteractiveTags.contains(tag)) {
    // These are interactive if they have cursor: pointer or similar
    // Would need computed style access for full check
    if (element.HasAttribute("onclick") || element.HasAttribute("role"))
      return true;
  }

  return false;
}

bool DOMExtractor::IsVisible(const blink::WebElement& element) const {
  // Check bounding box
  gfx::Rect bounds = element.BoundsInWidget();
  if (bounds.width() <= 0 || bounds.height() <= 0)
    return false;

  // Check hidden attribute
  if (element.HasAttribute("hidden"))
    return false;

  // Check aria-hidden
  if (element.GetAttribute("aria-hidden").Utf8() == "true")
    return false;

  // Check style attribute for common hidden patterns
  std::string style = element.GetAttribute("style").Utf8();
  std::string style_lower = base::ToLowerASCII(style);
  if (style_lower.find("display:none") != std::string::npos ||
      style_lower.find("display: none") != std::string::npos ||
      style_lower.find("visibility:hidden") != std::string::npos ||
      style_lower.find("visibility: hidden") != std::string::npos ||
      style_lower.find("opacity:0") != std::string::npos ||
      style_lower.find("opacity: 0") != std::string::npos) {
    return false;
  }

  return true;
}

bool DOMExtractor::IsInViewport(const blink::WebElement& element,
                                int viewport_width,
                                int viewport_height) const {
  gfx::Rect bounds = element.BoundsInWidget();

  // Check if element overlaps with viewport
  return bounds.x() < viewport_width && bounds.y() < viewport_height &&
         bounds.right() > 0 && bounds.bottom() > 0;
}

std::string DOMExtractor::GetVisibleText(
    const blink::WebElement& element) const {
  std::string text = element.TextContent().Utf8();

  // Trim whitespace
  text = base::TrimWhitespaceASCII(text, base::TRIM_ALL);

  // Collapse multiple spaces
  std::string collapsed;
  bool last_was_space = false;
  for (char c : text) {
    if (std::isspace(c)) {
      if (!last_was_space) {
        collapsed += ' ';
        last_was_space = true;
      }
    } else {
      collapsed += c;
      last_was_space = false;
    }
  }

  // Truncate if needed
  if (collapsed.length() > static_cast<size_t>(config_.max_text_length)) {
    collapsed = collapsed.substr(0, config_.max_text_length) + "...";
  }

  return collapsed;
}

std::string DOMExtractor::GetRole(const blink::WebElement& element) const {
  // Check explicit role
  std::string role = element.GetAttribute("role").Utf8();
  if (!role.empty())
    return role;

  // Infer from tag
  std::string tag = element.TagName().Utf8();
  auto it = kTagToRole.find(tag);
  if (it != kTagToRole.end())
    return it->second;

  // Special cases for input types
  if (tag == "INPUT") {
    std::string type = element.GetAttribute("type").Utf8();
    if (type == "checkbox")
      return "checkbox";
    if (type == "radio")
      return "radio";
    if (type == "submit" || type == "button")
      return "button";
    if (type == "search")
      return "searchbox";
    return "textbox";
  }

  return "";
}

std::vector<std::string> DOMExtractor::GetEventListeners(
    const blink::WebElement& element) const {
  std::vector<std::string> listeners;

  // Check inline event handlers
  static const char* kEventAttrs[] = {
      "onclick",     "ondblclick", "onmousedown", "onmouseup",
      "onmouseover", "onmouseout", "onmousemove", "onkeydown",
      "onkeyup",     "onkeypress", "onfocus",     "onblur",
      "onchange",    "onsubmit",   "oninput",     "ontouchstart",
      "ontouchend",  "ontouchmove"};

  for (const char* attr : kEventAttrs) {
    if (element.HasAttribute(attr)) {
      // Remove "on" prefix
      listeners.push_back(attr + 2);
    }
  }

  return listeners;
}

// =============================================================================
// Static Methods
// =============================================================================

std::string DOMExtractor::GenerateSelector(const blink::WebElement& element) {
  if (element.IsNull())
    return "";

  // Try ID first (most specific)
  std::string id = element.GetAttribute("id").Utf8();
  if (!id.empty() && id.find(' ') == std::string::npos &&
      id.find('.') == std::string::npos) {
    return "#" + id;
  }

  // Build path selector
  std::vector<std::string> path;
  blink::WebElement current = element;

  while (!current.IsNull() && path.size() < 5) {
    std::string part = current.TagName().Utf8();

    // Try to add ID
    std::string curr_id = current.GetAttribute("id").Utf8();
    if (!curr_id.empty() && curr_id.find(' ') == std::string::npos) {
      path.insert(path.begin(), "#" + curr_id);
      break;  // ID is unique enough
    }

    // Add first meaningful class
    std::string classes = current.GetAttribute("class").Utf8();
    if (!classes.empty()) {
      std::vector<std::string> class_list = base::SplitString(
          classes, " ", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
      for (const auto& cls : class_list) {
        // Skip utility classes
        if (cls.length() > 2 && cls.find('-') != 0) {
          part += "." + cls;
          break;
        }
      }
    }

    // Add nth-of-type if needed for uniqueness
    int index = 1;
    blink::WebNode sibling = current.PreviousSibling();
    while (!sibling.IsNull()) {
      if (sibling.IsElementNode()) {
        blink::WebElement sib_elem = sibling.To<blink::WebElement>();
        if (sib_elem.TagName() == current.TagName()) {
          index++;
        }
      }
      sibling = sibling.PreviousSibling();
    }
    if (index > 1) {
      part += ":nth-of-type(" + base::NumberToString(index) + ")";
    }

    path.insert(path.begin(), part);

    blink::WebNode parent = current.ParentNode();
    if (parent.IsNull() || !parent.IsElementNode())
      break;
    current = parent.To<blink::WebElement>();

    // Stop at body
    if (current.TagName().Utf8() == "BODY")
      break;
  }

  return base::JoinString(path, " > ");
}

std::string DOMExtractor::GenerateXPath(const blink::WebElement& element) {
  if (element.IsNull())
    return "";

  std::vector<std::string> path;
  blink::WebElement current = element;

  while (!current.IsNull()) {
    std::string tag = current.TagName().Utf8();
    std::transform(tag.begin(), tag.end(), tag.begin(), ::tolower);

    // Check for ID
    std::string id = current.GetAttribute("id").Utf8();
    if (!id.empty()) {
      path.insert(path.begin(), "//" + tag + "[@id='" + id + "']");
      break;
    }

    // Count preceding siblings with same tag
    int index = 1;
    blink::WebNode sibling = current.PreviousSibling();
    while (!sibling.IsNull()) {
      if (sibling.IsElementNode()) {
        blink::WebElement sib_elem = sibling.To<blink::WebElement>();
        if (sib_elem.TagName() == current.TagName()) {
          index++;
        }
      }
      sibling = sibling.PreviousSibling();
    }

    path.insert(path.begin(), tag + "[" + base::NumberToString(index) + "]");

    blink::WebNode parent = current.ParentNode();
    if (parent.IsNull() || !parent.IsElementNode())
      break;
    current = parent.To<blink::WebElement>();

    if (current.TagName().Utf8() == "HTML")
      break;
  }

  if (path.empty())
    return "";

  // Check if we found an ID
  if (path[0].find("//") == 0)
    return base::JoinString(path, "/");

  return "/html/body/" + base::JoinString(path, "/");
}

}  // namespace atlas
