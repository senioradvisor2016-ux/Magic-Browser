// Copyright 2025 Atlas Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license.

#include "atlas_browser/browser/ui/views/atlas_toolbar_view.h"

#include "atlas_browser/browser/ui/views/agent_panel_view.h"
#include "atlas_browser/features/llm_agent/browser/agent_service.h"
#include "base/functional/bind.h"
#include "chrome/app/vector_icons/vector_icons.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/view_ids.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/button/image_button_factory.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/widget/widget.h"

namespace atlas {

namespace {

// Button sizes
constexpr int kButtonSize = 28;
constexpr int kButtonPadding = 4;

// Status indicator sizes
constexpr int kStatusIndicatorSize = 8;

// Colors
constexpr SkColor kAgentActiveColor = SkColorSetRGB(0x4C, 0xAF, 0x50);
constexpr SkColor kAgentThinkingColor = SkColorSetRGB(0xFF, 0x98, 0x00);
constexpr SkColor kAgentErrorColor = SkColorSetRGB(0xF4, 0x43, 0x36);
constexpr SkColor kAgentIdleColor = SkColorSetRGB(0x9E, 0x9E, 0x9E);

}  // namespace

// =============================================================================
// AtlasToolbarView
// =============================================================================

AtlasToolbarView::AtlasToolbarView(Browser* browser) : ToolbarView(browser) {}

AtlasToolbarView::~AtlasToolbarView() = default;

void AtlasToolbarView::Init() {
  ToolbarView::Init();
  CreateAtlasButtons();
}

void AtlasToolbarView::CreateAtlasButtons() {
  // Create agent status indicator
  agent_status_ = AddChildView(std::make_unique<AgentStatusIndicator>());

  // Create agent button
  agent_button_ = AddChildView(views::CreateVectorImageButtonWithNativeTheme(
      base::BindRepeating(&AtlasToolbarView::OnAgentButtonClicked,
                          base::Unretained(this)),
      kSmartAssistantIcon));  // Using existing Chrome icon as placeholder
  agent_button_->SetTooltipText(u"AI Agent (Ctrl+Shift+A)");
  agent_button_->SetAccessibleName(u"AI Agent");
  agent_button_->SetID(VIEW_ID_ATLAS_AGENT_BUTTON);

  // Create privacy shield button
  privacy_button_ = AddChildView(views::CreateVectorImageButtonWithNativeTheme(
      base::BindRepeating(&AtlasToolbarView::OnPrivacyButtonClicked,
                          base::Unretained(this)),
      kSecurityIcon));  // Using existing Chrome icon
  privacy_button_->SetTooltipText(u"Privacy Shield");
  privacy_button_->SetAccessibleName(u"Privacy Shield");
  privacy_button_->SetID(VIEW_ID_ATLAS_PRIVACY_BUTTON);

  UpdateAgentButtonState();
}

void AtlasToolbarView::Layout(PassKey key) {
  ToolbarView::Layout(key);

  // Position Atlas buttons at the right end of the toolbar
  if (agent_status_) {
    int x = width() - kButtonPadding - kButtonSize;

    if (privacy_button_) {
      privacy_button_->SetBounds(x, (height() - kButtonSize) / 2,
                                  kButtonSize, kButtonSize);
      x -= kButtonSize + kButtonPadding;
    }

    if (agent_button_) {
      agent_button_->SetBounds(x, (height() - kButtonSize) / 2,
                                kButtonSize, kButtonSize);
      x -= kButtonPadding;
    }

    if (agent_status_) {
      agent_status_->SetBounds(x - kStatusIndicatorSize,
                                (height() - kStatusIndicatorSize) / 2,
                                kStatusIndicatorSize, kStatusIndicatorSize);
    }
  }
}

void AtlasToolbarView::OnAgentButtonClicked() {
  ToggleAgentPanel();
}

void AtlasToolbarView::OnPrivacyButtonClicked() {
  // TODO: Show privacy stats popup
  // For now, just toggle privacy mode
}

void AtlasToolbarView::ToggleAgentPanel() {
  if (!agent_panel_) {
    agent_panel_ = std::make_unique<AgentPanelView>(browser());
  }

  if (agent_panel_visible_) {
    // Hide panel
    agent_panel_->SetVisible(false);
    agent_panel_visible_ = false;
  } else {
    // Show panel
    // Position panel to the right of the browser window
    auto* widget = GetWidget();
    if (widget) {
      gfx::Rect bounds = widget->GetWindowBoundsInScreen();
      gfx::Point panel_origin(bounds.right() - agent_panel_->width() - 16,
                               bounds.y() + height() + 8);
      agent_panel_->SetBoundsRect(
          gfx::Rect(panel_origin, agent_panel_->GetPreferredSize()));
    }
    agent_panel_->SetVisible(true);
    agent_panel_visible_ = true;
  }

  UpdateAgentButtonState();
}

bool AtlasToolbarView::IsAgentPanelVisible() const {
  return agent_panel_visible_;
}

void AtlasToolbarView::SetAgentStatus(bool is_running,
                                       const std::string& status_text) {
  if (agent_status_) {
    if (is_running) {
      agent_status_->SetStatus(AgentStatusIndicator::Status::kActing);
    } else {
      agent_status_->SetStatus(AgentStatusIndicator::Status::kIdle);
    }
    agent_status_->SetStatusText(status_text);
  }
}

void AtlasToolbarView::UpdateAgentButtonState() {
  if (!agent_button_)
    return;

  // Update button appearance based on panel visibility
  if (agent_panel_visible_) {
    agent_button_->SetTooltipText(u"Hide AI Agent");
  } else {
    agent_button_->SetTooltipText(u"AI Agent (Ctrl+Shift+A)");
  }
}

BEGIN_METADATA(AtlasToolbarView)
END_METADATA

// =============================================================================
// AgentStatusIndicator
// =============================================================================

AgentStatusIndicator::AgentStatusIndicator() {
  SetPreferredSize(gfx::Size(kStatusIndicatorSize, kStatusIndicatorSize));
}

AgentStatusIndicator::~AgentStatusIndicator() = default;

void AgentStatusIndicator::SetStatus(Status status) {
  if (status_ != status) {
    status_ = status;
    SchedulePaint();
  }
}

void AgentStatusIndicator::SetStatusText(const std::string& text) {
  status_text_ = text;
  SetTooltipText(base::UTF8ToUTF16(text));
}

void AgentStatusIndicator::OnPaint(gfx::Canvas* canvas) {
  SkColor color;
  switch (status_) {
    case Status::kThinking:
      color = kAgentThinkingColor;
      break;
    case Status::kActing:
      color = kAgentActiveColor;
      break;
    case Status::kError:
      color = kAgentErrorColor;
      break;
    case Status::kIdle:
    default:
      color = kAgentIdleColor;
      break;
  }

  cc::PaintFlags flags;
  flags.setAntiAlias(true);
  flags.setColor(color);
  flags.setStyle(cc::PaintFlags::kFill_Style);

  gfx::Rect bounds = GetLocalBounds();
  int radius = std::min(bounds.width(), bounds.height()) / 2;
  canvas->DrawCircle(bounds.CenterPoint(), radius, flags);
}

gfx::Size AgentStatusIndicator::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  return gfx::Size(kStatusIndicatorSize, kStatusIndicatorSize);
}

BEGIN_METADATA(AgentStatusIndicator)
END_METADATA

}  // namespace atlas
