// Copyright 2025 Atlas Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license.

#ifndef ATLAS_BROWSER_BROWSER_UI_CONTEXT_MENU_ATLAS_CONTEXT_MENU_H_
#define ATLAS_BROWSER_BROWSER_UI_CONTEXT_MENU_ATLAS_CONTEXT_MENU_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "chrome/browser/renderer_context_menu/render_view_context_menu.h"

class Browser;

namespace content {
class WebContents;
}

namespace atlas {

class AgentService;

// AtlasContextMenuObserver adds Atlas-specific items to the context menu.
class AtlasContextMenuObserver : public RenderViewContextMenuObserver {
 public:
  AtlasContextMenuObserver(content::WebContents* web_contents,
                           Browser* browser);
  ~AtlasContextMenuObserver() override;

  AtlasContextMenuObserver(const AtlasContextMenuObserver&) = delete;
  AtlasContextMenuObserver& operator=(const AtlasContextMenuObserver&) = delete;

  // RenderViewContextMenuObserver implementation
  void InitMenu(const content::ContextMenuParams& params) override;
  bool IsCommandIdSupported(int command_id) override;
  bool IsCommandIdChecked(int command_id) override;
  bool IsCommandIdEnabled(int command_id) override;
  void ExecuteCommand(int command_id) override;

 private:
  // Add Atlas menu items based on context
  void AddAgentMenuItems(const content::ContextMenuParams& params);
  void AddPrivacyMenuItems();

  // Execute agent commands
  void ExecuteAgentOnSelection(const std::string& task_template);
  void ExecuteAgentOnPage(const std::string& task);
  void ExecuteAgentOnLink(const GURL& url, const std::string& task);
  void ExecuteAgentOnImage(const GURL& url, const std::string& task);

  // Get selected text
  std::string GetSelectedText() const;

  raw_ptr<content::WebContents> web_contents_;
  raw_ptr<Browser> browser_;
  raw_ptr<AgentService> agent_service_;

  // Current context menu params
  content::ContextMenuParams params_;
};

// =============================================================================
// Atlas Context Menu Command IDs
// =============================================================================

// These extend the base command IDs defined in atlas_command_ids.h
// Starting at a high offset to avoid conflicts

constexpr int kAtlasContextMenuBase = 55000;

// Selection context
constexpr int IDC_ATLAS_AGENT_EXPLAIN_SELECTION = kAtlasContextMenuBase + 1;
constexpr int IDC_ATLAS_AGENT_SUMMARIZE_SELECTION = kAtlasContextMenuBase + 2;
constexpr int IDC_ATLAS_AGENT_TRANSLATE_SELECTION = kAtlasContextMenuBase + 3;
constexpr int IDC_ATLAS_AGENT_SEARCH_SELECTION = kAtlasContextMenuBase + 4;

// Page context
constexpr int IDC_ATLAS_AGENT_SUMMARIZE_PAGE = kAtlasContextMenuBase + 10;
constexpr int IDC_ATLAS_AGENT_EXTRACT_DATA = kAtlasContextMenuBase + 11;
constexpr int IDC_ATLAS_AGENT_FILL_FORMS = kAtlasContextMenuBase + 12;
constexpr int IDC_ATLAS_AGENT_CUSTOM_TASK = kAtlasContextMenuBase + 13;

// Link context
constexpr int IDC_ATLAS_AGENT_SUMMARIZE_LINK = kAtlasContextMenuBase + 20;
constexpr int IDC_ATLAS_AGENT_OPEN_AND_ANALYZE = kAtlasContextMenuBase + 21;

// Image context
constexpr int IDC_ATLAS_AGENT_DESCRIBE_IMAGE = kAtlasContextMenuBase + 30;
constexpr int IDC_ATLAS_AGENT_EXTRACT_TEXT_IMAGE = kAtlasContextMenuBase + 31;

// Privacy context
constexpr int IDC_ATLAS_PRIVACY_BLOCK_DOMAIN = kAtlasContextMenuBase + 40;
constexpr int IDC_ATLAS_PRIVACY_ALLOW_DOMAIN = kAtlasContextMenuBase + 41;

}  // namespace atlas

#endif  // ATLAS_BROWSER_BROWSER_UI_CONTEXT_MENU_ATLAS_CONTEXT_MENU_H_
