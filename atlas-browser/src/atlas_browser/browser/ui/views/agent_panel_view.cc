// Copyright 2025 Atlas Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license.

#include "atlas_browser/browser/ui/views/agent_panel_view.h"

#include "atlas_browser/features/llm_agent/browser/agent_service.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/gfx/font_list.h"
#include "ui/views/background.h"
#include "ui/views/border.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/scroll_view.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"

namespace atlas {

namespace {

constexpr int kPanelWidth = 380;
constexpr int kPanelPadding = 16;
constexpr int kElementSpacing = 12;

// Colors (will be updated based on theme)
constexpr SkColor kBackgroundColor = SK_ColorWHITE;
constexpr SkColor kSuccessColor = SkColorSetRGB(0x4C, 0xAF, 0x50);
constexpr SkColor kErrorColor = SkColorSetRGB(0xF4, 0x43, 0x36);
constexpr SkColor kWarningColor = SkColorSetRGB(0xFF, 0x98, 0x00);
constexpr SkColor kInfoColor = SkColorSetRGB(0x21, 0x96, 0xF3);

std::u16string ActionTypeToString(agent::mojom::ActionType type) {
  switch (type) {
    case agent::mojom::ActionType::kClick:
      return u"Click";
    case agent::mojom::ActionType::kType:
      return u"Type";
    case agent::mojom::ActionType::kScroll:
      return u"Scroll";
    case agent::mojom::ActionType::kHover:
      return u"Hover";
    case agent::mojom::ActionType::kPressKey:
      return u"Key Press";
    case agent::mojom::ActionType::kNavigate:
      return u"Navigate";
    case agent::mojom::ActionType::kWait:
      return u"Wait";
    case agent::mojom::ActionType::kScreenshot:
      return u"Screenshot";
    case agent::mojom::ActionType::kExtract:
      return u"Extract";
    case agent::mojom::ActionType::kDone:
      return u"Done";
    default:
      return u"Unknown";
  }
}

}  // namespace

// =============================================================================
// AgentLogEntryView
// =============================================================================

AgentLogEntryView::AgentLogEntryView(const agent::mojom::AgentStep& step) {
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets::VH(8, 12), 4));

  SetBackground(views::CreateSolidBackground(SkColorSetRGB(0xF5, 0xF5, 0xF5)));
  SetBorder(views::CreateRoundedRectBorder(
      1, 4, SkColorSetRGB(0xE0, 0xE0, 0xE0)));

  // Step number and status
  std::u16string step_text = u"Step " + base::NumberToString16(step.step_number);
  if (step.result->success) {
    step_text += u" ✓";
  } else {
    step_text += u" ✗";
  }
  step_label_ = AddChildView(std::make_unique<views::Label>(
      step_text,
      views::Label::CustomFont{gfx::FontList().DeriveWithWeight(
          gfx::Font::Weight::BOLD)}));
  step_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  step_label_->SetEnabledColor(
      step.result->success ? kSuccessColor : kErrorColor);

  // Thought
  if (!step.thought.empty()) {
    std::u16string thought_text =
        u"💭 " + base::UTF8ToUTF16(step.thought);
    thought_label_ = AddChildView(std::make_unique<views::Label>(thought_text));
    thought_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
    thought_label_->SetMultiLine(true);
    thought_label_->SetMaximumWidth(kPanelWidth - 2 * kPanelPadding - 24);
  }

  // Action
  std::u16string action_text = u"🎯 " + ActionTypeToString(step.action->type);
  if (step.action->selector) {
    action_text += u" on \"" + base::UTF8ToUTF16(*step.action->selector) + u"\"";
  }
  if (step.action->value) {
    action_text += u" = \"" + base::UTF8ToUTF16(*step.action->value) + u"\"";
  }
  action_label_ = AddChildView(std::make_unique<views::Label>(action_text));
  action_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  action_label_->SetMultiLine(true);
  action_label_->SetMaximumWidth(kPanelWidth - 2 * kPanelPadding - 24);
  action_label_->SetEnabledColor(kInfoColor);

  // Error message if failed
  if (!step.result->success && step.result->error_message) {
    std::u16string error_text =
        u"❌ " + base::UTF8ToUTF16(*step.result->error_message);
    result_label_ = AddChildView(std::make_unique<views::Label>(error_text));
    result_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
    result_label_->SetMultiLine(true);
    result_label_->SetMaximumWidth(kPanelWidth - 2 * kPanelPadding - 24);
    result_label_->SetEnabledColor(kErrorColor);
  }
}

AgentLogEntryView::~AgentLogEntryView() = default;

BEGIN_METADATA(AgentLogEntryView)
END_METADATA

// =============================================================================
// AgentPanelView
// =============================================================================

AgentPanelView::AgentPanelView(Browser* browser) : browser_(browser) {
  InitializeUI();

  // Register as observer
  if (browser_) {
    auto* agent_service =
        AgentService::GetForBrowserContext(browser_->profile());
    if (agent_service) {
      agent_service->AddObserver(this);
    }
  }
}

