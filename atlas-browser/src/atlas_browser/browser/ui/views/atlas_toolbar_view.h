// Copyright 2025 Atlas Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license.

#ifndef ATLAS_BROWSER_BROWSER_UI_VIEWS_ATLAS_TOOLBAR_VIEW_H_
#define ATLAS_BROWSER_BROWSER_UI_VIEWS_ATLAS_TOOLBAR_VIEW_H_

#include "base/memory/raw_ptr.h"
#include "chrome/browser/ui/views/toolbar/toolbar_view.h"
#include "ui/base/metadata/metadata_header_macros.h"

class Browser;

namespace views {
class ImageButton;
class MdTextButton;
}  // namespace views

namespace atlas {

class AgentPanelView;
class AgentStatusIndicator;

// AtlasToolbarView extends Chrome's ToolbarView to add Atlas-specific
// buttons, including the AI Agent button.
class AtlasToolbarView : public ToolbarView {
  METADATA_HEADER(AtlasToolbarView, ToolbarView)

 public:
  explicit AtlasToolbarView(Browser* browser);
  ~AtlasToolbarView() override;

  AtlasToolbarView(const AtlasToolbarView&) = delete;
  AtlasToolbarView& operator=(const AtlasToolbarView&) = delete;

  // ToolbarView overrides
  void Init() override;
  void Layout(PassKey) override;

  // Show/hide the agent panel
  void ToggleAgentPanel();
  bool IsAgentPanelVisible() const;

  // Update agent status indicator
  void SetAgentStatus(bool is_running, const std::string& status_text);

  // Get the agent button for testing
  views::Button* GetAgentButtonForTesting() { return agent_button_; }

 private:
  // Button click handlers
  void OnAgentButtonClicked();
  void OnPrivacyButtonClicked();

  // Create Atlas-specific buttons
  void CreateAtlasButtons();

  // Update button states
  void UpdateAgentButtonState();

  // The agent panel (shown/hidden)
  std::unique_ptr<AgentPanelView> agent_panel_;

  // Atlas-specific buttons
  raw_ptr<views::ImageButton> agent_button_ = nullptr;
  raw_ptr<views::ImageButton> privacy_button_ = nullptr;
  raw_ptr<AgentStatusIndicator> agent_status_ = nullptr;

  // Is agent panel currently visible
  bool agent_panel_visible_ = false;
};

// =============================================================================
// AgentStatusIndicator
// =============================================================================

// Small indicator showing agent status (thinking, acting, idle)
class AgentStatusIndicator : public views::View {
  METADATA_HEADER(AgentStatusIndicator, views::View)

 public:
  enum class Status {
    kIdle,
    kThinking,
    kActing,
    kError,
  };

  AgentStatusIndicator();
  ~AgentStatusIndicator() override;

  void SetStatus(Status status);
  void SetStatusText(const std::string& text);

  // views::View overrides
  void OnPaint(gfx::Canvas* canvas) override;
  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;

 private:
  Status status_ = Status::kIdle;
  std::string status_text_;
};

}  // namespace atlas

#endif  // ATLAS_BROWSER_BROWSER_UI_VIEWS_ATLAS_TOOLBAR_VIEW_H_
