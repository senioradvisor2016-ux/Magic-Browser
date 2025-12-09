// Copyright 2025 Atlas Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license.

#ifndef ATLAS_BROWSER_BROWSER_UI_VIEWS_AGENT_PANEL_VIEW_H_
#define ATLAS_BROWSER_BROWSER_UI_VIEWS_AGENT_PANEL_VIEW_H_

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "atlas_browser/features/llm_agent/browser/agent_service.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/view.h"

class Browser;

namespace views {
class Label;
class MdTextButton;
class ScrollView;
class Textfield;
class Textarea;
}  // namespace views

namespace atlas {

// AgentLogEntryView displays a single step in the agent's execution log
class AgentLogEntryView : public views::View {
  METADATA_HEADER(AgentLogEntryView, views::View)

 public:
  explicit AgentLogEntryView(const agent::mojom::AgentStep& step);
  ~AgentLogEntryView() override;

 private:
  raw_ptr<views::Label> step_label_;
  raw_ptr<views::Label> thought_label_;
  raw_ptr<views::Label> action_label_;
  raw_ptr<views::Label> result_label_;
};

// AgentPanelView is the main UI panel for controlling the AI agent
class AgentPanelView : public views::View,
                       public AgentServiceObserver {
  METADATA_HEADER(AgentPanelView, views::View)

 public:
  explicit AgentPanelView(Browser* browser);
  ~AgentPanelView() override;

  AgentPanelView(const AgentPanelView&) = delete;
  AgentPanelView& operator=(const AgentPanelView&) = delete;

  // views::View overrides
  void OnThemeChanged() override;

  // AgentServiceObserver overrides
  void OnTaskStarted(const std::string& task_id,
                     const std::string& description) override;
  void OnTaskProgress(const std::string& task_id,
                      const agent::mojom::AgentStep& step) override;
  void OnTaskCompleted(const std::string& task_id,
                       const agent::mojom::TaskResult& result) override;
  void OnTaskFailed(const std::string& task_id,
                    const std::string& error) override;
  void OnTaskCancelled(const std::string& task_id) override;
  void OnAgentThinking(const std::string& task_id) override;
  void OnAgentActing(const std::string& task_id,
                     const agent::mojom::AgentAction& action) override;

 private:
  // Initialize the UI components
  void InitializeUI();

  // Button handlers
  void OnStartClicked();
  void OnPauseClicked();
  void OnStopClicked();
  void OnSettingsClicked();

  // Update UI state
  void UpdateButtonState(bool is_running);
  void AddLogEntry(const agent::mojom::AgentStep& step);
  void ClearLog();

  // Task completion handler
  void OnTaskComplete(agent::mojom::TaskResultPtr result);

  // The browser this panel belongs to
  raw_ptr<Browser> browser_;

  // UI components
  raw_ptr<views::Label> header_label_;
  raw_ptr<views::Textfield> task_input_;
  raw_ptr<views::MdTextButton> start_button_;
  raw_ptr<views::MdTextButton> pause_button_;
  raw_ptr<views::MdTextButton> stop_button_;
  raw_ptr<views::MdTextButton> settings_button_;
  raw_ptr<views::Label> status_label_;
  raw_ptr<views::ScrollView> log_scroll_view_;
  raw_ptr<views::View> log_container_;

  // Current task state
  std::string current_task_id_;
  bool is_paused_ = false;

  base::WeakPtrFactory<AgentPanelView> weak_factory_{this};
};

}  // namespace atlas

#endif  // ATLAS_BROWSER_BROWSER_UI_VIEWS_AGENT_PANEL_VIEW_H_
