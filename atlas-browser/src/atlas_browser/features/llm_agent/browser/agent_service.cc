// Copyright 2025 Atlas Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license.

#include "atlas_browser/features/llm_agent/browser/agent_service.h"

#include "atlas_browser/features/llm_agent/browser/llm_client.h"
#include "atlas_browser/features/llm_agent/browser/screenshot_capturer.h"
#include "base/guid.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/task/sequenced_task_runner.h"
#include "base/time/time.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"

namespace atlas {

namespace {

// Factory for creating AgentService instances per BrowserContext
class AgentServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  static AgentServiceFactory* GetInstance() {
    static base::NoDestructor<AgentServiceFactory> instance;
    return instance.get();
  }

  static AgentService* GetForBrowserContext(content::BrowserContext* context) {
    return static_cast<AgentService*>(
        GetInstance()->GetServiceForBrowserContext(context, /*create=*/false));
  }

  static AgentService* GetOrCreateForBrowserContext(
      content::BrowserContext* context) {
    return static_cast<AgentService*>(
        GetInstance()->GetServiceForBrowserContext(context, /*create=*/true));
  }

 private:
  friend class base::NoDestructor<AgentServiceFactory>;

  AgentServiceFactory()
      : BrowserContextKeyedServiceFactory(
            "AtlasAgentService",
            BrowserContextDependencyManager::GetInstance()) {}

  ~AgentServiceFactory() override = default;

  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override {
    return new AgentService(context);
  }
};

// Default agent system prompt
constexpr char kAgentSystemPrompt[] = R"(You are an AI browser agent. Your task is to help users accomplish goals by interacting with web pages.

## Capabilities
You can perform these actions:
- click: Click on elements (buttons, links, etc.)
- type: Enter text into input fields
- scroll: Scroll the page or to specific elements
- navigate: Go to a URL
- wait: Wait for elements or conditions
- done: Signal task completion

## Response Format
Always respond with valid JSON:
```json
{
  "thought": "Your reasoning about what to do next",
  "action": {
    "type": "click|type|scroll|navigate|wait|done",
    "selector": "CSS selector (if applicable)",
    "value": "text to type or URL (if applicable)"
  }
}
```

## Guidelines
1. Be methodical - analyze the page before acting
2. Use specific, unique selectors
3. If an action fails, try a different approach
4. Verify success before marking complete
5. Ask for clarification if the task is ambiguous)";

}  // namespace

// =============================================================================
// Static Factory Methods
// =============================================================================

AgentService* AgentService::GetForBrowserContext(
    content::BrowserContext* context) {
  return AgentServiceFactory::GetForBrowserContext(context);
}

AgentService* AgentService::GetOrCreateForBrowserContext(
    content::BrowserContext* context) {
  return AgentServiceFactory::GetOrCreateForBrowserContext(context);
}

// =============================================================================
// Constructor / Destructor
// =============================================================================

AgentService::AgentService(content::BrowserContext* context)
    : browser_context_(context),
      llm_client_(std::make_unique<LLMClient>()),
      screenshot_capturer_(std::make_unique<ScreenshotCapturer>()) {
  DCHECK(context);
  LOG(INFO) << "AgentService created for browser context";

  // Apply default configuration
  Configure(Config());
}

AgentService::~AgentService() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  LOG(INFO) << "AgentService destroyed";
}

void AgentService::Shutdown() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // Cancel all running tasks
  for (auto& [task_id, task] : tasks_) {
    if (task->status == agent::mojom::TaskStatus::kRunning ||
        task->status == agent::mojom::TaskStatus::kPaused) {
      task->status = agent::mojom::TaskStatus::kCancelled;
      NotifyTaskFailed(task_id, "Service shutting down");
    }
  }
  tasks_.clear();

  // Clear Mojo connections
  host_receivers_.Clear();
  renderer_clients_.clear();

  LOG(INFO) << "AgentService shutdown complete";
}

// =============================================================================
// Observer Management
// =============================================================================

