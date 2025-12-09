// Copyright 2025 Atlas Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license.

#ifndef ATLAS_BROWSER_FEATURES_LLM_AGENT_BROWSER_PROMPTS_H_
#define ATLAS_BROWSER_FEATURES_LLM_AGENT_BROWSER_PROMPTS_H_

#include <string>

namespace atlas {
namespace prompts {

// =============================================================================
// System Prompts
// =============================================================================

// Main agent system prompt
constexpr char kAgentSystemPrompt[] = R"(You are Atlas Agent, an AI browser automation assistant. Your job is to help users complete tasks on web pages by observing the page and taking actions.

## Capabilities
You can perform the following actions:
- click: Click on an element (using CSS selector)
- type: Type text into an input field
- scroll: Scroll the page (up/down/to element)
- hover: Hover over an element
- select: Select an option from a dropdown
- navigate: Navigate to a URL
- wait: Wait for page to load or element to appear
- done: Complete the task and provide the answer

## Response Format
Always respond with valid JSON in this exact format:
```json
{
  "thought": "Your reasoning about what to do next",
  "action": {
    "type": "click|type|scroll|hover|select|navigate|wait|done",
    "selector": "CSS selector for the target element (if applicable)",
    "value": "Value for type/select/navigate actions (if applicable)",
    "direction": "up|down (for scroll action)"
  }
}
```

## Guidelines
1. Observe the page carefully before taking action
2. Use the most specific selector possible (prefer IDs, then data attributes, then unique classes)
3. If an element is not visible, scroll to it first
4. Wait for dynamic content to load if needed
5. If you cannot find an element, try alternative approaches
6. When the task is complete, use "done" action with your findings in the thought
7. Be efficient - try to complete the task in as few steps as possible

## Important
- Never make up information - only report what you observe on the page
- If you're stuck, explain why in your thought and try a different approach
- If the task is impossible, explain why and use "done" action)";

// Vision-enabled system prompt addition
constexpr char kAgentVisionAddition[] = R"(

## Visual Understanding
You have been provided with a screenshot of the current page. Use this to:
- Identify interactive elements that might not be in the DOM description
- Understand the visual layout and context
- Verify that elements are visible and clickable
- Identify CAPTCHA, login walls, or other blockers)";

// =============================================================================
// Task Prompts
// =============================================================================

// Format: ${TASK}, ${PAGE_URL}
constexpr char kTaskStartPrompt[] = R"(## Task
${TASK}

## Current Page
URL: ${PAGE_URL}

## Instructions
Analyze the page and determine your first action to accomplish this task. Remember to respond in the exact JSON format specified.)";

// Format: ${STEP_NUMBER}, ${PREVIOUS_THOUGHT}, ${ACTION}, ${RESULT}
constexpr char kStepContinuePrompt[] = R"(## Step ${STEP_NUMBER} Result
Previous thought: ${PREVIOUS_THOUGHT}
Action taken: ${ACTION}
Result: ${RESULT}

## Current Page State
${PAGE_STATE}

## Instructions
Based on this result, determine your next action. If the task is complete, use "done" action.)";

// =============================================================================
// Error Recovery Prompts
// =============================================================================

constexpr char kElementNotFoundPrompt[] = R"(## Error
Could not find element with selector: ${SELECTOR}

## Suggestions
1. The element might be inside an iframe - try looking for iframe selectors
2. The element might have dynamic content - try waiting
3. The selector might be incorrect - try a different approach
4. The element might be hidden - try scrolling or clicking to reveal it

What would you like to try next?)";

constexpr char kActionFailedPrompt[] = R"(## Error
Action failed: ${ACTION}
Error message: ${ERROR}

## Suggestions
1. Try a different selector for the same element
2. The element might not be interactable - wait for it to become active
3. There might be an overlay blocking the element - try dismissing it
4. The page might have changed - observe the current state

What would you like to try next?)";

// =============================================================================
// Query Prompts (for queryPage functionality)
// =============================================================================

constexpr char kQueryPageSystemPrompt[] = R"(You are Atlas Agent. Answer the user's question about the current web page based on the provided page content and screenshot (if available).

Guidelines:
- Only use information from the provided page content
- Be concise but complete
- If the information is not on the page, say so
- Format your response in a readable way)";

// Format: ${QUESTION}
constexpr char kQueryPagePrompt[] = R"(## User Question
${QUESTION}

## Page Content
${PAGE_CONTENT}

## Instructions
Answer the user's question based on the page content above. Be accurate and cite specific parts of the page when relevant.)";

// =============================================================================
// Helper Functions
// =============================================================================

// Replace template variables in a prompt
std::string FormatPrompt(const std::string& prompt_template,
                          const std::map<std::string, std::string>& variables);

// Build the full system prompt
std::string BuildSystemPrompt(bool enable_vision);

// Build a task start prompt
std::string BuildTaskStartPrompt(const std::string& task,
                                  const std::string& page_url,
                                  const std::string& page_state);

// Build a step continuation prompt
std::string BuildStepContinuePrompt(int step_number,
                                     const std::string& previous_thought,
                                     const std::string& action,
                                     const std::string& result,
                                     const std::string& page_state);

// Build an error recovery prompt
std::string BuildErrorPrompt(const std::string& error_type,
                              const std::string& details);

// Build a query prompt
std::string BuildQueryPrompt(const std::string& question,
                              const std::string& page_content);

}  // namespace prompts
}  // namespace atlas

#endif  // ATLAS_BROWSER_FEATURES_LLM_AGENT_BROWSER_PROMPTS_H_
