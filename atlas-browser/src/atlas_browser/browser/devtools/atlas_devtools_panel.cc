// Copyright 2025 Atlas Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license.

#include "atlas_browser/browser/devtools/atlas_devtools_panel.h"

#include "atlas_browser/features/llm_agent/browser/agent_service.h"
#include "atlas_browser/features/llm_agent/browser/prompts.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/time/time.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/web_contents.h"

namespace atlas {

// =============================================================================
// AtlasDevToolsPanel
// =============================================================================

AtlasDevToolsPanel::AtlasDevToolsPanel(content::WebContents* inspected_contents)
    : inspected_contents_(inspected_contents) {
  if (inspected_contents_) {
    Profile* profile = Profile::FromBrowserContext(
        inspected_contents_->GetBrowserContext());
    agent_service_ = AgentService::GetForBrowserContext(profile);
  }
}

AtlasDevToolsPanel::~AtlasDevToolsPanel() = default;

void AtlasDevToolsPanel::Refresh() {
  // Refresh cached data
  if (agent_service_ && inspected_contents_) {
    // Get fresh DOM extraction
    agent_service_->GetDOM(
        inspected_contents_,
        base::BindOnce(
            [](AtlasDevToolsPanel* self,
               const std::string& dom_json,
               std::vector<agent::mojom::ElementInfoPtr> elements) {
              auto dom = base::JSONReader::Read(dom_json);
              if (dom && dom->is_dict()) {
                self->last_dom_ = std::move(dom->GetDict());
              }
            },
            base::Unretained(this)));
  }
}

void AtlasDevToolsPanel::Clear() {
  action_history_.clear();
  last_dom_.clear();
  ResetMetrics();
}

base::Value::Dict AtlasDevToolsPanel::GetAgentState() const {
  base::Value::Dict state;

  if (agent_service_) {
    const auto& config = agent_service_->GetConfig();
    
    state.Set("enabled", true);
    state.Set("provider", 
              config.provider == AgentService::Config::Provider::kAnthropic 
                  ? "anthropic" 
                  : config.provider == AgentService::Config::Provider::kOpenAI 
                      ? "openai" 
                      : "local");
    state.Set("model", config.model);
    state.Set("maxSteps", config.max_steps);
    state.Set("enableVision", config.enable_vision);
    state.Set("hasApiKey", !config.api_key.empty());
    state.Set("hasActiveTasks", agent_service_->HasActiveTasks());
  } else {
    state.Set("enabled", false);
    state.Set("error", "Agent service not available");
  }

  return state;
}

base::Value::Dict AtlasDevToolsPanel::GetLastTaskResult() const {
  base::Value::Dict result;
  // Would return the last task result from agent service
  result.Set("available", false);
  return result;
}

base::Value::List AtlasDevToolsPanel::GetActionHistory() const {
  base::Value::List history;
  for (const auto& action : action_history_) {
    history.Append(action);
  }
  return history;
}

base::Value::Dict AtlasDevToolsPanel::GetExtractedDOM() const {
  return last_dom_.Clone();
}

base::Value::List AtlasDevToolsPanel::GetInteractiveElements() const {
  base::Value::List elements;
  // Would extract from last_dom_
  return elements;
}

void AtlasDevToolsPanel::TestAction(const std::string& action_json) {
  if (!agent_service_ || !inspected_contents_) {
    return;
  }

  // Parse action
  auto parsed = base::JSONReader::Read(action_json);
  if (!parsed || !parsed->is_dict()) {
    LOG(ERROR) << "Invalid action JSON";
    return;
  }

  // Execute action
  auto action = agent::mojom::AgentAction::New();
  const auto& dict = parsed->GetDict();

  const std::string* type = dict.FindString("type");
  if (!type) {
    return;
  }

  // Map type string to enum
  if (*type == "click") {
    action->type = agent::mojom::ActionType::kClick;
  } else if (*type == "type") {
    action->type = agent::mojom::ActionType::kType;
  } else if (*type == "scroll") {
    action->type = agent::mojom::ActionType::kScroll;
  } else {
    return;
  }

  if (auto* selector = dict.FindString("selector")) {
    action->selector = *selector;
  }
  if (auto* value = dict.FindString("value")) {
    action->value = *value;
  }

  // Record in history
  action_history_.push_back(action_json);
  metrics_.total_steps++;

  // Execute
  agent_service_->PerformAction(
      inspected_contents_,
      std::move(action),
      base::BindOnce([](agent::mojom::ActionResultPtr result) {
        LOG(INFO) << "Test action result: " 
                  << (result->success ? "success" : "failed");
      }));
}

void AtlasDevToolsPanel::TestSelector(const std::string& selector) {
  if (!agent_service_ || !inspected_contents_) {
    return;
  }

  // Would highlight the element matching the selector
  LOG(INFO) << "Testing selector: " << selector;
}

std::string AtlasDevToolsPanel::GetCurrentSystemPrompt() const {
  if (!custom_system_prompt_.empty()) {
    return custom_system_prompt_;
  }
  return prompts::kAgentSystemPrompt;
}

void AtlasDevToolsPanel::SetCustomSystemPrompt(const std::string& prompt) {
  custom_system_prompt_ = prompt;
  // Would apply to agent service
}

void AtlasDevToolsPanel::ResetSystemPrompt() {
  custom_system_prompt_.clear();
}

AtlasDevToolsPanel::Metrics AtlasDevToolsPanel::GetMetrics() const {
  return metrics_;
}

void AtlasDevToolsPanel::ResetMetrics() {
  metrics_ = Metrics();
}

std::string AtlasDevToolsPanel::ExportLogs() const {
  base::Value::Dict export_data;
  export_data.Set("timestamp", base::Time::Now().ToJsTime());
  export_data.Set("actionHistory", GetActionHistory());
  export_data.Set("dom", GetExtractedDOM());
  
  base::Value::Dict metrics_dict;
  metrics_dict.Set("totalTasks", metrics_.total_tasks);
  metrics_dict.Set("successfulTasks", metrics_.successful_tasks);
  metrics_dict.Set("failedTasks", metrics_.failed_tasks);
  metrics_dict.Set("totalSteps", metrics_.total_steps);
  export_data.Set("metrics", std::move(metrics_dict));

  std::string json;
  base::JSONWriter::WriteWithOptions(
      export_data,
      base::JSONWriter::OPTIONS_PRETTY_PRINT,
      &json);
  return json;
}

std::string AtlasDevToolsPanel::ExportConfig() const {
  base::Value::Dict config;
  config.Set("agentState", GetAgentState());
  config.Set("customPrompt", custom_system_prompt_);

  std::string json;
  base::JSONWriter::Write(config, &json);
  return json;
}

void AtlasDevToolsPanel::ImportConfig(const std::string& config_json) {
  auto parsed = base::JSONReader::Read(config_json);
  if (!parsed || !parsed->is_dict()) {
    return;
  }

  const auto& dict = parsed->GetDict();
  if (auto* prompt = dict.FindString("customPrompt")) {
    custom_system_prompt_ = *prompt;
  }
}

// =============================================================================
// AtlasDevToolsProtocolHandler
// =============================================================================

AtlasDevToolsProtocolHandler::AtlasDevToolsProtocolHandler() = default;
AtlasDevToolsProtocolHandler::~AtlasDevToolsProtocolHandler() = default;

void AtlasDevToolsProtocolHandler::HandleMessage(
    const std::string& method,
    const base::Value::Dict& params,
    base::OnceCallback<void(base::Value::Dict)> callback) {
  
  if (method == "Atlas.getAgentState") {
    HandleGetAgentState(std::move(callback));
  } else if (method == "Atlas.getDOM") {
    HandleGetDOM(std::move(callback));
  } else if (method == "Atlas.getElements") {
    HandleGetElements(std::move(callback));
  } else if (method == "Atlas.testAction") {
    HandleTestAction(params, std::move(callback));
  } else if (method == "Atlas.testSelector") {
    HandleTestSelector(params, std::move(callback));
  } else if (method == "Atlas.getMetrics") {
    HandleGetMetrics(std::move(callback));
  } else if (method == "Atlas.setPrompt") {
    HandleSetPrompt(params, std::move(callback));
  } else if (method == "Atlas.startTask") {
    HandleStartTask(params, std::move(callback));
  } else if (method == "Atlas.cancelTask") {
    HandleCancelTask(params, std::move(callback));
  } else {
    base::Value::Dict error;
    error.Set("error", "Unknown method: " + method);
    std::move(callback).Run(std::move(error));
  }
}

void AtlasDevToolsProtocolHandler::HandleGetAgentState(
    base::OnceCallback<void(base::Value::Dict)> callback) {
  base::Value::Dict result;
  if (panel_) {
    result = panel_->GetAgentState();
  }
  std::move(callback).Run(std::move(result));
}

void AtlasDevToolsProtocolHandler::HandleGetDOM(
    base::OnceCallback<void(base::Value::Dict)> callback) {
  base::Value::Dict result;
  if (panel_) {
    result = panel_->GetExtractedDOM();
  }
  std::move(callback).Run(std::move(result));
}

void AtlasDevToolsProtocolHandler::HandleGetElements(
    base::OnceCallback<void(base::Value::Dict)> callback) {
  base::Value::Dict result;
  if (panel_) {
    result.Set("elements", panel_->GetInteractiveElements());
  }
  std::move(callback).Run(std::move(result));
}

void AtlasDevToolsProtocolHandler::HandleTestAction(
    const base::Value::Dict& params,
    base::OnceCallback<void(base::Value::Dict)> callback) {
  base::Value::Dict result;
  
  if (panel_) {
    std::string action_json;
    base::JSONWriter::Write(params, &action_json);
    panel_->TestAction(action_json);
    result.Set("success", true);
  } else {
    result.Set("error", "Panel not initialized");
  }
  
  std::move(callback).Run(std::move(result));
}

void AtlasDevToolsProtocolHandler::HandleTestSelector(
    const base::Value::Dict& params,
    base::OnceCallback<void(base::Value::Dict)> callback) {
  base::Value::Dict result;
  
  if (panel_) {
    if (auto* selector = params.FindString("selector")) {
      panel_->TestSelector(*selector);
      result.Set("success", true);
    } else {
      result.Set("error", "Missing selector parameter");
    }
  }
  
  std::move(callback).Run(std::move(result));
}

void AtlasDevToolsProtocolHandler::HandleGetMetrics(
    base::OnceCallback<void(base::Value::Dict)> callback) {
  base::Value::Dict result;
  
  if (panel_) {
    auto metrics = panel_->GetMetrics();
    result.Set("totalTasks", metrics.total_tasks);
    result.Set("successfulTasks", metrics.successful_tasks);
    result.Set("failedTasks", metrics.failed_tasks);
    result.Set("totalSteps", metrics.total_steps);
    result.Set("avgStepsPerTask", metrics.avg_steps_per_task);
    result.Set("avgTaskDuration", metrics.avg_task_duration_ms);
    result.Set("llmRequests", metrics.llm_requests);
    result.Set("avgLlmLatency", metrics.avg_llm_latency_ms);
  }
  
  std::move(callback).Run(std::move(result));
}

void AtlasDevToolsProtocolHandler::HandleSetPrompt(
    const base::Value::Dict& params,
    base::OnceCallback<void(base::Value::Dict)> callback) {
  base::Value::Dict result;
  
  if (panel_) {
    if (auto* prompt = params.FindString("prompt")) {
      panel_->SetCustomSystemPrompt(*prompt);
      result.Set("success", true);
    } else {
      result.Set("error", "Missing prompt parameter");
    }
  }
  
  std::move(callback).Run(std::move(result));
}

void AtlasDevToolsProtocolHandler::HandleStartTask(
    const base::Value::Dict& params,
    base::OnceCallback<void(base::Value::Dict)> callback) {
  base::Value::Dict result;
  result.Set("error", "Not implemented");
  std::move(callback).Run(std::move(result));
}

void AtlasDevToolsProtocolHandler::HandleCancelTask(
    const base::Value::Dict& params,
    base::OnceCallback<void(base::Value::Dict)> callback) {
  base::Value::Dict result;
  result.Set("error", "Not implemented");
  std::move(callback).Run(std::move(result));
}

}  // namespace atlas
