// Copyright 2025 Atlas Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license.

#include "atlas_browser/browser/atlas_keyboard_shortcuts.h"

#include "atlas_browser/browser/atlas_command_ids.h"
#include "build/build_config.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_command_controller.h"
#include "ui/events/event_constants.h"

namespace atlas {

std::vector<AtlasAccelerator> GetAtlasAccelerators() {
  std::vector<AtlasAccelerator> accelerators;

#if BUILDFLAG(IS_MAC)
  // On Mac, use Command instead of Control
  constexpr int kPlatformModifier = ui::EF_COMMAND_DOWN;
#else
  constexpr int kPlatformModifier = ui::EF_CONTROL_DOWN;
#endif

  // Agent shortcuts
  accelerators.push_back({
      IDC_ATLAS_AGENT_TOGGLE,
      shortcuts::kAgentToggleKey,
      kPlatformModifier | ui::EF_SHIFT_DOWN,
      "Toggle AI Agent panel"
  });

  accelerators.push_back({
      IDC_ATLAS_AGENT_STOP,
      shortcuts::kAgentStopKey,
      kPlatformModifier | ui::EF_SHIFT_DOWN,
      "Stop current agent task"
  });

  // Privacy shortcuts
  accelerators.push_back({
      IDC_ATLAS_PRIVACY_TOGGLE,
      shortcuts::kPrivacyToggleKey,
      kPlatformModifier | ui::EF_SHIFT_DOWN,
      "Toggle privacy mode"
  });

  // Settings shortcut
  accelerators.push_back({
      IDC_ATLAS_SETTINGS,
      shortcuts::kSettingsKey,
      kPlatformModifier,
      "Open Atlas settings"
  });

  // Additional useful shortcuts
  
  // Escape - Stop agent task (also)
  accelerators.push_back({
      IDC_ATLAS_AGENT_STOP,
      ui::VKEY_ESCAPE,
      0,
      "Stop agent task"
  });

  // F2 - Toggle element labels (for debugging/agent)
  accelerators.push_back({
      IDC_ATLAS_DEBUG_DOM,
      ui::VKEY_F2,
      0,
      "Toggle element labels"
  });

  return accelerators;
}

void RegisterAtlasAccelerators(Browser* browser) {
  if (!browser)
    return;

  // Get all Atlas accelerators
  auto accelerators = GetAtlasAccelerators();

  // Register each accelerator
  for (const auto& accel : accelerators) {
    ui::Accelerator ui_accel(accel.key_code, accel.modifiers);
    
    // Register with browser's command controller
    // Note: In a real implementation, this would integrate with
    // Chrome's accelerator table system
    
    // For now, we just log the registration
    LOG(INFO) << "Registered Atlas accelerator: " << accel.description
              << " (command " << accel.command_id << ")";
  }
}

// =============================================================================
// Command Handler
// =============================================================================

bool HandleAtlasCommand(Browser* browser, int command_id) {
  switch (command_id) {
    case IDC_ATLAS_AGENT_TOGGLE:
      // Toggle agent panel
      // browser->atlas_toolbar()->ToggleAgentPanel();
      LOG(INFO) << "Toggle agent panel";
      return true;

    case IDC_ATLAS_AGENT_STOP:
      // Stop current agent task
      LOG(INFO) << "Stop agent task";
      return true;

    case IDC_ATLAS_AGENT_PAUSE:
      LOG(INFO) << "Pause agent task";
      return true;

    case IDC_ATLAS_AGENT_RESUME:
      LOG(INFO) << "Resume agent task";
      return true;

    case IDC_ATLAS_PRIVACY_TOGGLE:
      LOG(INFO) << "Toggle privacy mode";
      return true;

    case IDC_ATLAS_SETTINGS:
      // Open Atlas settings page
      // chrome::ShowSettingsSubPage(browser, "atlas");
      LOG(INFO) << "Open Atlas settings";
      return true;

    case IDC_ATLAS_CHECK_UPDATE:
      // Check for updates
      // AtlasUpdateClient::GetInstance()->CheckForUpdates();
      LOG(INFO) << "Check for updates";
      return true;

    case IDC_ATLAS_DEBUG_DOM:
      // Toggle DOM element labels
      LOG(INFO) << "Toggle element labels";
      return true;

    case IDC_ATLAS_DEBUG_SCREENSHOT:
      // Take debug screenshot
      LOG(INFO) << "Take debug screenshot";
      return true;

    default:
      return false;
  }
}

}  // namespace atlas
