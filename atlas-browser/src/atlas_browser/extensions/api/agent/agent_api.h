// Copyright 2025 Atlas Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license.

#ifndef ATLAS_BROWSER_EXTENSIONS_API_AGENT_AGENT_API_H_
#define ATLAS_BROWSER_EXTENSIONS_API_AGENT_AGENT_API_H_

#include "extensions/browser/extension_function.h"

namespace atlas {

// =============================================================================
// atlas.agent.runTask
// =============================================================================
// Runs an AI agent task on the current tab.
//
// Parameters:
//   task: string - Natural language description of the task
//   options: object (optional)
//     maxSteps: number - Maximum steps (default: 50)
//     timeout: number - Timeout in ms (default: 300000)
//     enableVision: boolean - Use screenshots (default: true)
//
// Returns:
//   Promise<TaskResult>
//     status: "completed" | "failed" | "cancelled"
//     answer: string - Final answer from agent
//     steps: array - Steps taken
//     duration: number - Total duration in seconds
//
class AgentRunTaskFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("atlas.agent.runTask", ATLAS_AGENT_RUNTASK)

  AgentRunTaskFunction();

 protected:
  ~AgentRunTaskFunction() override;

  // ExtensionFunction:
  ResponseAction Run() override;

 private:
  void OnTaskComplete(const std::string& result_json);
};

// =============================================================================
// atlas.agent.cancelTask
// =============================================================================
// Cancels a running agent task.
//
// Parameters:
//   taskId: string - ID of the task to cancel
//
// Returns:
//   Promise<boolean> - true if cancelled successfully
//
class AgentCancelTaskFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("atlas.agent.cancelTask", ATLAS_AGENT_CANCELTASK)

  AgentCancelTaskFunction();

 protected:
  ~AgentCancelTaskFunction() override;

  ResponseAction Run() override;
};

// =============================================================================
// atlas.agent.queryPage
// =============================================================================
// Query the current page with natural language.
//
// Parameters:
//   question: string - Natural language question about the page
//
// Returns:
//   Promise<string> - Answer from the LLM
//
class AgentQueryPageFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("atlas.agent.queryPage", ATLAS_AGENT_QUERYPAGE)

  AgentQueryPageFunction();

 protected:
  ~AgentQueryPageFunction() override;

  ResponseAction Run() override;

 private:
  void OnQueryComplete(const std::string& answer);
};

// =============================================================================
// atlas.agent.getElements
// =============================================================================
// Get interactive elements from the current page.
//
// Parameters:
//   options: object (optional)
//     selector: string - Filter by CSS selector
//     includeHidden: boolean - Include hidden elements
//
// Returns:
//   Promise<ElementInfo[]> - Array of element information
//
class AgentGetElementsFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("atlas.agent.getElements", ATLAS_AGENT_GETELEMENTS)

  AgentGetElementsFunction();

 protected:
  ~AgentGetElementsFunction() override;

  ResponseAction Run() override;

 private:
  void OnElementsReady(const std::string& elements_json);
};

// =============================================================================
// atlas.agent.performAction
// =============================================================================
// Perform a single action on the page.
//
// Parameters:
//   action: object
//     type: "click" | "type" | "scroll" | "hover" | "navigate"
//     selector: string (optional) - CSS selector for target
//     value: string (optional) - Value for type/navigate actions
//     x: number (optional) - X coordinate
//     y: number (optional) - Y coordinate
//
// Returns:
//   Promise<ActionResult>
//     success: boolean
//     error: string (optional)
//
class AgentPerformActionFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("atlas.agent.performAction", 
                             ATLAS_AGENT_PERFORMACTION)

  AgentPerformActionFunction();

 protected:
  ~AgentPerformActionFunction() override;

  ResponseAction Run() override;

 private:
  void OnActionComplete(const std::string& result_json);
};

// =============================================================================
// atlas.agent.getStatus
// =============================================================================
// Get the status of the agent service.
//
// Returns:
//   Promise<AgentStatus>
//     enabled: boolean
//     provider: string
//     model: string
//     runningTasks: number
//
class AgentGetStatusFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("atlas.agent.getStatus", ATLAS_AGENT_GETSTATUS)

  AgentGetStatusFunction();

 protected:
  ~AgentGetStatusFunction() override;

  ResponseAction Run() override;
};

// =============================================================================
// atlas.agent.configure
// =============================================================================
// Configure the agent service.
//
// Parameters:
//   config: object
//     provider: string - "anthropic" | "openai" | "local"
//     model: string - Model name
//     apiKey: string - API key
//     maxSteps: number - Max steps per task
//     enableVision: boolean - Enable screenshots
//
// Returns:
//   Promise<boolean> - true if configured successfully
//
class AgentConfigureFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("atlas.agent.configure", ATLAS_AGENT_CONFIGURE)

  AgentConfigureFunction();

 protected:
  ~AgentConfigureFunction() override;

  ResponseAction Run() override;
};

}  // namespace atlas

#endif  // ATLAS_BROWSER_EXTENSIONS_API_AGENT_AGENT_API_H_
