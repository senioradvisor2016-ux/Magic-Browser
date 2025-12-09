// Copyright 2025 Atlas Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license.

#include "atlas_browser/features/llm_agent/browser/prompts.h"

#include "base/strings/string_util.h"

namespace atlas {
namespace prompts {

std::string FormatPrompt(const std::string& prompt_template,
                          const std::map<std::string, std::string>& variables) {
  std::string result = prompt_template;
  
  for (const auto& [key, value] : variables) {
    std::string placeholder = "${" + key + "}";
    base::ReplaceSubstringsAfterOffset(&result, 0, placeholder, value);
  }
  
  return result;
}

std::string BuildSystemPrompt(bool enable_vision) {
  std::string prompt = kAgentSystemPrompt;
  
  if (enable_vision) {
    prompt += kAgentVisionAddition;
  }
  
  return prompt;
}

std::string BuildTaskStartPrompt(const std::string& task,
                                  const std::string& page_url,
                                  const std::string& page_state) {
  std::map<std::string, std::string> vars = {
      {"TASK", task},
      {"PAGE_URL", page_url},
  };
  
  std::string prompt = FormatPrompt(kTaskStartPrompt, vars);
  
  // Add page state
  prompt += "\n\n## Page DOM\n" + page_state;
  
  return prompt;
}

std::string BuildStepContinuePrompt(int step_number,
                                     const std::string& previous_thought,
                                     const std::string& action,
                                     const std::string& result,
                                     const std::string& page_state) {
  std::map<std::string, std::string> vars = {
      {"STEP_NUMBER", std::to_string(step_number)},
      {"PREVIOUS_THOUGHT", previous_thought},
      {"ACTION", action},
      {"RESULT", result},
      {"PAGE_STATE", page_state},
  };
  
  return FormatPrompt(kStepContinuePrompt, vars);
}

std::string BuildErrorPrompt(const std::string& error_type,
                              const std::string& details) {
  if (error_type == "element_not_found") {
    return FormatPrompt(kElementNotFoundPrompt, {{"SELECTOR", details}});
  } else if (error_type == "action_failed") {
    // Parse details as "action|error"
    size_t sep = details.find('|');
    std::string action = sep != std::string::npos ? details.substr(0, sep) : details;
    std::string error = sep != std::string::npos ? details.substr(sep + 1) : "";
    return FormatPrompt(kActionFailedPrompt, {
        {"ACTION", action},
        {"ERROR", error},
    });
  }
  
  return "Error: " + details;
}

std::string BuildQueryPrompt(const std::string& question,
                              const std::string& page_content) {
  std::map<std::string, std::string> vars = {
      {"QUESTION", question},
      {"PAGE_CONTENT", page_content},
  };
  
  return FormatPrompt(kQueryPagePrompt, vars);
}

}  // namespace prompts
}  // namespace atlas
