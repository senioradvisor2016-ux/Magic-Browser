// Copyright 2025 Atlas Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license.

#include "atlas_browser/browser/atlas_prefs.h"

#include "base/json/values_util.h"
#include "base/values.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/sync_preferences/pref_service_syncable.h"

namespace atlas {
namespace prefs {

// =============================================================================
// Preference Keys
// =============================================================================

// Agent preferences
const char kAgentEnabled[] = "atlas.agent.enabled";
const char kAgentProvider[] = "atlas.agent.provider";
const char kAgentModel[] = "atlas.agent.model";
const char kAgentApiKey[] = "atlas.agent.api_key";
const char kAgentApiEndpoint[] = "atlas.agent.api_endpoint";
const char kAgentMaxSteps[] = "atlas.agent.max_steps";
const char kAgentStepTimeout[] = "atlas.agent.step_timeout_ms";
const char kAgentEnableVision[] = "atlas.agent.enable_vision";
const char kAgentEnableElementLabels[] = "atlas.agent.enable_element_labels";
const char kAgentTemperature[] = "atlas.agent.temperature";
const char kAgentVerboseLogging[] = "atlas.agent.verbose_logging";

// Privacy preferences
const char kPrivacyTrackerBlockingEnabled[] = "atlas.privacy.tracker_blocking";
const char kPrivacyFingerprintProtectionEnabled[] = 
    "atlas.privacy.fingerprint_protection";
const char kPrivacyBlockedDomains[] = "atlas.privacy.blocked_domains";
const char kPrivacyAllowedDomains[] = "atlas.privacy.allowed_domains";

// UI preferences
const char kUIAgentPanelPosition[] = "atlas.ui.agent_panel_position";
const char kUIAgentPanelWidth[] = "atlas.ui.agent_panel_width";
const char kUIShowAgentInToolbar[] = "atlas.ui.show_agent_in_toolbar";

// Statistics
const char kStatsTrackersBlocked[] = "atlas.stats.trackers_blocked";
const char kStatsAgentTasksCompleted[] = "atlas.stats.agent_tasks_completed";
const char kStatsAgentStepsExecuted[] = "atlas.stats.agent_steps_executed";

// =============================================================================
// Registration Functions
// =============================================================================

void RegisterLocalStatePrefs(PrefRegistrySimple* registry) {
  // Statistics are stored locally, not synced
  registry->RegisterIntegerPref(kStatsTrackersBlocked, 0);
  registry->RegisterIntegerPref(kStatsAgentTasksCompleted, 0);
  registry->RegisterIntegerPref(kStatsAgentStepsExecuted, 0);
}

void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry) {
  // Agent preferences
  registry->RegisterBooleanPref(kAgentEnabled, true);
  registry->RegisterStringPref(kAgentProvider, "anthropic");
  registry->RegisterStringPref(kAgentModel, "claude-sonnet-4-20250514");
  registry->RegisterStringPref(kAgentApiKey, "");
  registry->RegisterStringPref(kAgentApiEndpoint, "");
  registry->RegisterIntegerPref(kAgentMaxSteps, 50);
  registry->RegisterIntegerPref(kAgentStepTimeout, 30000);
  registry->RegisterBooleanPref(kAgentEnableVision, true);
  registry->RegisterBooleanPref(kAgentEnableElementLabels, true);
  registry->RegisterDoublePref(kAgentTemperature, 0.0);
  registry->RegisterBooleanPref(kAgentVerboseLogging, false);

  // Privacy preferences
  registry->RegisterBooleanPref(kPrivacyTrackerBlockingEnabled, true);
  registry->RegisterBooleanPref(kPrivacyFingerprintProtectionEnabled, true);
  registry->RegisterListPref(kPrivacyBlockedDomains);
  registry->RegisterListPref(kPrivacyAllowedDomains);

  // UI preferences
  registry->RegisterStringPref(kUIAgentPanelPosition, "right");
  registry->RegisterIntegerPref(kUIAgentPanelWidth, 380);
  registry->RegisterBooleanPref(kUIShowAgentInToolbar, true);
}

// =============================================================================
// Helper Functions
// =============================================================================

AgentConfig GetAgentConfigFromPrefs(PrefService* prefs) {
  AgentConfig config;
  
  config.enabled = prefs->GetBoolean(kAgentEnabled);
  config.provider = prefs->GetString(kAgentProvider);
  config.model = prefs->GetString(kAgentModel);
  config.api_key = prefs->GetString(kAgentApiKey);
  config.api_endpoint = prefs->GetString(kAgentApiEndpoint);
  config.max_steps = prefs->GetInteger(kAgentMaxSteps);
  config.step_timeout_ms = prefs->GetInteger(kAgentStepTimeout);
  config.enable_vision = prefs->GetBoolean(kAgentEnableVision);
  config.enable_element_labels = prefs->GetBoolean(kAgentEnableElementLabels);
  config.temperature = static_cast<float>(prefs->GetDouble(kAgentTemperature));
  config.verbose_logging = prefs->GetBoolean(kAgentVerboseLogging);

  return config;
}

void SetAgentConfigToPrefs(PrefService* prefs, const AgentConfig& config) {
  prefs->SetBoolean(kAgentEnabled, config.enabled);
  prefs->SetString(kAgentProvider, config.provider);
  prefs->SetString(kAgentModel, config.model);
  prefs->SetString(kAgentApiKey, config.api_key);
  prefs->SetString(kAgentApiEndpoint, config.api_endpoint);
  prefs->SetInteger(kAgentMaxSteps, config.max_steps);
  prefs->SetInteger(kAgentStepTimeout, config.step_timeout_ms);
  prefs->SetBoolean(kAgentEnableVision, config.enable_vision);
  prefs->SetBoolean(kAgentEnableElementLabels, config.enable_element_labels);
  prefs->SetDouble(kAgentTemperature, static_cast<double>(config.temperature));
  prefs->SetBoolean(kAgentVerboseLogging, config.verbose_logging);
}

PrivacyConfig GetPrivacyConfigFromPrefs(PrefService* prefs) {
  PrivacyConfig config;
  
  config.tracker_blocking_enabled = 
      prefs->GetBoolean(kPrivacyTrackerBlockingEnabled);
  config.fingerprint_protection_enabled = 
      prefs->GetBoolean(kPrivacyFingerprintProtectionEnabled);

  // Get blocked domains list
  const base::Value::List& blocked_list = 
      prefs->GetList(kPrivacyBlockedDomains);
  for (const auto& value : blocked_list) {
    if (value.is_string()) {
      config.blocked_domains.push_back(value.GetString());
    }
  }

  // Get allowed domains list
  const base::Value::List& allowed_list = 
      prefs->GetList(kPrivacyAllowedDomains);
  for (const auto& value : allowed_list) {
    if (value.is_string()) {
      config.allowed_domains.push_back(value.GetString());
    }
  }

  return config;
}

void SetPrivacyConfigToPrefs(PrefService* prefs, const PrivacyConfig& config) {
  prefs->SetBoolean(kPrivacyTrackerBlockingEnabled, 
                    config.tracker_blocking_enabled);
  prefs->SetBoolean(kPrivacyFingerprintProtectionEnabled, 
                    config.fingerprint_protection_enabled);

  // Set blocked domains list
  base::Value::List blocked_list;
  for (const auto& domain : config.blocked_domains) {
    blocked_list.Append(domain);
  }
  prefs->SetList(kPrivacyBlockedDomains, std::move(blocked_list));

  // Set allowed domains list
  base::Value::List allowed_list;
  for (const auto& domain : config.allowed_domains) {
    allowed_list.Append(domain);
  }
  prefs->SetList(kPrivacyAllowedDomains, std::move(allowed_list));
}

}  // namespace prefs
}  // namespace atlas
