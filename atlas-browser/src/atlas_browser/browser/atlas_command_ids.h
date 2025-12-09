// Copyright 2025 Atlas Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license.

#ifndef ATLAS_BROWSER_BROWSER_ATLAS_COMMAND_IDS_H_
#define ATLAS_BROWSER_BROWSER_ATLAS_COMMAND_IDS_H_

// Atlas-specific command IDs
// These are used for keyboard shortcuts and menu items
// Must not conflict with Chrome's command IDs (see chrome/app/chrome_command_ids.h)

// Start at a high number to avoid conflicts
#define ATLAS_COMMAND_ID_BASE 50000

// Agent commands
#define IDC_ATLAS_AGENT_TOGGLE        (ATLAS_COMMAND_ID_BASE + 1)
#define IDC_ATLAS_AGENT_START         (ATLAS_COMMAND_ID_BASE + 2)
#define IDC_ATLAS_AGENT_STOP          (ATLAS_COMMAND_ID_BASE + 3)
#define IDC_ATLAS_AGENT_PAUSE         (ATLAS_COMMAND_ID_BASE + 4)
#define IDC_ATLAS_AGENT_RESUME        (ATLAS_COMMAND_ID_BASE + 5)
#define IDC_ATLAS_AGENT_SETTINGS      (ATLAS_COMMAND_ID_BASE + 6)

// Privacy commands
#define IDC_ATLAS_PRIVACY_TOGGLE      (ATLAS_COMMAND_ID_BASE + 10)
#define IDC_ATLAS_PRIVACY_STATS       (ATLAS_COMMAND_ID_BASE + 11)
#define IDC_ATLAS_PRIVACY_SETTINGS    (ATLAS_COMMAND_ID_BASE + 12)

// Settings commands
#define IDC_ATLAS_SETTINGS            (ATLAS_COMMAND_ID_BASE + 20)
#define IDC_ATLAS_ABOUT               (ATLAS_COMMAND_ID_BASE + 21)
#define IDC_ATLAS_CHECK_UPDATE        (ATLAS_COMMAND_ID_BASE + 22)

// Debug commands (only in debug builds)
#define IDC_ATLAS_DEBUG_AGENT         (ATLAS_COMMAND_ID_BASE + 100)
#define IDC_ATLAS_DEBUG_DOM           (ATLAS_COMMAND_ID_BASE + 101)
#define IDC_ATLAS_DEBUG_SCREENSHOT    (ATLAS_COMMAND_ID_BASE + 102)

// View IDs for testing
#define VIEW_ID_ATLAS_AGENT_BUTTON    1000
#define VIEW_ID_ATLAS_PRIVACY_BUTTON  1001
#define VIEW_ID_ATLAS_AGENT_PANEL     1002

#endif  // ATLAS_BROWSER_BROWSER_ATLAS_COMMAND_IDS_H_
