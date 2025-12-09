// Copyright 2025 Atlas Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license.

#include "atlas_browser/extensions/api/agent/agent_api.h"

#include "atlas_browser/features/llm_agent/browser/agent_service.h"
#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/values.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/web_contents.h"
#include "extensions/browser/extension_function.h"

namespace atlas {

namespace {

// Convert TaskResult to JSON value
base::Value::Dict TaskResultToValue(
    const agent::mojom::TaskResult& result) {
  base::Value::Dict dict;
  
  // Status
  switch (result.status) {
    case agent::mojom::TaskStatus::kCompleted:
      dict.Set("status", "completed");
      break;
    case agent::mojom::TaskStatus::kFailed:
      dict.Set("status", "failed");
      break;
    case agent::mojom::TaskStatus::kCancelled:
      dict.Set("status", "cancelled");
      break;
    default:
      dict.Set("status", "unknown");
  }

  // Answer
  if (result.final_answer) {
    dict.Set("answer", *result.final_answer);
  }

  // Duration
  dict.Set("duration", result.duration_seconds);

  // Steps
  base::Value::List steps_list;
  for (const auto& step : result.steps) {
    base::Value::Dict step_dict;
    step_dict.Set("stepNumber", step->step_number);
    step_dict.Set("thought", step->thought);
    step_dict.Set("actionType", static_cast<int>(step->action->type));
    if (step->action->selector) {
      step_dict.Set("selector", *step->action->selector);
    }
    if (step->action->value) {
      step_dict.Set("value", *step->action->value);
    }
    step_dict.Set("success", step->result->success);
    if (step->result->error_message) {
      step_dict.Set("error", *step->result->error_message);
    }
    steps_list.Append(std::move(step_dict));
  }
  dict.Set("steps", std::move(steps_list));

  return dict;
}

}  // namespace

// =============================================================================
// AgentRunTaskFunction
// =============================================================================

AgentRunTaskFunction::AgentRunTaskFunction() = default;
AgentRunTaskFunction::~AgentRunTaskFunction() = default;

ExtensionFunction::ResponseAction AgentRunTaskFunction::Run() {
  // Parse arguments
  EXTENSION_FUNCTION_VALIDATE(args().size() >= 1);
  EXTENSION_FUNCTION_VALIDATE(args()[0].is_string());
  
  std::string task = args()[0].GetString();

  // Parse options
  int max_steps = 50;
  int timeout_ms = 300000;
  bool enable_vision = true;

  if (args().size() > 1 && args()[1].is_dict()) {
    const auto& options = args()[1].GetDict();
    
    if (auto* val = options.FindInt("maxSteps")) {
      max_steps = *val;
    }
    if (auto* val = options.FindInt("timeout")) {
      timeout_ms = *val;
    }
    if (auto* val = options.FindBool("enableVision")) {
      enable_vision = *val;
    }
  }

  // Get web contents
  content::WebContents* web_contents = GetSenderWebContents();
  if (!web_contents) {
    return RespondNow(Error("No active tab"));
  }

  // Get agent service
  Profile* profile = Profile::FromBrowserContext(browser_context());
  AgentService* agent_service = AgentService::GetForBrowserContext(profile);
  if (!agent_service) {
    return RespondNow(Error("Agent service not available"));
  }

  // Configure for this task
  AgentService::Config config = agent_service->GetConfig();
  config.max_steps = max_steps;
  config.task_timeout_ms = timeout_ms;
  config.enable_vision = enable_vision;
  agent_service->Configure(config);

  // Start task
  agent_service->StartTask(
      web_contents, task,
      base::BindOnce(
          [](AgentRunTaskFunction* self,
             agent::mojom::TaskResultPtr result) {
            base::Value::Dict result_dict = TaskResultToValue(*result);
            self->Respond(WithArguments(std::move(result_dict)));
          },
          base::RetainedRef(this)));

  return RespondLater();
}

// =============================================================================
// AgentCancelTaskFunction
// =============================================================================

AgentCancelTaskFunction::AgentCancelTaskFunction() = default;
AgentCancelTaskFunction::~AgentCancelTaskFunction() = default;

ExtensionFunction::ResponseAction AgentCancelTaskFunction::Run() {
  EXTENSION_FUNCTION_VALIDATE(args().size() >= 1);
  EXTENSION_FUNCTION_VALIDATE(args()[0].is_string());
  
  std::string task_id = args()[0].GetString();

  Profile* profile = Profile::FromBrowserContext(browser_context());
  AgentService* agent_service = AgentService::GetForBrowserContext(profile);
  if (!agent_service) {
    return RespondNow(Error("Agent service not available"));
  }

  agent_service->CancelTask(task_id);
  return RespondNow(WithArguments(true));
}

// =============================================================================
// AgentQueryPageFunction
// =============================================================================

AgentQueryPageFunction::AgentQueryPageFunction() = default;
AgentQueryPageFunction::~AgentQueryPageFunction() = default;

ExtensionFunction::ResponseAction AgentQueryPageFunction::Run() {
  EXTENSION_FUNCTION_VALIDATE(args().size() >= 1);
  EXTENSION_FUNCTION_VALIDATE(args()[0].is_string());
  
  std::string question = args()[0].GetString();

  content::WebContents* web_contents = GetSenderWebContents();
  if (!web_contents) {
    return RespondNow(Error("No active tab"));
  }

  Profile* profile = Profile::FromBrowserContext(browser_context());
  AgentService* agent_service = AgentService::GetForBrowserContext(profile);
  if (!agent_service) {
    return RespondNow(Error("Agent service not available"));
  }

  agent_service->QueryPage(
      web_contents, question,
      base::BindOnce(
          [](AgentQueryPageFunction* self, const std::string& answer) {
            self->Respond(WithArguments(answer));
          },
          base::RetainedRef(this)));

  return RespondLater();
}

// =============================================================================
// AgentGetElementsFunction
// =============================================================================

AgentGetElementsFunction::AgentGetElementsFunction() = default;
AgentGetElementsFunction::~AgentGetElementsFunction() = default;

ExtensionFunction::ResponseAction AgentGetElementsFunction::Run() {
  // Parse options
  std::string selector;
  bool include_hidden = false;

  if (args().size() > 0 && args()[0].is_dict()) {
    const auto& options = args()[0].GetDict();
    
    if (auto* val = options.FindString("selector")) {
      selector = *val;
    }
    if (auto* val = options.FindBool("includeHidden")) {
      include_hidden = *val;
    }
  }

  content::WebContents* web_contents = GetSenderWebContents();
  if (!web_contents) {
    return RespondNow(Error("No active tab"));
  }

  // Get elements via agent service
  Profile* profile = Profile::FromBrowserContext(browser_context());
  AgentService* agent_service = AgentService::GetForBrowserContext(profile);
  if (!agent_service) {
    return RespondNow(Error("Agent service not available"));
  }

  agent_service->GetDOM(
      web_contents,
      base::BindOnce(
          [](AgentGetElementsFunction* self,
             const std::string& dom_json,
             std::vector<agent::mojom::ElementInfoPtr> elements) {
            base::Value::List elements_list;
            for (const auto& elem : elements) {
              base::Value::Dict elem_dict;
              elem_dict.Set("tag", elem->tag_name);
              if (elem->id) elem_dict.Set("id", *elem->id);
              if (elem->class_name) elem_dict.Set("class", *elem->class_name);
              if (elem->text_content) elem_dict.Set("text", *elem->text_content);
              elem_dict.Set("selector", elem->css_selector);
              elem_dict.Set("visible", elem->is_visible);
              elem_dict.Set("clickable", elem->is_clickable);
              
              base::Value::Dict bounds;
              bounds.Set("x", elem->bounding_box.x());
              bounds.Set("y", elem->bounding_box.y());
              bounds.Set("width", elem->bounding_box.width());
              bounds.Set("height", elem->bounding_box.height());
              elem_dict.Set("bounds", std::move(bounds));
              
              elements_list.Append(std::move(elem_dict));
            }
            self->Respond(WithArguments(std::move(elements_list)));
          },
          base::RetainedRef(this)));

  return RespondLater();
}

// =============================================================================
// AgentPerformActionFunction
// =============================================================================

AgentPerformActionFunction::AgentPerformActionFunction() = default;
AgentPerformActionFunction::~AgentPerformActionFunction() = default;

ExtensionFunction::ResponseAction AgentPerformActionFunction::Run() {
  EXTENSION_FUNCTION_VALIDATE(args().size() >= 1);
  EXTENSION_FUNCTION_VALIDATE(args()[0].is_dict());
  
  const auto& action_dict = args()[0].GetDict();

  // Parse action
  auto action = agent::mojom::AgentAction::New();

  const std::string* type_str = action_dict.FindString("type");
  if (!type_str) {
    return RespondNow(Error("Missing action type"));
  }

  // Map string to ActionType
  if (*type_str == "click") {
    action->type = agent::mojom::ActionType::kClick;
  } else if (*type_str == "type") {
    action->type = agent::mojom::ActionType::kType;
  } else if (*type_str == "scroll") {
    action->type = agent::mojom::ActionType::kScroll;
  } else if (*type_str == "hover") {
    action->type = agent::mojom::ActionType::kHover;
  } else if (*type_str == "navigate") {
    action->type = agent::mojom::ActionType::kNavigate;
  } else {
    return RespondNow(Error("Invalid action type"));
  }

  // Optional fields
  if (auto* val = action_dict.FindString("selector")) {
    action->selector = *val;
  }
  if (auto* val = action_dict.FindString("value")) {
    action->value = *val;
  }
  if (auto val = action_dict.FindInt("x")) {
    action->x = *val;
  }
  if (auto val = action_dict.FindInt("y")) {
    action->y = *val;
  }

  content::WebContents* web_contents = GetSenderWebContents();
  if (!web_contents) {
    return RespondNow(Error("No active tab"));
  }

  Profile* profile = Profile::FromBrowserContext(browser_context());
  AgentService* agent_service = AgentService::GetForBrowserContext(profile);
  if (!agent_service) {
    return RespondNow(Error("Agent service not available"));
  }

  agent_service->PerformAction(
      web_contents, std::move(action),
      base::BindOnce(
          [](AgentPerformActionFunction* self,
             agent::mojom::ActionResultPtr result) {
            base::Value::Dict result_dict;
            result_dict.Set("success", result->success);
            if (result->error_message) {
              result_dict.Set("error", *result->error_message);
            }
            self->Respond(WithArguments(std::move(result_dict)));
          },
          base::RetainedRef(this)));

  return RespondLater();
}

// =============================================================================
// AgentGetStatusFunction
// =============================================================================

AgentGetStatusFunction::AgentGetStatusFunction() = default;
AgentGetStatusFunction::~AgentGetStatusFunction() = default;

ExtensionFunction::ResponseAction AgentGetStatusFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  AgentService* agent_service = AgentService::GetForBrowserContext(profile);

