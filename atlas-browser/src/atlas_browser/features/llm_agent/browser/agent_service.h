// Copyright 2025 Atlas Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license.

#ifndef ATLAS_BROWSER_FEATURES_LLM_AGENT_BROWSER_AGENT_SERVICE_H_
#define ATLAS_BROWSER_FEATURES_LLM_AGENT_BROWSER_AGENT_SERVICE_H_

#include <memory>
#include <string>
#include <unordered_map>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/sequence_checker.h"
#include "components/keyed_service/core/keyed_service.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "atlas_browser/features/llm_agent/public/mojom/agent.mojom.h"

namespace content {
class BrowserContext;
class RenderFrameHost;
class WebContents;
}  // namespace content

namespace atlas {

class LLMClient;
class TaskManager;
class ScreenshotCapturer;

// =============================================================================
// AgentServiceObserver
// =============================================================================

// Observer interface for agent events
class AgentServiceObserver : public base::CheckedObserver {
 public:
  virtual void OnTaskStarted(const std::string& task_id,
                             const std::string& description) {}
  virtual void OnTaskProgress(const std::string& task_id,
                              const agent::mojom::AgentStep& step) {}
  virtual void OnTaskCompleted(const std::string& task_id,
                               const agent::mojom::TaskResult& result) {}
  virtual void OnTaskFailed(const std::string& task_id,
                            const std::string& error) {}
  virtual void OnTaskCancelled(const std::string& task_id) {}
  virtual void OnAgentThinking(const std::string& task_id) {}
  virtual void OnAgentActing(const std::string& task_id,
                             const agent::mojom::AgentAction& action) {}
};

// =============================================================================
// AgentService
// =============================================================================

// The AgentService manages AI agent tasks for browser automation.
// It coordinates between the LLM, the renderer (for DOM access), and the UI.
class AgentService : public KeyedService,
                     public agent::mojom::AgentBrowserHost {
 public:
  // Configuration for the agent service
  struct Config {
    enum class Provider {
      kLocal,      // Ollama, llama.cpp
      kOpenAI,     // OpenAI API
      kAnthropic,  // Anthropic API
      kCustom,     // Custom endpoint
    };

    Provider provider = Provider::kAnthropic;
    std::string model = "claude-sonnet-4-20250514";
    std::string api_key;
    std::string api_endpoint;

    int max_steps = 50;
    int step_timeout_ms = 30000;
    int task_timeout_ms = 300000;

    bool enable_vision = true;
    bool enable_element_labels = true;
    bool verbose_logging = false;

    float temperature = 0.0f;

    std::vector<std::string> blocked_domains;
  };

  // Get or create the service for a browser context
  static AgentService* GetForBrowserContext(content::BrowserContext* context);
  static AgentService* GetOrCreateForBrowserContext(
      content::BrowserContext* context);

  explicit AgentService(content::BrowserContext* context);
  ~AgentService() override;

  AgentService(const AgentService&) = delete;
  AgentService& operator=(const AgentService&) = delete;

  // KeyedService implementation
  void Shutdown() override;

  // ==========================================================================
  // Observer Management
  // ==========================================================================

  void AddObserver(AgentServiceObserver* observer);
  void RemoveObserver(AgentServiceObserver* observer);

  // ==========================================================================
  // Task Management
  // ==========================================================================

  // Callback types
  using TaskCallback =
      base::OnceCallback<void(agent::mojom::TaskResultPtr result)>;
  using ActionCallback =
      base::OnceCallback<void(agent::mojom::ActionResultPtr result)>;
  using QueryCallback = base::OnceCallback<void(const std::string& answer)>;

  // Start a new agent task
  std::string StartTask(content::WebContents* web_contents,
                        const std::string& task_description,
                        TaskCallback callback);

  // Task control
  void PauseTask(const std::string& task_id);
  void ResumeTask(const std::string& task_id);
  void CancelTask(const std::string& task_id);

  // Get task status
  agent::mojom::TaskStatus GetTaskStatus(const std::string& task_id) const;
  bool IsTaskRunning(const std::string& task_id) const;

  // ==========================================================================
  // Single Actions
  // ==========================================================================

  // Perform a single action without a full task
  void PerformAction(content::WebContents* web_contents,
                     agent::mojom::AgentActionPtr action,
                     ActionCallback callback);

  // Query page with natural language
  void QueryPage(content::WebContents* web_contents,
                 const std::string& question,
                 QueryCallback callback);

  // ==========================================================================
  // Configuration
  // ==========================================================================

  void Configure(const Config& config);
  const Config& GetConfig() const { return config_; }

  // ==========================================================================
  // Mojo Binding
  // ==========================================================================

  void BindAgentHost(
      content::RenderFrameHost* frame_host,
      mojo::PendingReceiver<agent::mojom::AgentBrowserHost> receiver);

  // ==========================================================================
  // AgentBrowserHost Implementation (Mojo interface)
  // ==========================================================================

  void OnNavigationStarted(const GURL& url) override;
  void OnNavigationCompleted(const GURL& url, bool success) override;
  void OnNavigationFailed(const GURL& url, const std::string& error) override;
  void OnPageStateChanged(agent::mojom::PageStatePtr state) override;
  void OnDOMContentLoaded() override;
  void OnPageFullyLoaded() override;
  void OnDOMMutation(const std::string& mutation_summary) override;
  void RequestAgentDecision(
      const std::string& task,
      const std::string& current_state_json,
      const std::optional<std::string>& screenshot_base64,
      RequestAgentDecisionCallback callback) override;
  void LogAgentStep(agent::mojom::AgentStepPtr step) override;
  void LogAgentError(const std::string& error) override;
  void OnAlertOpened(const std::string& text) override;
  void OnAlertClosed(bool accepted) override;
  void OnElementClicked(const std::string& selector) override;
  void OnElementFocused(const std::string& selector) override;
  void OnFormSubmitted(const std::string& form_selector) override;

 private:
  // Internal task structure
  struct Task {
    std::string id;
    std::string description;
    raw_ptr<content::WebContents> web_contents;
    TaskCallback callback;
    agent::mojom::TaskStatus status;
    std::vector<agent::mojom::AgentStepPtr> steps;
    base::Time start_time;
    int current_step = 0;
    bool is_paused = false;
  };

  // Agent loop execution
  void ExecuteAgentLoop(const std::string& task_id);
  void OnScreenshotReady(const std::string& task_id,
                         const std::string& screenshot_base64);
  void OnDOMReady(const std::string& task_id,
                  const std::string& screenshot_base64,
                  const std::string& dom_json,
                  std::vector<agent::mojom::ElementInfoPtr> elements);
  void OnLLMResponse(const std::string& task_id, const std::string& response);
  void OnActionComplete(const std::string& task_id,
                        agent::mojom::AgentStepPtr step,
                        agent::mojom::ActionResultPtr result);

  // Task completion
  void CompleteTask(const std::string& task_id,
                    agent::mojom::TaskStatus status,
                    const std::string& message);
  void FailTask(const std::string& task_id, const std::string& error);

  // Prompt building
  std::string BuildAgentPrompt(
      const std::string& task,
      const std::string& dom_json,
      const std::vector<agent::mojom::AgentStepPtr>& history);

  // Response parsing
  struct ParsedResponse {
    std::string thought;
    agent::mojom::AgentActionPtr action;
  };
  std::optional<ParsedResponse> ParseAgentResponse(const std::string& response);

  // Helper to get renderer interface
  mojo::Remote<agent::mojom::AgentRendererClient>& GetRendererClient(
      content::WebContents* web_contents);

  // Notify observers
  void NotifyTaskStarted(const std::string& task_id,
                         const std::string& description);
  void NotifyTaskProgress(const std::string& task_id,
                          const agent::mojom::AgentStep& step);
  void NotifyTaskCompleted(const std::string& task_id,
                           const agent::mojom::TaskResult& result);
  void NotifyTaskFailed(const std::string& task_id, const std::string& error);

  // Browser context this service belongs to
  raw_ptr<content::BrowserContext> browser_context_;

  // Configuration
  Config config_;

  // LLM client for generating agent decisions
  std::unique_ptr<LLMClient> llm_client_;

  // Screenshot capturer
  std::unique_ptr<ScreenshotCapturer> screenshot_capturer_;

  // Active tasks
  std::unordered_map<std::string, std::unique_ptr<Task>> tasks_;

  // Renderer client connections (per WebContents)
  std::map<content::WebContents*,
           mojo::Remote<agent::mojom::AgentRendererClient>>
      renderer_clients_;

  // Mojo receivers for browser host interface
  mojo::ReceiverSet<agent::mojom::AgentBrowserHost> host_receivers_;

  // Observers
  base::ObserverList<AgentServiceObserver> observers_;

  SEQUENCE_CHECKER(sequence_checker_);

  base::WeakPtrFactory<AgentService> weak_factory_{this};
};

}  // namespace atlas

#endif  // ATLAS_BROWSER_FEATURES_LLM_AGENT_BROWSER_AGENT_SERVICE_H_