void AgentService::AddObserver(AgentServiceObserver* observer) {
  observers_.AddObserver(observer);
}

void AgentService::RemoveObserver(AgentServiceObserver* observer) {
  observers_.RemoveObserver(observer);
}

// =============================================================================
// Configuration
// =============================================================================

void AgentService::Configure(const Config& config) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  config_ = config;

  // Configure LLM client
  LLMClient::Config llm_config;
  llm_config.provider = static_cast<LLMClient::Provider>(config.provider);
  llm_config.model = config.model;
  llm_config.api_key = config.api_key;
  llm_config.endpoint = config.api_endpoint;
  llm_config.enable_vision = config.enable_vision;
  llm_config.temperature = config.temperature;

  llm_client_->Configure(llm_config);

  LOG(INFO) << "AgentService configured:"
            << " provider=" << static_cast<int>(config.provider)
            << " model=" << config.model
            << " vision=" << config.enable_vision;
}

// =============================================================================
// Task Management
// =============================================================================

std::string AgentService::StartTask(content::WebContents* web_contents,
                                    const std::string& task_description,
                                    TaskCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(web_contents);

  // Generate unique task ID
  std::string task_id = base::GenerateGUID();

  // Create task
  auto task = std::make_unique<Task>();
  task->id = task_id;
  task->description = task_description;
  task->web_contents = web_contents;
  task->callback = std::move(callback);
  task->status = agent::mojom::TaskStatus::kPending;
  task->start_time = base::Time::Now();

  tasks_[task_id] = std::move(task);

  LOG(INFO) << "Starting agent task: " << task_id
            << " description: " << task_description;

  NotifyTaskStarted(task_id, task_description);

  // Start the agent loop
  ExecuteAgentLoop(task_id);

  return task_id;
}

void AgentService::PauseTask(const std::string& task_id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  auto it = tasks_.find(task_id);
  if (it == tasks_.end()) {
    LOG(WARNING) << "Cannot pause: task not found: " << task_id;
    return;
  }

  auto& task = it->second;
  if (task->status == agent::mojom::TaskStatus::kRunning) {
    task->status = agent::mojom::TaskStatus::kPaused;
    task->is_paused = true;
    LOG(INFO) << "Task paused: " << task_id;
  }
}

void AgentService::ResumeTask(const std::string& task_id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  auto it = tasks_.find(task_id);
  if (it == tasks_.end()) {
    LOG(WARNING) << "Cannot resume: task not found: " << task_id;
    return;
  }

  auto& task = it->second;
  if (task->status == agent::mojom::TaskStatus::kPaused) {
    task->status = agent::mojom::TaskStatus::kRunning;
    task->is_paused = false;
    LOG(INFO) << "Task resumed: " << task_id;
    ExecuteAgentLoop(task_id);
  }
}

void AgentService::CancelTask(const std::string& task_id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  auto it = tasks_.find(task_id);
  if (it == tasks_.end()) {
    LOG(WARNING) << "Cannot cancel: task not found: " << task_id;
    return;
  }

  auto& task = it->second;
  task->status = agent::mojom::TaskStatus::kCancelled;

  LOG(INFO) << "Task cancelled: " << task_id;

  // Notify observers
  for (auto& observer : observers_) {
    observer.OnTaskCancelled(task_id);
  }

  // Call callback with cancelled result
  if (task->callback) {
    auto result = agent::mojom::TaskResult::New();
    result->status = agent::mojom::TaskStatus::kCancelled;
    result->error_message = "Task cancelled by user";
    std::move(task->callback).Run(std::move(result));
  }
}

agent::mojom::TaskStatus AgentService::GetTaskStatus(
    const std::string& task_id) const {
  auto it = tasks_.find(task_id);
  if (it == tasks_.end()) {
    return agent::mojom::TaskStatus::kFailed;
  }
  return it->second->status;
}

bool AgentService::IsTaskRunning(const std::string& task_id) const {
  auto it = tasks_.find(task_id);
  if (it == tasks_.end()) {
    return false;
  }
  return it->second->status == agent::mojom::TaskStatus::kRunning;
}