  base::Value::Dict status;
  
  if (agent_service) {
    const auto& config = agent_service->GetConfig();
    status.Set("enabled", true);
    status.Set("provider", 
               config.provider == AgentService::Config::Provider::kAnthropic 
                   ? "anthropic" 
                   : config.provider == AgentService::Config::Provider::kOpenAI 
                       ? "openai" 
                       : "local");
    status.Set("model", config.model);
    status.Set("maxSteps", config.max_steps);
    status.Set("enableVision", config.enable_vision);
    // Count running tasks would require additional tracking
    status.Set("runningTasks", 0);
  } else {
    status.Set("enabled", false);
    status.Set("runningTasks", 0);
  }

  return RespondNow(WithArguments(std::move(status)));
}

// =============================================================================
// AgentConfigureFunction
// =============================================================================

AgentConfigureFunction::AgentConfigureFunction() = default;
AgentConfigureFunction::~AgentConfigureFunction() = default;

ExtensionFunction::ResponseAction AgentConfigureFunction::Run() {
  EXTENSION_FUNCTION_VALIDATE(args().size() >= 1);
  EXTENSION_FUNCTION_VALIDATE(args()[0].is_dict());
  
  const auto& config_dict = args()[0].GetDict();

  Profile* profile = Profile::FromBrowserContext(browser_context());
  AgentService* agent_service = AgentService::GetForBrowserContext(profile);
  if (!agent_service) {
    return RespondNow(Error("Agent service not available"));
  }

  AgentService::Config config = agent_service->GetConfig();

  // Update from provided values
  if (auto* val = config_dict.FindString("provider")) {
    if (*val == "anthropic") {
      config.provider = AgentService::Config::Provider::kAnthropic;
    } else if (*val == "openai") {
      config.provider = AgentService::Config::Provider::kOpenAI;
    } else if (*val == "local") {
      config.provider = AgentService::Config::Provider::kLocal;
    }
  }
  if (auto* val = config_dict.FindString("model")) {
    config.model = *val;
  }
  if (auto* val = config_dict.FindString("apiKey")) {
    config.api_key = *val;
  }
  if (auto val = config_dict.FindInt("maxSteps")) {
    config.max_steps = *val;
  }
  if (auto* val = config_dict.FindBool("enableVision")) {
    config.enable_vision = *val;
  }

  agent_service->Configure(config);
  return RespondNow(WithArguments(true));
}

}  // namespace atlas
