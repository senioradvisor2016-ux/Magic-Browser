// Copyright 2025 Atlas Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license.

#include "atlas_browser/browser/ui/webui/atlas_settings_ui.h"

#include "atlas_browser/browser/atlas_prefs.h"
#include "atlas_browser/browser/update/atlas_update_client.h"
#include "atlas_browser/features/llm_agent/browser/agent_service.h"
#include "atlas_browser/features/privacy/tracker_blocker.h"
#include "base/functional/bind.h"
#include "base/values.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"

namespace atlas {

namespace {

void CreateAndAddDataSource(content::BrowserContext* browser_context) {
  auto source = content::WebUIDataSource::CreateAndAdd(
      browser_context, "atlas-settings");

  // Add resources
  source->AddResourcePath("settings.html", IDR_ATLAS_SETTINGS_HTML);
  source->AddResourcePath("settings.js", IDR_ATLAS_SETTINGS_JS);
  source->AddResourcePath("settings.css", IDR_ATLAS_SETTINGS_CSS);
  source->SetDefaultResource(IDR_ATLAS_SETTINGS_HTML);

  // Allow inline scripts
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ScriptSrc,
      "script-src 'self' 'unsafe-inline';");
}

}  // namespace

// =============================================================================
// AtlasSettingsUI
// =============================================================================

AtlasSettingsUI::AtlasSettingsUI(content::WebUI* web_ui)
    : content::WebUIController(web_ui) {
  Profile* profile = Profile::FromWebUI(web_ui);
  CreateAndAddDataSource(profile);
  
  web_ui->AddMessageHandler(std::make_unique<AtlasSettingsHandler>());
}

AtlasSettingsUI::~AtlasSettingsUI() = default;

// =============================================================================
// AtlasSettingsHandler
// =============================================================================

AtlasSettingsHandler::AtlasSettingsHandler() = default;
AtlasSettingsHandler::~AtlasSettingsHandler() = default;

void AtlasSettingsHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "loadSettings",
      base::BindRepeating(&AtlasSettingsHandler::HandleLoadSettings,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "saveSettings",
      base::BindRepeating(&AtlasSettingsHandler::HandleSaveSettings,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "getStatistics",
      base::BindRepeating(&AtlasSettingsHandler::HandleGetStatistics,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "checkForUpdates",
      base::BindRepeating(&AtlasSettingsHandler::HandleCheckForUpdates,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "testApiKey",
      base::BindRepeating(&AtlasSettingsHandler::HandleTestApiKey,
                          base::Unretained(this)));
}

void AtlasSettingsHandler::HandleLoadSettings(const base::Value::List& args) {
  AllowJavascript();

  base::Value::Dict settings;
  settings.Set("agent", GetAgentSettings());
  settings.Set("privacy", GetPrivacySettings());
  settings.Set("updates", GetUpdateSettings());

  // Send to JavaScript
  CallJavascriptFunction("settingsLoaded", settings);
}

void AtlasSettingsHandler::HandleSaveSettings(const base::Value::List& args) {
  if (args.empty() || !args[0].is_dict()) {
    return;
  }

  const auto& settings = args[0].GetDict();

  // Save agent settings
  if (auto* agent = settings.FindDict("agent")) {
    SetAgentSettings(*agent);
  }

  // Save privacy settings
  if (auto* privacy = settings.FindDict("privacy")) {
    SetPrivacySettings(*privacy);
  }

  // Save update settings
  if (auto* updates = settings.FindDict("updates")) {
    SetUpdateSettings(*updates);
  }
}

void AtlasSettingsHandler::HandleGetStatistics(const base::Value::List& args) {
  AllowJavascript();

  auto* tracker_blocker = TrackerBlocker::GetInstance();
  
  base::Value::Dict stats;
  stats.Set("trackersBlocked", tracker_blocker->GetBlockedCount());
  stats.Set("fingerprintsBlocked", 0);  // TODO: Implement

  CallJavascriptFunction("statisticsUpdated", stats);
}

void AtlasSettingsHandler::HandleCheckForUpdates(const base::Value::List& args) {
  AllowJavascript();

  auto* update_client = AtlasUpdateClient::GetInstance();
  update_client->CheckForUpdates();

  // Observer will handle callback
}

void AtlasSettingsHandler::HandleTestApiKey(const base::Value::List& args) {
  AllowJavascript();

  if (args.empty() || !args[0].is_string()) {
    CallJavascriptFunction("apiKeyTestResult", false, "Missing API key");
    return;
  }

  std::string api_key = args[0].GetString();

  // TODO: Actually test the API key with a simple request
  // For now, just check if it looks valid
  bool valid = !api_key.empty() && api_key.length() > 10;
  
  if (valid) {
    CallJavascriptFunction("apiKeyTestResult", true, "API key is valid");
  } else {
    CallJavascriptFunction("apiKeyTestResult", false, "Invalid API key format");
  }
}

// =============================================================================
// Settings Helpers
// =============================================================================

base::Value::Dict AtlasSettingsHandler::GetAgentSettings() {
  Profile* profile = Profile::FromWebUI(web_ui());
  PrefService* prefs = profile->GetPrefs();

  base::Value::Dict settings;
  settings.Set("enabled", prefs->GetBoolean(prefs::kAgentEnabled));
  settings.Set("provider", prefs->GetString(prefs::kAgentProvider));
  settings.Set("model", prefs->GetString(prefs::kAgentModel));
  settings.Set("apiKey", prefs->GetString(prefs::kAgentApiKey));
  settings.Set("maxSteps", prefs->GetInteger(prefs::kAgentMaxSteps));
  settings.Set("enableVision", prefs->GetBoolean(prefs::kAgentEnableVision));
  settings.Set("enableLabels", 
               prefs->GetBoolean(prefs::kAgentEnableElementLabels));

  return settings;
}

base::Value::Dict AtlasSettingsHandler::GetPrivacySettings() {
  Profile* profile = Profile::FromWebUI(web_ui());
  PrefService* prefs = profile->GetPrefs();

  base::Value::Dict settings;
  settings.Set("trackerBlocking", 
               prefs->GetBoolean(prefs::kPrivacyTrackerBlockingEnabled));
  settings.Set("fingerprintProtection", 
               prefs->GetBoolean(prefs::kPrivacyFingerprintProtectionEnabled));

  // Get blocked domains
  base::Value::List domains;
  const auto& blocked_list = prefs->GetList(prefs::kPrivacyBlockedDomains);
  for (const auto& domain : blocked_list) {
    if (domain.is_string()) {
      domains.Append(domain.GetString());
    }
  }
  settings.Set("blockedDomains", std::move(domains));

  return settings;
}

base::Value::Dict AtlasSettingsHandler::GetUpdateSettings() {
  base::Value::Dict settings;
  
  auto* update_client = AtlasUpdateClient::GetInstance();
  settings.Set("autoUpdate", update_client->IsAutoCheckEnabled());
  settings.Set("channel", update_client->GetConfig().channel);
  settings.Set("currentVersion", 
               update_client->GetCurrentVersion().GetString());

  return settings;
}

void AtlasSettingsHandler::SetAgentSettings(const base::Value::Dict& settings) {
  Profile* profile = Profile::FromWebUI(web_ui());
  PrefService* prefs = profile->GetPrefs();

  if (auto* val = settings.FindBool("enabled")) {
    prefs->SetBoolean(prefs::kAgentEnabled, *val);
  }
  if (auto* val = settings.FindString("provider")) {
    prefs->SetString(prefs::kAgentProvider, *val);
  }
  if (auto* val = settings.FindString("model")) {
    prefs->SetString(prefs::kAgentModel, *val);
  }
  if (auto* val = settings.FindString("apiKey")) {
    prefs->SetString(prefs::kAgentApiKey, *val);
  }
  if (auto val = settings.FindInt("maxSteps")) {
    prefs->SetInteger(prefs::kAgentMaxSteps, *val);
  }
  if (auto* val = settings.FindBool("enableVision")) {
    prefs->SetBoolean(prefs::kAgentEnableVision, *val);
  }
  if (auto* val = settings.FindBool("enableLabels")) {
    prefs->SetBoolean(prefs::kAgentEnableElementLabels, *val);
  }

  // Apply to agent service
  AgentService* agent_service = 
      AgentService::GetForBrowserContext(profile);
  if (agent_service) {
    auto config = prefs::GetAgentConfigFromPrefs(prefs);
    AgentService::Config service_config;
    service_config.api_key = config.api_key;
    service_config.model = config.model;
    service_config.max_steps = config.max_steps;
    service_config.enable_vision = config.enable_vision;
    agent_service->Configure(service_config);
  }
}

void AtlasSettingsHandler::SetPrivacySettings(const base::Value::Dict& settings) {
  Profile* profile = Profile::FromWebUI(web_ui());
  PrefService* prefs = profile->GetPrefs();

  if (auto* val = settings.FindBool("trackerBlocking")) {
    prefs->SetBoolean(prefs::kPrivacyTrackerBlockingEnabled, *val);
  }
  if (auto* val = settings.FindBool("fingerprintProtection")) {
    prefs->SetBoolean(prefs::kPrivacyFingerprintProtectionEnabled, *val);
  }

  // Update blocked domains
  if (auto* domains = settings.FindList("blockedDomains")) {
    base::Value::List domain_list;
    for (const auto& domain : *domains) {
      if (domain.is_string()) {
        domain_list.Append(domain.GetString());
      }
    }
    prefs->SetList(prefs::kPrivacyBlockedDomains, std::move(domain_list));
  }

  // Apply to tracker blocker
  auto* tracker_blocker = TrackerBlocker::GetInstance();
  tracker_blocker->SetEnabled(
      prefs->GetBoolean(prefs::kPrivacyTrackerBlockingEnabled));
}

void AtlasSettingsHandler::SetUpdateSettings(const base::Value::Dict& settings) {
  auto* update_client = AtlasUpdateClient::GetInstance();

  if (auto* val = settings.FindBool("autoUpdate")) {
    update_client->SetAutoCheckEnabled(*val);
  }

  if (auto* val = settings.FindString("channel")) {
    AtlasUpdateClient::Config config = update_client->GetConfig();
    config.channel = *val;
    update_client->Configure(config);
  }
}

}  // namespace atlas