// =============================================================================
// Agent Loop
// =============================================================================

void AgentService::ExecuteAgentLoop(const std::string& task_id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  auto it = tasks_.find(task_id);
  if (it == tasks_.end()) {
    LOG(ERROR) << "Task not found: " << task_id;
    return;
  }

  auto& task = it->second;

  // Check if task should continue
  if (task->status == agent::mojom::TaskStatus::kCancelled ||
      task->status == agent::mojom::TaskStatus::kCompleted ||
      task->status == agent::mojom::TaskStatus::kFailed) {
    return;
  }

  if (task->is_paused) {
    return;
  }

  // Check max steps
  if (task->current_step >= config_.max_steps) {
    FailTask(task_id, "Maximum steps exceeded");
    return;
  }

  // Update status
  task->status = agent::mojom::TaskStatus::kRunning;

  // Notify observers that agent is thinking
  for (auto& observer : observers_) {
    observer.OnAgentThinking(task_id);
  }

  // Capture screenshot if vision is enabled
  if (config_.enable_vision) {
    screenshot_capturer_->Capture(
        task->web_contents,
        base::BindOnce(&AgentService::OnScreenshotReady,
                       weak_factory_.GetWeakPtr(), task_id));
  } else {
    OnScreenshotReady(task_id, "");
  }
}

void AgentService::OnScreenshotReady(const std::string& task_id,
                                     const std::string& screenshot_base64) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  auto it = tasks_.find(task_id);
  if (it == tasks_.end() ||
      it->second->status != agent::mojom::TaskStatus::kRunning) {
    return;
  }

  auto& task = it->second;

  // Get DOM from renderer
  auto& renderer = GetRendererClient(task->web_contents);
  if (!renderer) {
    FailTask(task_id, "No renderer connection");
    return;
  }

  renderer->GetAccessibleDOM(base::BindOnce(
      &AgentService::OnDOMReady, weak_factory_.GetWeakPtr(), task_id,
      screenshot_base64));
}

void AgentService::OnDOMReady(
    const std::string& task_id,
    const std::string& screenshot_base64,
    const std::string& dom_json,
    std::vector<agent::mojom::ElementInfoPtr> elements) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  auto it = tasks_.find(task_id);
  if (it == tasks_.end() ||
      it->second->status != agent::mojom::TaskStatus::kRunning) {
    return;
  }

  auto& task = it->second;

  // Build prompt for LLM
  std::string prompt = BuildAgentPrompt(task->description, dom_json, task->steps);

  // Send to LLM
  llm_client_->GetCompletion(
      prompt, screenshot_base64,
      base::BindOnce(&AgentService::OnLLMResponse, weak_factory_.GetWeakPtr(),
                     task_id));
}

void AgentService::OnLLMResponse(const std::string& task_id,
                                 const std::string& response) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  auto it = tasks_.find(task_id);
  if (it == tasks_.end() ||
      it->second->status != agent::mojom::TaskStatus::kRunning) {
    return;
  }

  auto& task = it->second;

  // Parse LLM response
  auto parsed = ParseAgentResponse(response);
  if (!parsed) {
    LOG(ERROR) << "Failed to parse LLM response: " << response;
    // Retry once before failing
    if (task->current_step > 0) {
      FailTask(task_id, "Failed to parse LLM response");
    } else {
      // Retry with a clearer prompt
      ExecuteAgentLoop(task_id);
    }
    return;
  }

  // Create step
  auto step = agent::mojom::AgentStep::New();
  step->step_number = ++task->current_step;
  step->thought = parsed->thought;
  step->action = parsed->action.Clone();
  step->timestamp = base::Time::Now().InSecondsFSinceUnixEpoch();

  // Check if task is complete
  if (step->action->type == agent::mojom::ActionType::kDone) {
    step->result = agent::mojom::ActionResult::New();
    step->result->success = true;
    task->steps.push_back(std::move(step));
    CompleteTask(task_id, agent::mojom::TaskStatus::kCompleted, parsed->thought);
    return;
  }

  // Notify observers
  for (auto& observer : observers_) {
    observer.OnAgentActing(task_id, *step->action);
  }

  // Execute action
  auto& renderer = GetRendererClient(task->web_contents);
  if (!renderer) {
    FailTask(task_id, "Lost renderer connection");
    return;
  }

  renderer->PerformAction(
      step->action.Clone(),
      base::BindOnce(&AgentService::OnActionComplete, weak_factory_.GetWeakPtr(),
                     task_id, std::move(step)));
}

