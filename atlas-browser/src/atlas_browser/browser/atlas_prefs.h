// Copyright 2025 Atlas Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license.

#ifndef ATLAS_BROWSER_BROWSER_ATLAS_PREFS_H_
#define ATLAS_BROWSER_BROWSER_ATLAS_PREFS_H_

class PrefRegistrySimple;
class PrefService;

namespace user_prefs {
class PrefRegistrySyncable;
}

namespace atlas {
namespace prefs {

// =============================================================================
// Preference Keys
// =============================================================================

// Agent preferences
extern const char kAgentEnabled[];
extern const char kAgentProvider[];
extern const char kAgentModel[];
extern const char kAgentApiKey[];
extern const char kAgentApiEndpoint[];
extern const char kAgentMaxSteps[];
extern const char kAgentStepTimeout[];
extern const char kAgentEnableVision[];
extern const char kAgentEnableElementLabels[];
extern const char kAgentTemperature[];
extern const char kAgentVerboseLogging[];

// Privacy preferences
extern const char kPrivacyTrackerBlockingEnabled[];
extern const char kPrivacyFingerprintProtectionEnabled[];
extern const char kPrivacyBlockedDomains[];
extern const char kPrivacyAllowedDomains[];

// UI preferences
extern const char kUIAgentPanelPosition[];
extern const char kUIAgentPanelWidth[];
extern const char kUIShowAgentInToolbar[];

// Statistics
extern const char kStatsTrackersBlocked[];
extern const char kStatsAgentTasksCompleted[];
extern const char kStatsAgentStepsExecuted[];

// =============================================================================
// Registration Functions
// =============================================================================

// Register preferences that are stored locally (not synced)
void RegisterLocalStatePrefs(PrefRegistrySimple* registry);

// Register preferences that are per-profile and can be synced
void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

// =============================================================================
// Helper Functions
// =============================================================================

// Get agent configuration from preferences
struct AgentConfig {
  bool enabled;
  std::string provider;
  std::string model;
  std::string api_key;
  std::string api_endpoint;
  int max_steps;
  int step_timeout_ms;
  bool enable_vision;
  bool enable_element_labels;
  float temperature;
  bool verbose_logging;
};

AgentConfig GetAgentConfigFromPrefs(PrefService* prefs);
void SetAgentConfigToPrefs(PrefService* prefs, const AgentConfig& config);

// Get privacy configuration from preferences
struct PrivacyConfig {
  bool tracker_blocking_enabled;
  bool fingerprint_protection_enabled;
  std::vector<std::string> blocked_domains;
  std::vector<std::string> allowed_domains;
};

PrivacyConfig GetPrivacyConfigFromPrefs(PrefService* prefs);
void SetPrivacyConfigToPrefs(PrefService* prefs, const PrivacyConfig& config);

}  // namespace prefs
}  // namespace atlas

#endif  // ATLAS_BROWSER_BROWSER_ATLAS_PREFS_H_
