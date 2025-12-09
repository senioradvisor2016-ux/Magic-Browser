// Copyright 2025 Atlas Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license.

#ifndef ATLAS_BROWSER_BROWSER_DEVTOOLS_ATLAS_DEVTOOLS_PANEL_H_
#define ATLAS_BROWSER_BROWSER_DEVTOOLS_ATLAS_DEVTOOLS_PANEL_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/values.h"

namespace content {
class DevToolsAgentHost;
class WebContents;
}

namespace atlas {

class AgentService;

// AtlasDevToolsPanel provides a DevTools panel for debugging and
// interacting with the AI agent.
//
// Features:
// - View agent logs and reasoning
// - Inspect DOM extraction results
// - Test individual actions
// - View/modify agent prompts
// - Performance metrics
class AtlasDevToolsPanel {
 public:
  explicit AtlasDevToolsPanel(content::WebContents* inspected_contents);
  ~AtlasDevToolsPanel();

  AtlasDevToolsPanel(const AtlasDevToolsPanel&) = delete;
  AtlasDevToolsPanel& operator=(const AtlasDevToolsPanel&) = delete;

  // Panel actions
  void Refresh();
  void Clear();

  // Agent inspection
  base::Value::Dict GetAgentState() const;
  base::Value::Dict GetLastTaskResult() const;
  base::Value::List GetActionHistory() const;

  // DOM inspection
  base::Value::Dict GetExtractedDOM() const;
  base::Value::List GetInteractiveElements() const;

  // Testing
  void TestAction(const std::string& action_json);
  void TestSelector(const std::string& selector);

  // Prompts
  std::string GetCurrentSystemPrompt() const;
  void SetCustomSystemPrompt(const std::string& prompt);
  void ResetSystemPrompt();

  // Metrics
  struct Metrics {
    int total_tasks = 0;
    int successful_tasks = 0;
    int failed_tasks = 0;
    int total_steps = 0;
    double avg_steps_per_task = 0.0;
    double avg_task_duration_ms = 0.0;
    int dom_extractions = 0;
    int screenshots_captured = 0;
    int llm_requests = 0;
    double avg_llm_latency_ms = 0.0;
  };

  Metrics GetMetrics() const;
  void ResetMetrics();

  // Export/Import
  std::string ExportLogs() const;
  std::string ExportConfig() const;
  void ImportConfig(const std::string& config_json);

 private:
  raw_ptr<content::WebContents> inspected_contents_;
  raw_ptr<AgentService> agent_service_;

  // Panel state
  std::vector<std::string> action_history_;
  base::Value::Dict last_dom_;
  std::string custom_system_prompt_;

  // Metrics tracking
  Metrics metrics_;
};

// =============================================================================
// DevTools Protocol Handler
// =============================================================================

// Handles DevTools protocol messages for the Atlas panel
class AtlasDevToolsProtocolHandler {
 public:
  AtlasDevToolsProtocolHandler();
  ~AtlasDevToolsProtocolHandler();

  // Handle incoming DevTools protocol message
  void HandleMessage(const std::string& method,
                     const base::Value::Dict& params,
                     base::OnceCallback<void(base::Value::Dict)> callback);

  // Available methods:
  // Atlas.getAgentState
  // Atlas.getDOM
  // Atlas.getElements
  // Atlas.testAction
  // Atlas.testSelector
  // Atlas.getMetrics
  // Atlas.setPrompt
  // Atlas.startTask
  // Atlas.cancelTask

 private:
  void HandleGetAgentState(base::OnceCallback<void(base::Value::Dict)> callback);
  void HandleGetDOM(base::OnceCallback<void(base::Value::Dict)> callback);
  void HandleGetElements(base::OnceCallback<void(base::Value::Dict)> callback);
  void HandleTestAction(const base::Value::Dict& params,
                        base::OnceCallback<void(base::Value::Dict)> callback);
  void HandleTestSelector(const base::Value::Dict& params,
                          base::OnceCallback<void(base::Value::Dict)> callback);
  void HandleGetMetrics(base::OnceCallback<void(base::Value::Dict)> callback);
  void HandleSetPrompt(const base::Value::Dict& params,
                       base::OnceCallback<void(base::Value::Dict)> callback);
  void HandleStartTask(const base::Value::Dict& params,
                       base::OnceCallback<void(base::Value::Dict)> callback);
  void HandleCancelTask(const base::Value::Dict& params,
                        base::OnceCallback<void(base::Value::Dict)> callback);

  std::unique_ptr<AtlasDevToolsPanel> panel_;
};

}  // namespace atlas

#endif  // ATLAS_BROWSER_BROWSER_DEVTOOLS_ATLAS_DEVTOOLS_PANEL_H_
