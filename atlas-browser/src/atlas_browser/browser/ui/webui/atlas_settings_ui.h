// Copyright 2025 Atlas Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license.

#ifndef ATLAS_BROWSER_BROWSER_UI_WEBUI_ATLAS_SETTINGS_UI_H_
#define ATLAS_BROWSER_BROWSER_UI_WEBUI_ATLAS_SETTINGS_UI_H_

#include "content/public/browser/web_ui_controller.h"

namespace atlas {

// WebUI controller for atlas://settings
class AtlasSettingsUI : public content::WebUIController {
 public:
  explicit AtlasSettingsUI(content::WebUI* web_ui);
  ~AtlasSettingsUI() override;

  AtlasSettingsUI(const AtlasSettingsUI&) = delete;
  AtlasSettingsUI& operator=(const AtlasSettingsUI&) = delete;
};

// Message handler for atlas://settings
class AtlasSettingsHandler : public content::WebUIMessageHandler {
 public:
  AtlasSettingsHandler();
  ~AtlasSettingsHandler() override;

  AtlasSettingsHandler(const AtlasSettingsHandler&) = delete;
  AtlasSettingsHandler& operator=(const AtlasSettingsHandler&) = delete;

  // WebUIMessageHandler implementation
  void RegisterMessages() override;

 private:
  // Message handlers
  void HandleLoadSettings(const base::Value::List& args);
  void HandleSaveSettings(const base::Value::List& args);
  void HandleGetStatistics(const base::Value::List& args);
  void HandleCheckForUpdates(const base::Value::List& args);
  void HandleTestApiKey(const base::Value::List& args);

  // Settings helpers
  base::Value::Dict GetAgentSettings();
  base::Value::Dict GetPrivacySettings();
  base::Value::Dict GetUpdateSettings();
  
  void SetAgentSettings(const base::Value::Dict& settings);
  void SetPrivacySettings(const base::Value::Dict& settings);
  void SetUpdateSettings(const base::Value::Dict& settings);
};

}  // namespace atlas

#endif  // ATLAS_BROWSER_BROWSER_UI_WEBUI_ATLAS_SETTINGS_UI_H_