AgentPanelView::~AgentPanelView() {
  // Unregister as observer
  if (browser_) {
    auto* agent_service =
        AgentService::GetForBrowserContext(browser_->profile());
    if (agent_service) {
      agent_service->RemoveObserver(this);
    }
  }
}

void AgentPanelView::InitializeUI() {
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets(kPanelPadding), kElementSpacing));

  SetBackground(views::CreateSolidBackground(kBackgroundColor));
  SetPreferredSize(gfx::Size(kPanelWidth, 500));

  // Header
  header_label_ = AddChildView(std::make_unique<views::Label>(
      u"🤖 AI Agent",
      views::Label::CustomFont{
          gfx::FontList().DeriveWithSizeDelta(4).DeriveWithWeight(
              gfx::Font::Weight::BOLD)}));
  header_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);

  // Task input section
  auto* input_label = AddChildView(
      std::make_unique<views::Label>(u"What should I do?"));
  input_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);

  task_input_ = AddChildView(std::make_unique<views::Textfield>());
  task_input_->SetPlaceholderText(
      u"e.g., Find the best price for iPhone 16...");
  task_input_->SetAccessibleName(u"Task description");

  // Button row
  auto* button_container = AddChildView(std::make_unique<views::View>());
  button_container->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal, gfx::Insets(), 8));

  start_button_ = button_container->AddChildView(
      std::make_unique<views::MdTextButton>(
          base::BindRepeating(&AgentPanelView::OnStartClicked,
                              base::Unretained(this)),
          u"▶️ Start"));
  start_button_->SetProminent(true);

  pause_button_ = button_container->AddChildView(
      std::make_unique<views::MdTextButton>(
          base::BindRepeating(&AgentPanelView::OnPauseClicked,
                              base::Unretained(this)),
          u"⏸️ Pause"));
  pause_button_->SetEnabled(false);

  stop_button_ = button_container->AddChildView(
      std::make_unique<views::MdTextButton>(
          base::BindRepeating(&AgentPanelView::OnStopClicked,
                              base::Unretained(this)),
          u"⏹️ Stop"));
  stop_button_->SetEnabled(false);
  stop_button_->SetStyle(views::MdTextButton::Style::kAlert);

  settings_button_ = button_container->AddChildView(
      std::make_unique<views::MdTextButton>(
          base::BindRepeating(&AgentPanelView::OnSettingsClicked,
                              base::Unretained(this)),
          u"⚙️"));

  // Status label
  status_label_ = AddChildView(
      std::make_unique<views::Label>(u"Ready to start"));
  status_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  status_label_->SetEnabledColor(kInfoColor);

  // Log section header
  auto* log_header = AddChildView(
      std::make_unique<views::Label>(u"Action Log"));
  log_header->SetHorizontalAlignment(gfx::ALIGN_LEFT);

  // Scroll view for action log
  log_scroll_view_ = AddChildView(std::make_unique<views::ScrollView>());
  log_scroll_view_->SetPreferredSize(
      gfx::Size(kPanelWidth - 2 * kPanelPadding, 280));
  log_scroll_view_->SetBackgroundColor(SkColorSetRGB(0xFA, 0xFA, 0xFA));
  log_scroll_view_->SetBorder(views::CreateRoundedRectBorder(
      1, 4, SkColorSetRGB(0xE0, 0xE0, 0xE0)));

  // Log container inside scroll view
  auto log_content = std::make_unique<views::View>();
  log_content->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets(8), 8));
  log_container_ = log_scroll_view_->SetContents(std::move(log_content));
}

void AgentPanelView::OnThemeChanged() {
  views::View::OnThemeChanged();
  // Update colors based on theme
}

// =============================================================================
// Button Handlers
// =============================================================================

void AgentPanelView::OnStartClicked() {
  if (!browser_)
    return;

  std::string task = base::UTF16ToUTF8(task_input_->GetText());
  if (task.empty()) {
    status_label_->SetText(u"Please enter a task description");
    status_label_->SetEnabledColor(kWarningColor);
    return;
  }

  auto* web_contents =
      browser_->tab_strip_model()->GetActiveWebContents();
  if (!web_contents) {
    status_label_->SetText(u"No active tab");
    status_label_->SetEnabledColor(kErrorColor);
    return;
  }

  auto* agent_service =
      AgentService::GetForBrowserContext(browser_->profile());
  if (!agent_service) {
    status_label_->SetText(u"Agent service not available");
    status_label_->SetEnabledColor(kErrorColor);
    return;
  }

  // Clear previous log
  ClearLog();

  // Start the task
  current_task_id_ = agent_service->StartTask(
      web_contents, task,
      base::BindOnce(&AgentPanelView::OnTaskComplete,
                     weak_factory_.GetWeakPtr()));

  UpdateButtonState(/*is_running=*/true);
  status_label_->SetText(u"🔄 Starting...");
  status_label_->SetEnabledColor(kInfoColor);
}