void AgentService::OnActionComplete(const std::string& task_id,
                                    agent::mojom::AgentStepPtr step,
                                    agent::mojom::ActionResultPtr result) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  auto it = tasks_.find(task_id);
  if (it == tasks_.end()) {
    return;
  }

  auto& task = it->second;

  // Record result
  step->result = std::move(result);
  step->duration_ms = (base::Time::Now().InSecondsFSinceUnixEpoch() -
                       step->timestamp) * 1000;

  // Notify observers
  NotifyTaskProgress(task_id, *step);

  // Store step
  task->steps.push_back(std::move(step));

  // Continue loop after a short delay
  base::SequencedTaskRunner::GetCurrentDefault()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&AgentService::ExecuteAgentLoop, weak_factory_.GetWeakPtr(),
                     task_id),
      base::Milliseconds(500));
}

// =============================================================================
// Task Completion
// =============================================================================

void AgentService::CompleteTask(const std::string& task_id,
                                agent::mojom::TaskStatus status,
                                const std::string& message) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  auto it = tasks_.find(task_id);
  if (it == tasks_.end()) {
    return;
  }

  auto& task = it->second;
  task->status = status;

  // Build result
  auto result = agent::mojom::TaskResult::New();
  result->status = status;
  result->final_answer = message;
  result->duration_seconds =
      (base::Time::Now() - task->start_time).InSecondsF();

  // Copy steps
  for (const auto& step : task->steps) {
    result->steps.push_back(step.Clone());
  }

  LOG(INFO) << "Task completed: " << task_id << " status=" << static_cast<int>(status)
            << " steps=" << result->steps.size();

  // Notify observers
  NotifyTaskCompleted(task_id, *result);

  // Call callback
  if (task->callback) {
    std::move(task->callback).Run(std::move(result));
  }
}

void AgentService::FailTask(const std::string& task_id,
                            const std::string& error) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  LOG(ERROR) << "Task failed: " << task_id << " error=" << error;

  auto it = tasks_.find(task_id);
  if (it == tasks_.end()) {
    return;
  }

  auto& task = it->second;
  task->status = agent::mojom::TaskStatus::kFailed;

  // Notify observers
  NotifyTaskFailed(task_id, error);

  // Build result
  auto result = agent::mojom::TaskResult::New();
  result->status = agent::mojom::TaskStatus::kFailed;
  result->error_message = error;
  result->duration_seconds =
      (base::Time::Now() - task->start_time).InSecondsF();

  for (const auto& step : task->steps) {
    result->steps.push_back(step.Clone());
  }

  if (task->callback) {
    std::move(task->callback).Run(std::move(result));
  }
}

// =============================================================================
// Prompt Building
// =============================================================================

std::string AgentService::BuildAgentPrompt(
    const std::string& task,
    const std::string& dom_json,
    const std::vector<agent::mojom::AgentStepPtr>& history) {
  std::ostringstream prompt;

  prompt << kAgentSystemPrompt << "\n\n";

  prompt << "## Current Task\n" << task << "\n\n";

  prompt << "## Current Page DOM\n"
         << "```json\n"
         << dom_json << "\n```\n\n";

  if (!history.empty()) {
    prompt << "## Previous Actions\n";
    for (const auto& step : history) {
      prompt << "**Step " << step->step_number << "**\n"
             << "- Thought: " << step->thought << "\n"
             << "- Action: " << static_cast<int>(step->action->type);
      if (step->action->selector) {
        prompt << " on `" << *step->action->selector << "`";
      }
      if (step->action->value) {
        prompt << " with value \"" << *step->action->value << "\"";
      }
      prompt << "\n"
             << "- Result: "
             << (step->result->success ? "Success" : "Failed");
      if (step->result->error_message) {
        prompt << " (" << *step->result->error_message << ")";
      }
      prompt << "\n\n";
    }
  }

  prompt << "## Your Response\n"
         << "Analyze the current state and decide on the next action. "
         << "Respond with valid JSON only.\n";

  return prompt.str();
}

