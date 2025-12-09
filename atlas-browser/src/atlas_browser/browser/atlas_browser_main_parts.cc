// Copyright 2025 Atlas Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license.

#include "atlas_browser/browser/atlas_browser_main_parts.h"

#include "atlas_browser/features/llm_agent/browser/agent_service.h"
#include "atlas_browser/features/privacy/tracker_blocker.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "chrome/browser/profiles/profile_manager.h"

namespace atlas {

namespace {

const char kEnableAtlasAgent[] = "enable-atlas-agent";
const char kDisableTrackerBlocking[] = "disable-tracker-blocking";

}  // namespace

AtlasBrowserMainParts::AtlasBrowserMainParts(bool is_integration_test)
    : ChromeBrowserMainParts(is_integration_test),
      is_integration_test_(is_integration_test) {}

AtlasBrowserMainParts::~AtlasBrowserMainParts() = default;

int AtlasBrowserMainParts::PreEarlyInitialization() {
  int result = ChromeBrowserMainParts::PreEarlyInitialization();
  if (result != content::RESULT_CODE_NORMAL_EXIT) {
    return result;
  }

  LOG(INFO) << "Atlas Browser: PreEarlyInitialization";
  return content::RESULT_CODE_NORMAL_EXIT;
}

void AtlasBrowserMainParts::PreCreateMainMessageLoop() {
  ChromeBrowserMainParts::PreCreateMainMessageLoop();
  LOG(INFO) << "Atlas Browser: PreCreateMainMessageLoop";
}

void AtlasBrowserMainParts::PostCreateMainMessageLoop() {
  ChromeBrowserMainParts::PostCreateMainMessageLoop();
  LOG(INFO) << "Atlas Browser: PostCreateMainMessageLoop";

  // Initialize Atlas systems that need to be ready early
  InitializePrivacyFeatures();
}

int AtlasBrowserMainParts::PreMainMessageLoopRun() {
  int result = ChromeBrowserMainParts::PreMainMessageLoopRun();
  if (result != content::RESULT_CODE_NORMAL_EXIT) {
    return result;
  }

  LOG(INFO) << "Atlas Browser: PreMainMessageLoopRun";

  // Initialize Atlas UI
  InitializeAtlasUI();

  // Initialize agent system
  InitializeAgentSystem();

  LOG(INFO) << "Atlas Browser ready!";
  return content::RESULT_CODE_NORMAL_EXIT;
}

void AtlasBrowserMainParts::PostMainMessageLoopRun() {
  LOG(INFO) << "Atlas Browser: Shutting down...";

  // Cleanup Atlas systems
  // Agent service cleanup is handled by KeyedService destruction

  ChromeBrowserMainParts::PostMainMessageLoopRun();
}

void AtlasBrowserMainParts::InitializeAtlasUI() {
  LOG(INFO) << "Initializing Atlas UI components...";

  // Register Atlas-specific Views
  // This includes the Agent Panel, custom toolbar buttons, etc.
  
  // Note: Actual View registration happens through the ViewsDelegate
  // and BrowserView customization
}

void AtlasBrowserMainParts::InitializeAgentSystem() {
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();

  if (!command_line->HasSwitch(kEnableAtlasAgent)) {
    LOG(INFO) << "Atlas Agent is disabled";
    return;
  }

  LOG(INFO) << "Initializing Atlas Agent system...";

  // The AgentService is a KeyedService, so it will be created on-demand
  // when a profile is loaded. Here we just log that the system is ready.

  // Pre-warm the agent system for the default profile if available
  if (ProfileManager* profile_manager = g_browser_process->profile_manager()) {
    Profile* profile = profile_manager->GetLastUsedProfile();
    if (profile) {
      AgentService::GetOrCreateForBrowserContext(profile);
      LOG(INFO) << "Atlas Agent initialized for default profile";
    }
  }
}

void AtlasBrowserMainParts::InitializePrivacyFeatures() {
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();

  if (command_line->HasSwitch(kDisableTrackerBlocking)) {
    LOG(INFO) << "Tracker blocking is disabled";
    return;
  }

  LOG(INFO) << "Initializing privacy features...";

  // Initialize tracker blocker
  TrackerBlocker::GetInstance()->Initialize();

  LOG(INFO) << "  - Tracker blocking: ENABLED";
  LOG(INFO) << "  - Fingerprint protection: ENABLED";
}

}  // namespace atlas
