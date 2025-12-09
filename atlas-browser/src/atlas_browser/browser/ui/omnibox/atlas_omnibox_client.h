// Copyright 2025 Atlas Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license.

#ifndef ATLAS_BROWSER_BROWSER_UI_OMNIBOX_ATLAS_OMNIBOX_CLIENT_H_
#define ATLAS_BROWSER_BROWSER_UI_OMNIBOX_ATLAS_OMNIBOX_CLIENT_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "chrome/browser/ui/omnibox/chrome_omnibox_client.h"

class Browser;

namespace atlas {

class AgentService;

// AtlasOmniboxClient extends Chrome's omnibox client to add
// Atlas-specific functionality like AI agent commands.
class AtlasOmniboxClient : public ChromeOmniboxClient {
 public:
  AtlasOmniboxClient(LocationBar* location_bar,
                     Browser* browser,
                     Profile* profile);
  ~AtlasOmniboxClient() override;

  AtlasOmniboxClient(const AtlasOmniboxClient&) = delete;
  AtlasOmniboxClient& operator=(const AtlasOmniboxClient&) = delete;

  // OmniboxClient overrides
  void OnInputStateChanged() override;
  void OnFocusChanged(OmniboxFocusState state,
                      OmniboxFocusChangeReason reason) override;

  // Check if input is an Atlas command
  bool IsAtlasCommand(const std::u16string& input) const;

  // Handle Atlas-specific commands
  bool HandleAtlasCommand(const std::u16string& input);

  // Get agent-related suggestions
  struct AgentSuggestion {
    std::u16string text;
    std::u16string description;
    std::string icon;
  };
  std::vector<AgentSuggestion> GetAgentSuggestions(
      const std::u16string& input) const;

 private:
  // Parse command from input (e.g., "@agent search for...")
  std::string ParseAgentCommand(const std::u16string& input) const;

  // Start agent task from omnibox
  void StartAgentTask(const std::string& task);

  // Quick actions
  void HandleQuickAction(const std::string& action);

  raw_ptr<Browser> browser_;
  raw_ptr<AgentService> agent_service_;
};

// =============================================================================
// Atlas Omnibox Commands
// =============================================================================

namespace omnibox_commands {

// Command prefixes
constexpr char16_t kAgentPrefix[] = u"@agent ";
constexpr char16_t kAskPrefix[] = u"@ask ";
constexpr char16_t kDoPrefix[] = u"@do ";
constexpr char16_t kFindPrefix[] = u"@find ";

// Quick action commands
constexpr char16_t kSummarizeCommand[] = u"@summarize";
constexpr char16_t kTranslateCommand[] = u"@translate";
constexpr char16_t kExtractCommand[] = u"@extract";

// Check if input starts with any Atlas command
bool IsAtlasPrefix(const std::u16string& input);

// Get command type from input
enum class CommandType {
  kNone,
  kAgent,    // @agent - full agent task
  kAsk,      // @ask - query about page
  kDo,       // @do - single action
  kFind,     // @find - find on page
  kSummarize,
  kTranslate,
  kExtract,
};

CommandType GetCommandType(const std::u16string& input);

// Extract the query/task from command input
std::u16string ExtractQuery(const std::u16string& input);

}  // namespace omnibox_commands

}  // namespace atlas

#endif  // ATLAS_BROWSER_BROWSER_UI_OMNIBOX_ATLAS_OMNIBOX_CLIENT_H_