// =============================================================================
// Response Parsing
// =============================================================================

std::optional<AgentService::ParsedResponse> AgentService::ParseAgentResponse(
    const std::string& response) {
  // Find JSON in response (may be wrapped in markdown code blocks)
  std::string json_str = response;

  // Extract from code block if present
  size_t json_start = response.find("```json");
  if (json_start != std::string::npos) {
    json_start = response.find('\n', json_start) + 1;
    size_t json_end = response.find("```", json_start);
    if (json_end != std::string::npos) {
      json_str = response.substr(json_start, json_end - json_start);
    }
  } else {
    // Try to find raw JSON
    json_start = response.find('{');
    size_t json_end = response.rfind('}');
    if (json_start != std::string::npos && json_end != std::string::npos) {
      json_str = response.substr(json_start, json_end - json_start + 1);
    }
  }

  // Parse JSON
  auto parsed = base::JSONReader::Read(json_str);
  if (!parsed || !parsed->is_dict()) {
    LOG(ERROR) << "Invalid JSON in response";
    return std::nullopt;
  }

  const auto& dict = parsed->GetDict();

  ParsedResponse result;

  // Extract thought
  const std::string* thought = dict.FindString("thought");
  if (thought) {
    result.thought = *thought;
  }

  // Extract action
  const base::Value::Dict* action_dict = dict.FindDict("action");
  if (!action_dict) {
    LOG(ERROR) << "No action in response";
    return std::nullopt;
  }

  result.action = agent::mojom::AgentAction::New();

  // Parse action type
  const std::string* type_str = action_dict->FindString("type");
  if (!type_str) {
    LOG(ERROR) << "No action type";
    return std::nullopt;
  }

  static const std::unordered_map<std::string, agent::mojom::ActionType>
      kActionTypes = {
          {"click", agent::mojom::ActionType::kClick},
          {"type", agent::mojom::ActionType::kType},
          {"scroll", agent::mojom::ActionType::kScroll},
          {"hover", agent::mojom::ActionType::kHover},
          {"press_key", agent::mojom::ActionType::kPressKey},
          {"navigate", agent::mojom::ActionType::kNavigate},
          {"wait", agent::mojom::ActionType::kWait},
          {"screenshot", agent::mojom::ActionType::kScreenshot},
          {"extract", agent::mojom::ActionType::kExtract},
          {"done", agent::mojom::ActionType::kDone},
      };

  auto type_it = kActionTypes.find(base::ToLowerASCII(*type_str));
  if (type_it == kActionTypes.end()) {
    LOG(ERROR) << "Unknown action type: " << *type_str;
    return std::nullopt;
  }
  result.action->type = type_it->second;

  // Parse optional fields
  if (const std::string* selector = action_dict->FindString("selector")) {
    result.action->selector = *selector;
  }
  if (const std::string* value = action_dict->FindString("value")) {
    result.action->value = *value;
  }
  if (auto x = action_dict->FindInt("x")) {
    result.action->x = *x;
  }
  if (auto y = action_dict->FindInt("y")) {
    result.action->y = *y;
  }
  if (const std::string* key = action_dict->FindString("key")) {
    result.action->key = *key;
  }
  if (auto delay = action_dict->FindInt("delay_ms")) {
    result.action->delay_ms = *delay;
  }

  return result;
}

// =============================================================================
// Mojo Binding
// =============================================================================

