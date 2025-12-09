// Copyright 2025 Atlas Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license.

#include "atlas_browser/browser/ui/omnibox/atlas_omnibox_client.h"

#include "atlas_browser/features/llm_agent/browser/agent_service.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"

namespace atlas {

// =============================================================================
// AtlasOmniboxClient
// =============================================================================

AtlasOmniboxClient::AtlasOmniboxClient(LocationBar* location_bar,
                                       Browser* browser,
                                       Profile* profile)
    : ChromeOmniboxClient(location_bar, browser, profile),
      browser_(browser) {
  agent_service_ = AgentService::GetForBrowserContext(profile);
}

AtlasOmniboxClient::~AtlasOmniboxClient() = default;

void AtlasOmniboxClient::OnInputStateChanged() {
  ChromeOmniboxClient::OnInputStateChanged();
  // Could add Atlas-specific input handling here
}

void AtlasOmniboxClient::OnFocusChanged(OmniboxFocusState state,
                                        OmniboxFocusChangeReason reason) {
  ChromeOmniboxClient::OnFocusChanged(state, reason);
}

bool AtlasOmniboxClient::IsAtlasCommand(const std::u16string& input) const {
  return omnibox_commands::IsAtlasPrefix(input);
}

bool AtlasOmniboxClient::HandleAtlasCommand(const std::u16string& input) {
  if (!IsAtlasCommand(input)) {
    return false;
  }

  auto command_type = omnibox_commands::GetCommandType(input);
  std::u16string query = omnibox_commands::ExtractQuery(input);
  std::string query_utf8 = base::UTF16ToUTF8(query);

  switch (command_type) {
    case omnibox_commands::CommandType::kAgent:
      StartAgentTask(query_utf8);
      return true;

    case omnibox_commands::CommandType::kAsk:
      // Query the page
      if (agent_service_) {
        auto* web_contents = 
            browser_->tab_strip_model()->GetActiveWebContents();
        if (web_contents) {
          agent_service_->QueryPage(
              web_contents, query_utf8,
              base::BindOnce([](const std::string& answer) {
                // Show answer in UI
                LOG(INFO) << "Answer: " << answer;
              }));
        }
      }
      return true;

    case omnibox_commands::CommandType::kDo:
      // Single action
      StartAgentTask("Perform this single action: " + query_utf8);
      return true;

    case omnibox_commands::CommandType::kFind:
      // Enhanced find in page
      // Could use agent to find semantically
      return false;  // Fall back to regular find

    case omnibox_commands::CommandType::kSummarize:
      HandleQuickAction("summarize");
      return true;

    case omnibox_commands::CommandType::kTranslate:
      HandleQuickAction("translate");
      return true;

    case omnibox_commands::CommandType::kExtract:
      HandleQuickAction("extract");
      return true;

    default:
      return false;
  }
}

std::vector<AtlasOmniboxClient::AgentSuggestion>
AtlasOmniboxClient::GetAgentSuggestions(const std::u16string& input) const {
  std::vector<AgentSuggestion> suggestions;

  // Only suggest if input starts with @
  if (input.empty() || input[0] != u'@') {
    return suggestions;
  }

  // Add relevant suggestions based on input
  std::u16string lower_input = base::ToLowerASCII(input);

  if (base::StartsWith(lower_input, u"@a")) {
    suggestions.push_back({
        u"@agent ",
        u"Run AI agent task on this page",
        "agent_icon"
    });
    suggestions.push_back({
        u"@ask ",
        u"Ask a question about this page",
        "question_icon"
    });
  }

  if (base::StartsWith(lower_input, u"@d")) {
    suggestions.push_back({
        u"@do ",
        u"Perform a single action",
        "action_icon"
    });
  }

  if (base::StartsWith(lower_input, u"@s")) {
    suggestions.push_back({
        u"@summarize",
        u"Summarize this page",
        "summarize_icon"
    });
  }

  if (base::StartsWith(lower_input, u"@t")) {
    suggestions.push_back({
        u"@translate",
        u"Translate this page",
        "translate_icon"
    });
  }

  if (base::StartsWith(lower_input, u"@e")) {
    suggestions.push_back({
        u"@extract",
        u"Extract data from this page",
        "extract_icon"
    });
  }

  if (base::StartsWith(lower_input, u"@f")) {
    suggestions.push_back({
        u"@find ",
        u"Smart find on this page",
        "find_icon"
    });
  }

  return suggestions;
}

std::string AtlasOmniboxClient::ParseAgentCommand(
    const std::u16string& input) const {
  return base::UTF16ToUTF8(omnibox_commands::ExtractQuery(input));
}

void AtlasOmniboxClient::StartAgentTask(const std::string& task) {
  if (!agent_service_ || task.empty()) {
    return;
  }

  auto* web_contents = browser_->tab_strip_model()->GetActiveWebContents();
  if (!web_contents) {
    return;
  }

  LOG(INFO) << "Starting agent task from omnibox: " << task;

  agent_service_->StartTask(
      web_contents, task,
      base::BindOnce([](agent::mojom::TaskResultPtr result) {
        LOG(INFO) << "Agent task completed with status: "
                  << static_cast<int>(result->status);
      }));
}

void AtlasOmniboxClient::HandleQuickAction(const std::string& action) {
  if (!agent_service_) {
    return;
  }

  auto* web_contents = browser_->tab_strip_model()->GetActiveWebContents();
  if (!web_contents) {
    return;
  }

  std::string task;
  if (action == "summarize") {
    task = "Summarize the main content of this page in 3-5 bullet points.";
  } else if (action == "translate") {
    task = "Translate the main content of this page to English.";
  } else if (action == "extract") {
    task = "Extract all important data from this page as structured JSON.";
  } else {
    return;
  }

  StartAgentTask(task);
}

// =============================================================================
// Omnibox Commands
// =============================================================================

namespace omnibox_commands {

bool IsAtlasPrefix(const std::u16string& input) {
  if (input.empty()) {
    return false;
  }
  return input[0] == u'@';
}

CommandType GetCommandType(const std::u16string& input) {
  std::u16string lower = base::ToLowerASCII(input);

  if (base::StartsWith(lower, kAgentPrefix)) {
    return CommandType::kAgent;
  }
  if (base::StartsWith(lower, kAskPrefix)) {
    return CommandType::kAsk;
  }
  if (base::StartsWith(lower, kDoPrefix)) {
    return CommandType::kDo;
  }
  if (base::StartsWith(lower, kFindPrefix)) {
    return CommandType::kFind;
  }
  if (base::StartsWith(lower, kSummarizeCommand)) {
    return CommandType::kSummarize;
  }
  if (base::StartsWith(lower, kTranslateCommand)) {
    return CommandType::kTranslate;
  }
  if (base::StartsWith(lower, kExtractCommand)) {
    return CommandType::kExtract;
  }

  return CommandType::kNone;
}

std::u16string ExtractQuery(const std::u16string& input) {
  // Find the space after the command prefix
  size_t space_pos = input.find(u' ');
  if (space_pos == std::u16string::npos) {
    return std::u16string();
  }

  // Return everything after the space, trimmed
  std::u16string query = input.substr(space_pos + 1);
  base::TrimWhitespace(query, base::TRIM_ALL, &query);
  return query;
}

}  // namespace omnibox_commands

}  // namespace atlas
