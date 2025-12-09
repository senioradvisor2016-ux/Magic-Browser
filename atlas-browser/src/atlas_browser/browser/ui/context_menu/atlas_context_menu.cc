// Copyright 2025 Atlas Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license.

#include "atlas_browser/browser/ui/context_menu/atlas_context_menu.h"

#include "atlas_browser/features/llm_agent/browser/agent_service.h"
#include "atlas_browser/features/privacy/tracker_blocker.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "content/public/browser/web_contents.h"

namespace atlas {

AtlasContextMenuObserver::AtlasContextMenuObserver(
    content::WebContents* web_contents,
    Browser* browser)
    : web_contents_(web_contents),
      browser_(browser) {
  Profile* profile = Profile::FromBrowserContext(
      web_contents->GetBrowserContext());
  agent_service_ = AgentService::GetForBrowserContext(profile);
}

AtlasContextMenuObserver::~AtlasContextMenuObserver() = default;

void AtlasContextMenuObserver::InitMenu(
    const content::ContextMenuParams& params) {
  params_ = params;
  AddAgentMenuItems(params);
  AddPrivacyMenuItems();
}

void AtlasContextMenuObserver::AddAgentMenuItems(
    const content::ContextMenuParams& params) {
  // Check if we have selected text
  bool has_selection = !params.selection_text.empty();
  
  // Check context type
  bool is_link = !params.link_url.is_empty();
  bool is_image = params.has_image_contents;

  // Create Atlas submenu
  ui::SimpleMenuModel* atlas_menu = new ui::SimpleMenuModel(this);

  if (has_selection) {
    // Selection-specific actions
    atlas_menu->AddItem(IDC_ATLAS_AGENT_EXPLAIN_SELECTION,
                        u"🤖 Explain Selection");
    atlas_menu->AddItem(IDC_ATLAS_AGENT_SUMMARIZE_SELECTION,
                        u"📝 Summarize Selection");
    atlas_menu->AddItem(IDC_ATLAS_AGENT_TRANSLATE_SELECTION,
                        u"🌐 Translate Selection");
    atlas_menu->AddItem(IDC_ATLAS_AGENT_SEARCH_SELECTION,
                        u"🔍 Search for Selection");
    atlas_menu->AddSeparator(ui::NORMAL_SEPARATOR);
  }

  if (is_link) {
    // Link-specific actions
    atlas_menu->AddItem(IDC_ATLAS_AGENT_SUMMARIZE_LINK,
                        u"📄 Summarize Linked Page");
    atlas_menu->AddItem(IDC_ATLAS_AGENT_OPEN_AND_ANALYZE,
                        u"🔬 Open & Analyze");
    atlas_menu->AddSeparator(ui::NORMAL_SEPARATOR);
  }

  if (is_image) {
    // Image-specific actions
    atlas_menu->AddItem(IDC_ATLAS_AGENT_DESCRIBE_IMAGE,
                        u"🖼️ Describe Image");
    atlas_menu->AddItem(IDC_ATLAS_AGENT_EXTRACT_TEXT_IMAGE,
                        u"📝 Extract Text from Image");
    atlas_menu->AddSeparator(ui::NORMAL_SEPARATOR);
  }

  // General page actions (always available)
  atlas_menu->AddItem(IDC_ATLAS_AGENT_SUMMARIZE_PAGE,
                      u"📄 Summarize This Page");
  atlas_menu->AddItem(IDC_ATLAS_AGENT_EXTRACT_DATA,
                      u"📊 Extract Page Data");
  atlas_menu->AddItem(IDC_ATLAS_AGENT_FILL_FORMS,
                      u"📝 Auto-fill Forms");
  atlas_menu->AddSeparator(ui::NORMAL_SEPARATOR);
  atlas_menu->AddItem(IDC_ATLAS_AGENT_CUSTOM_TASK,
                      u"✨ Custom AI Task...");

  // Add submenu to main menu
  // Note: In real implementation, this would add to the actual context menu
}

void AtlasContextMenuObserver::AddPrivacyMenuItems() {
  // Privacy-related context menu items
  // These would be added to a "Privacy" submenu
}

bool AtlasContextMenuObserver::IsCommandIdSupported(int command_id) {
  return command_id >= kAtlasContextMenuBase &&
         command_id < kAtlasContextMenuBase + 100;
}

bool AtlasContextMenuObserver::IsCommandIdChecked(int command_id) {
  return false;
}

bool AtlasContextMenuObserver::IsCommandIdEnabled(int command_id) {
  if (!agent_service_) {
    return false;
  }

  switch (command_id) {
    case IDC_ATLAS_AGENT_EXPLAIN_SELECTION:
    case IDC_ATLAS_AGENT_SUMMARIZE_SELECTION:
    case IDC_ATLAS_AGENT_TRANSLATE_SELECTION:
    case IDC_ATLAS_AGENT_SEARCH_SELECTION:
      return !params_.selection_text.empty();

    case IDC_ATLAS_AGENT_SUMMARIZE_LINK:
    case IDC_ATLAS_AGENT_OPEN_AND_ANALYZE:
      return !params_.link_url.is_empty();

    case IDC_ATLAS_AGENT_DESCRIBE_IMAGE:
    case IDC_ATLAS_AGENT_EXTRACT_TEXT_IMAGE:
      return params_.has_image_contents;

    case IDC_ATLAS_AGENT_SUMMARIZE_PAGE:
    case IDC_ATLAS_AGENT_EXTRACT_DATA:
    case IDC_ATLAS_AGENT_FILL_FORMS:
    case IDC_ATLAS_AGENT_CUSTOM_TASK:
      return true;

    default:
      return false;
  }
}

void AtlasContextMenuObserver::ExecuteCommand(int command_id) {
  switch (command_id) {
    case IDC_ATLAS_AGENT_EXPLAIN_SELECTION:
      ExecuteAgentOnSelection("Explain this text in simple terms: \"${TEXT}\"");
      break;

    case IDC_ATLAS_AGENT_SUMMARIZE_SELECTION:
      ExecuteAgentOnSelection("Summarize this text: \"${TEXT}\"");
      break;

    case IDC_ATLAS_AGENT_TRANSLATE_SELECTION:
      ExecuteAgentOnSelection(
          "Translate this text to English: \"${TEXT}\"");
      break;

    case IDC_ATLAS_AGENT_SEARCH_SELECTION:
      ExecuteAgentOnSelection(
          "Search the web for more information about: \"${TEXT}\"");
      break;

    case IDC_ATLAS_AGENT_SUMMARIZE_PAGE:
      ExecuteAgentOnPage(
          "Summarize the main content of this page in 3-5 bullet points.");
      break;

    case IDC_ATLAS_AGENT_EXTRACT_DATA:
      ExecuteAgentOnPage(
          "Extract all important structured data from this page as JSON.");
      break;

    case IDC_ATLAS_AGENT_FILL_FORMS:
      ExecuteAgentOnPage(
          "Find and fill out any forms on this page with reasonable test data.");
      break;

    case IDC_ATLAS_AGENT_CUSTOM_TASK:
      // Would show a dialog to enter custom task
      // For now, just start with a placeholder
      ExecuteAgentOnPage("Analyze this page and tell me what's interesting.");
      break;

    case IDC_ATLAS_AGENT_SUMMARIZE_LINK:
      ExecuteAgentOnLink(params_.link_url,
          "Navigate to this link and summarize its content.");
      break;

    case IDC_ATLAS_AGENT_OPEN_AND_ANALYZE:
      ExecuteAgentOnLink(params_.link_url,
          "Navigate to this link and analyze its content thoroughly.");
      break;

    case IDC_ATLAS_AGENT_DESCRIBE_IMAGE:
      ExecuteAgentOnImage(params_.src_url,
          "Describe what's in this image in detail.");
      break;

    case IDC_ATLAS_AGENT_EXTRACT_TEXT_IMAGE:
      ExecuteAgentOnImage(params_.src_url,
          "Extract any text visible in this image (OCR).");
      break;

    case IDC_ATLAS_PRIVACY_BLOCK_DOMAIN:
      if (!params_.page_url.is_empty()) {
        TrackerBlocker::GetInstance()->AddBlockedDomain(
            params_.page_url.host());
      }
      break;

    case IDC_ATLAS_PRIVACY_ALLOW_DOMAIN:
      if (!params_.page_url.is_empty()) {
        TrackerBlocker::GetInstance()->RemoveBlockedDomain(
            params_.page_url.host());
      }
      break;
  }
}

void AtlasContextMenuObserver::ExecuteAgentOnSelection(
    const std::string& task_template) {
  if (!agent_service_ || !web_contents_) {
    return;
  }

  std::string selected_text = GetSelectedText();
  if (selected_text.empty()) {
    return;
  }

  // Replace placeholder with actual selection
  std::string task = task_template;
  base::ReplaceSubstringsAfterOffset(&task, 0, "${TEXT}", selected_text);

  agent_service_->StartTask(
      web_contents_, task,
      base::BindOnce([](agent::mojom::TaskResultPtr result) {
        LOG(INFO) << "Selection task completed";
      }));
}

void AtlasContextMenuObserver::ExecuteAgentOnPage(const std::string& task) {
  if (!agent_service_ || !web_contents_) {
    return;
  }

  agent_service_->StartTask(
      web_contents_, task,
      base::BindOnce([](agent::mojom::TaskResultPtr result) {
        LOG(INFO) << "Page task completed";
      }));
}

void AtlasContextMenuObserver::ExecuteAgentOnLink(const GURL& url,
                                                   const std::string& task) {
  if (!agent_service_ || !web_contents_ || !url.is_valid()) {
    return;
  }

  // Prepend navigation instruction
  std::string full_task = "Navigate to " + url.spec() + ", then " + task;

  agent_service_->StartTask(
      web_contents_, full_task,
      base::BindOnce([](agent::mojom::TaskResultPtr result) {
        LOG(INFO) << "Link task completed";
      }));
}

void AtlasContextMenuObserver::ExecuteAgentOnImage(const GURL& url,
                                                    const std::string& task) {
  if (!agent_service_ || !web_contents_) {
    return;
  }

  // For images, we'd need special handling
  // The agent would need to capture/analyze the specific image
  std::string full_task = task;
  if (url.is_valid()) {
    full_task += " (Image URL: " + url.spec() + ")";
  }

  agent_service_->StartTask(
      web_contents_, full_task,
      base::BindOnce([](agent::mojom::TaskResultPtr result) {
        LOG(INFO) << "Image task completed";
      }));
}

std::string AtlasContextMenuObserver::GetSelectedText() const {
  return base::UTF16ToUTF8(params_.selection_text);
}

}  // namespace atlas