void AgentService::BindAgentHost(
    content::RenderFrameHost* frame_host,
    mojo::PendingReceiver<agent::mojom::AgentBrowserHost> receiver) {
  host_receivers_.Add(this, std::move(receiver));
}

mojo::Remote<agent::mojom::AgentRendererClient>&
AgentService::GetRendererClient(content::WebContents* web_contents) {
  // Note: In a real implementation, this would get/create a Mojo connection
  // to the renderer process for this WebContents
  return renderer_clients_[web_contents];
}

// =============================================================================
// AgentBrowserHost Implementation
// =============================================================================

void AgentService::OnNavigationStarted(const GURL& url) {
  LOG(INFO) << "Navigation started: " << url;
}

void AgentService::OnNavigationCompleted(const GURL& url, bool success) {
  LOG(INFO) << "Navigation completed: " << url << " success=" << success;
}

void AgentService::OnNavigationFailed(const GURL& url,
                                      const std::string& error) {
  LOG(ERROR) << "Navigation failed: " << url << " error=" << error;
}

void AgentService::OnPageStateChanged(agent::mojom::PageStatePtr state) {
  LOG(INFO) << "Page state changed: " << state->url.spec()
            << " loading=" << state->is_loading;
}

void AgentService::OnDOMContentLoaded() {
  LOG(INFO) << "DOM content loaded";
}

void AgentService::OnPageFullyLoaded() {
  LOG(INFO) << "Page fully loaded";
}

void AgentService::OnDOMMutation(const std::string& mutation_summary) {
  if (config_.verbose_logging) {
    LOG(INFO) << "DOM mutation: " << mutation_summary;
  }
}

void AgentService::RequestAgentDecision(
    const std::string& task,
    const std::string& current_state_json,
    const std::optional<std::string>& screenshot_base64,
    RequestAgentDecisionCallback callback) {
  // This is called by the renderer when it needs an agent decision
  // For now, just return a wait action
  auto action = agent::mojom::AgentAction::New();
  action->type = agent::mojom::ActionType::kWait;
  action->delay_ms = 1000;
  std::move(callback).Run(std::move(action));
}

void AgentService::LogAgentStep(agent::mojom::AgentStepPtr step) {
  LOG(INFO) << "Agent step " << step->step_number << ": " << step->thought;
}

void AgentService::LogAgentError(const std::string& error) {
  LOG(ERROR) << "Agent error: " << error;
}

void AgentService::OnAlertOpened(const std::string& text) {
  LOG(INFO) << "Alert opened: " << text;
}

void AgentService::OnAlertClosed(bool accepted) {
  LOG(INFO) << "Alert closed: accepted=" << accepted;
}

void AgentService::OnElementClicked(const std::string& selector) {
  if (config_.verbose_logging) {
    LOG(INFO) << "Element clicked: " << selector;
  }
}

void AgentService::OnElementFocused(const std::string& selector) {
  if (config_.verbose_logging) {
    LOG(INFO) << "Element focused: " << selector;
  }
}

void AgentService::OnFormSubmitted(const std::string& form_selector) {
  LOG(INFO) << "Form submitted: " << form_selector;
}

// =============================================================================
// Observer Notifications
// =============================================================================

void AgentService::NotifyTaskStarted(const std::string& task_id,
                                     const std::string& description) {
  for (auto& observer : observers_) {
    observer.OnTaskStarted(task_id, description);
  }
}

void AgentService::NotifyTaskProgress(const std::string& task_id,
                                      const agent::mojom::AgentStep& step) {
  for (auto& observer : observers_) {
    observer.OnTaskProgress(task_id, step);
  }
}

void AgentService::NotifyTaskCompleted(const std::string& task_id,
                                       const agent::mojom::TaskResult& result) {
  for (auto& observer : observers_) {
    observer.OnTaskCompleted(task_id, result);
  }
}

void AgentService::NotifyTaskFailed(const std::string& task_id,
                                    const std::string& error) {
  for (auto& observer : observers_) {
    observer.OnTaskFailed(task_id, error);
  }
}

}  // namespace atlas
