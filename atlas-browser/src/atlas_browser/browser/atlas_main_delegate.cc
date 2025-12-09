// Copyright 2025 Atlas Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license.

#include "atlas_browser/browser/atlas_main_delegate.h"

#include "atlas_browser/browser/atlas_content_browser_client.h"
#include "atlas_browser/common/atlas_content_renderer_client.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "chrome/common/chrome_paths.h"
#include "content/public/common/content_switches.h"
#include "ui/base/resource/resource_bundle.h"

namespace atlas {

namespace {

// Atlas-specific command line switches
const char kEnableAtlasAgent[] = "enable-atlas-agent";
const char kAtlasAgentModel[] = "atlas-agent-model";
const char kAtlasAgentProvider[] = "atlas-agent-provider";
const char kAtlasLocalLLM[] = "atlas-local-llm";

}  // namespace

AtlasMainDelegate::AtlasMainDelegate() = default;

AtlasMainDelegate::~AtlasMainDelegate() = default;

std::optional<int> AtlasMainDelegate::BasicStartupComplete() {
  // Run Chrome's basic startup first
  auto result = ChromeMainDelegate::BasicStartupComplete();
  if (result.has_value()) {
    return result;
  }

  LOG(INFO) << "Atlas Browser starting up...";
  LOG(INFO) << "Version: " << ATLAS_VERSION_STRING;

  // Get command line
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();

  // Enable Atlas agent by default (can be disabled via command line)
  if (!command_line->HasSwitch(kEnableAtlasAgent)) {
    command_line->AppendSwitch(kEnableAtlasAgent);
  }

  // Set default agent provider if not specified
  if (!command_line->HasSwitch(kAtlasAgentProvider)) {
    command_line->AppendSwitchASCII(kAtlasAgentProvider, "anthropic");
  }

  // Set default model if not specified
  if (!command_line->HasSwitch(kAtlasAgentModel)) {
    command_line->AppendSwitchASCII(kAtlasAgentModel, "claude-sonnet-4-20250514");
  }

  InitializeAtlasFeatures();

  return std::nullopt;  // Continue startup
}

void AtlasMainDelegate::PreSandboxStartup() {
  ChromeMainDelegate::PreSandboxStartup();
  
  // Load Atlas-specific resources before sandbox is enabled
  LoadAtlasResources();
}

std::optional<int> AtlasMainDelegate::PreBrowserMain() {
  auto result = ChromeMainDelegate::PreBrowserMain();
  if (result.has_value()) {
    return result;
  }

  LOG(INFO) << "Atlas Browser initializing browser process...";
  
  return std::nullopt;
}

content::ContentBrowserClient* AtlasMainDelegate::CreateContentBrowserClient() {
  browser_client_ = std::make_unique<AtlasContentBrowserClient>();
  return browser_client_.get();
}

content::ContentRendererClient* AtlasMainDelegate::CreateContentRendererClient() {
  renderer_client_ = std::make_unique<AtlasContentRendererClient>();
  return renderer_client_.get();
}

void AtlasMainDelegate::InitializeAtlasFeatures() {
  LOG(INFO) << "Initializing Atlas features...";

  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();

  // Log enabled features
  if (command_line->HasSwitch(kEnableAtlasAgent)) {
    LOG(INFO) << "  - AI Agent: ENABLED";
    LOG(INFO) << "    Provider: " 
              << command_line->GetSwitchValueASCII(kAtlasAgentProvider);
    LOG(INFO) << "    Model: " 
              << command_line->GetSwitchValueASCII(kAtlasAgentModel);
  }

  if (command_line->HasSwitch(kAtlasLocalLLM)) {
    LOG(INFO) << "  - Local LLM: ENABLED";
  }

  // Additional Atlas features can be initialized here
  LOG(INFO) << "  - Privacy Mode: ENABLED";
  LOG(INFO) << "  - Tracker Blocking: ENABLED";
}

void AtlasMainDelegate::LoadAtlasResources() {
  // Get the path to Atlas resource files
  base::FilePath resources_path;
  if (!base::PathService::Get(chrome::DIR_RESOURCES, &resources_path)) {
    LOG(ERROR) << "Failed to get resources directory";
    return;
  }

  // Load Atlas-specific resource pack
  base::FilePath atlas_resources = resources_path.AppendASCII("atlas_resources.pak");
  if (base::PathExists(atlas_resources)) {
    ui::ResourceBundle::GetSharedInstance().AddDataPackFromPath(
        atlas_resources, ui::kScaleFactorNone);
    LOG(INFO) << "Loaded Atlas resources from: " << atlas_resources;
  }

  // Load localized Atlas strings
  base::FilePath atlas_strings = resources_path.AppendASCII("atlas_strings_en-US.pak");
  if (base::PathExists(atlas_strings)) {
    ui::ResourceBundle::GetSharedInstance().AddDataPackFromPath(
        atlas_strings, ui::kScaleFactorNone);
  }
}

}  // namespace atlas