void AgentPanelView::OnPauseClicked() {
  if (current_task_id_.empty())
    return;

  auto* agent_service =
      AgentService::GetForBrowserContext(browser_->profile());
  if (!agent_service)
    return;

  if (is_paused_) {
    agent_service->ResumeTask(current_task_id_);
    pause_button_->SetText(u"⏸️ Pause");
    status_label_->SetText(u"🔄 Resumed");
    is_paused_ = false;
  } else {
    agent_service->PauseTask(current_task_id_);
    pause_button_->SetText(u"▶️ Resume");
    status_label_->SetText(u"⏸️ Paused");
    is_paused_ = true;
  }
}

void AgentPanelView::OnStopClicked() {
  if (current_task_id_.empty())
    return;

  auto* agent_service =
      AgentService::GetForBrowserContext(browser_->profile());
  if (!agent_service)
    return;

  agent_service->CancelTask(current_task_id_);
  current_task_id_.clear();
  UpdateButtonState(/*is_running=*/false);
  status_label_->SetText(u"⏹️ Stopped");
  status_label_->SetEnabledColor(kWarningColor);
}

void AgentPanelView::OnSettingsClicked() {
  // TODO: Open settings dialog
  status_label_->SetText(u"Settings coming soon...");
}

// =============================================================================
// UI State
// =============================================================================

void AgentPanelView::UpdateButtonState(bool is_running) {
  start_button_->SetEnabled(!is_running);
  pause_button_->SetEnabled(is_running);
  stop_button_->SetEnabled(is_running);
  task_input_->SetEnabled(!is_running);
  is_paused_ = false;
  pause_button_->SetText(u"⏸️ Pause");
}

void AgentPanelView::AddLogEntry(const agent::mojom::AgentStep& step) {
  log_container_->AddChildView(
      std::make_unique<AgentLogEntryView>(step));

  // Scroll to bottom
  log_scroll_view_->ScrollToPosition(
      log_scroll_view_->vertical_scroll_bar(),
      log_scroll_view_->GetVisibleRect().bottom());

  log_container_->InvalidateLayout();
}

void AgentPanelView::ClearLog() {
  log_container_->RemoveAllChildViews();
}

void AgentPanelView::OnTaskComplete(agent::mojom::TaskResultPtr result) {
  current_task_id_.clear();
  UpdateButtonState(/*is_running=*/false);

  if (result->status == agent::mojom::TaskStatus::kCompleted) {
    status_label_->SetText(u"✅ Complete!");
    status_label_->SetEnabledColor(kSuccessColor);
  } else if (result->status == agent::mojom::TaskStatus::kFailed) {
    std::u16string error_text = u"❌ Failed";
    if (result->error_message) {
      error_text += u": " + base::UTF8ToUTF16(*result->error_message);
    }
    status_label_->SetText(error_text);
    status_label_->SetEnabledColor(kErrorColor);
  } else if (result->status == agent::mojom::TaskStatus::kCancelled) {
    status_label_->SetText(u"⏹️ Cancelled");
    status_label_->SetEnabledColor(kWarningColor);
  }
}

// =============================================================================
// AgentServiceObserver
// =============================================================================

void AgentPanelView::OnTaskStarted(const std::string& task_id,
                                   const std::string& description) {
  if (task_id != current_task_id_)
    return;

  status_label_->SetText(u"🔄 Running...");
  status_label_->SetEnabledColor(kInfoColor);
}

void AgentPanelView::OnTaskProgress(const std::string& task_id,
                                    const agent::mojom::AgentStep& step) {
  if (task_id != current_task_id_)
    return;

  AddLogEntry(step);
}

void AgentPanelView::OnTaskCompleted(const std::string& task_id,
                                     const agent::mojom::TaskResult& result) {
  if (task_id != current_task_id_)
    return;

  status_label_->SetText(u"✅ Complete!");
  status_label_->SetEnabledColor(kSuccessColor);
}

void AgentPanelView::OnTaskFailed(const std::string& task_id,
                                  const std::string& error) {
  if (task_id != current_task_id_)
    return;

  std::u16string error_text = u"❌ " + base::UTF8ToUTF16(error);
  status_label_->SetText(error_text);
  status_label_->SetEnabledColor(kErrorColor);
}

void AgentPanelView::OnTaskCancelled(const std::string& task_id) {
  if (task_id != current_task_id_)
    return;

  status_label_->SetText(u"⏹️ Cancelled");
  status_label_->SetEnabledColor(kWarningColor);
}

void AgentPanelView::OnAgentThinking(const std::string& task_id) {
  if (task_id != current_task_id_)
    return;

  status_label_->SetText(u"🤔 Thinking...");
}

void AgentPanelView::OnAgentActing(const std::string& task_id,
                                   const agent::mojom::AgentAction& action) {
  if (task_id != current_task_id_)
    return;

  std::u16string action_text = u"🎯 " + ActionTypeToString(action.type);
  status_label_->SetText(action_text);
}

BEGIN_METADATA(AgentPanelView)
END_METADATA

}  // namespace atlas
