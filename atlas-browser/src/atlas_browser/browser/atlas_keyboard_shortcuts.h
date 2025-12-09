// Copyright 2025 Atlas Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license.

#ifndef ATLAS_BROWSER_BROWSER_ATLAS_KEYBOARD_SHORTCUTS_H_
#define ATLAS_BROWSER_BROWSER_ATLAS_KEYBOARD_SHORTCUTS_H_

#include <vector>

#include "ui/base/accelerators/accelerator.h"

namespace atlas {

// Atlas-specific keyboard shortcuts
// These are registered in addition to Chrome's default shortcuts

struct AtlasAccelerator {
  int command_id;
  ui::KeyboardCode key_code;
  int modifiers;  // ui::EF_* flags
  const char* description;
};

// Get all Atlas keyboard shortcuts
std::vector<AtlasAccelerator> GetAtlasAccelerators();

// Register Atlas accelerators with the browser
void RegisterAtlasAccelerators(class Browser* browser);

// Default shortcuts (can be customized by user)
namespace shortcuts {

// Agent shortcuts
// Ctrl+Shift+A (Cmd+Shift+A on Mac) - Toggle agent panel
constexpr ui::KeyboardCode kAgentToggleKey = ui::VKEY_A;
constexpr int kAgentToggleModifiers = 
    ui::EF_CONTROL_DOWN | ui::EF_SHIFT_DOWN;

// Ctrl+Shift+S (Cmd+Shift+S on Mac) - Stop current agent task
constexpr ui::KeyboardCode kAgentStopKey = ui::VKEY_S;
constexpr int kAgentStopModifiers = 
    ui::EF_CONTROL_DOWN | ui::EF_SHIFT_DOWN;

// Privacy shortcuts
// Ctrl+Shift+P (Cmd+Shift+P on Mac) - Toggle privacy mode
constexpr ui::KeyboardCode kPrivacyToggleKey = ui::VKEY_P;
constexpr int kPrivacyToggleModifiers = 
    ui::EF_CONTROL_DOWN | ui::EF_SHIFT_DOWN;

// Settings shortcuts
// Ctrl+, (Cmd+, on Mac) - Open Atlas settings
// Note: This may conflict with Chrome's preferences shortcut on Mac
constexpr ui::KeyboardCode kSettingsKey = ui::VKEY_OEM_COMMA;
constexpr int kSettingsModifiers = ui::EF_CONTROL_DOWN;

}  // namespace shortcuts

}  // namespace atlas

#endif  // ATLAS_BROWSER_BROWSER_ATLAS_KEYBOARD_SHORTCUTS_H_
