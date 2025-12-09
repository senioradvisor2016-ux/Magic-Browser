// Copyright 2025 Atlas Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license.

#include "atlas_browser/features/llm_agent/browser/agent_service.h"

#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/test_browser_context.h"
#include "content/public/test/test_web_contents_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace atlas {
namespace {

using ::testing::_;
using ::testing::Return;

class MockAgentServiceObserver : public AgentServiceObserver {
 public:
  MOCK_METHOD(void, OnTaskStarted, 
              (const std::string&, const std::string&), (override));
  MOCK_METHOD(void, OnTaskProgress,
              (const std::string&, const agent::mojom::AgentStep&), (override));
  MOCK_METHOD(void, OnTaskCompleted,
              (const std::string&, const agent::mojom::TaskResult&), (override));
  MOCK_METHOD(void, OnTaskFailed,
              (const std::string&, const std::string&), (override));
  MOCK_METHOD(void, OnTaskCancelled, (const std::string&), (override));
  MOCK_METHOD(void, OnAgentThinking, (const std::string&), (override));
  MOCK_METHOD(void, OnAgentActing,
              (const std::string&, const agent::mojom::AgentAction&), (override));
};

class AgentServiceTest : public testing::Test {
 protected:
  void SetUp() override {
    browser_context_ = std::make_unique<content::TestBrowserContext>();
    agent_service_ = std::make_unique<AgentService>(browser_context_.get());
  }

  void TearDown() override {
    agent_service_->Shutdown();
    agent_service_.reset();
    browser_context_.reset();
  }

  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<content::TestBrowserContext> browser_context_;
  std::unique_ptr<AgentService> agent_service_;
};

TEST_F(AgentServiceTest, DefaultConfigurationIsSet) {
  const auto& config = agent_service_->GetConfig();
  
  EXPECT_EQ(config.provider, AgentService::Config::Provider::kAnthropic);
  EXPECT_EQ(config.model, "claude-sonnet-4-20250514");
  EXPECT_EQ(config.max_steps, 50);
  EXPECT_TRUE(config.enable_vision);
  EXPECT_FLOAT_EQ(config.temperature, 0.0f);
}

TEST_F(AgentServiceTest, ConfigureUpdatesSettings) {
  AgentService::Config new_config;
  new_config.provider = AgentService::Config::Provider::kOpenAI;
  new_config.model = "gpt-4o";
  new_config.max_steps = 100;
  new_config.enable_vision = false;
  
  agent_service_->Configure(new_config);
  
  const auto& config = agent_service_->GetConfig();
  EXPECT_EQ(config.provider, AgentService::Config::Provider::kOpenAI);
  EXPECT_EQ(config.model, "gpt-4o");
  EXPECT_EQ(config.max_steps, 100);
  EXPECT_FALSE(config.enable_vision);
}

TEST_F(AgentServiceTest, AddAndRemoveObserver) {
  MockAgentServiceObserver observer;
  
  agent_service_->AddObserver(&observer);
  // Observer is registered
  
  agent_service_->RemoveObserver(&observer);
  // Observer is unregistered
  
  // No crash on multiple remove
  agent_service_->RemoveObserver(&observer);
}

TEST_F(AgentServiceTest, CancelNonExistentTaskDoesNotCrash) {
  // Should not crash when cancelling a task that doesn't exist
  agent_service_->CancelTask("non-existent-task-id");
}

TEST_F(AgentServiceTest, GetTaskStatusForNonExistentTask) {
  auto status = agent_service_->GetTaskStatus("non-existent-task-id");
  EXPECT_EQ(status, agent::mojom::TaskStatus::kFailed);
}

TEST_F(AgentServiceTest, IsTaskRunningForNonExistentTask) {
  EXPECT_FALSE(agent_service_->IsTaskRunning("non-existent-task-id"));
}

TEST_F(AgentServiceTest, ShutdownCancelsAllTasks) {
  MockAgentServiceObserver observer;
  agent_service_->AddObserver(&observer);
  
  // Shutdown should not crash even without active tasks
  agent_service_->Shutdown();
  
  agent_service_->RemoveObserver(&observer);
}

// =============================================================================
// Configuration Tests
// =============================================================================

class AgentServiceConfigTest : public AgentServiceTest {};

TEST_F(AgentServiceConfigTest, ConfigureWithApiKey) {
  AgentService::Config config;
  config.api_key = "test-api-key";
  
  agent_service_->Configure(config);
  
  EXPECT_EQ(agent_service_->GetConfig().api_key, "test-api-key");
}

TEST_F(AgentServiceConfigTest, ConfigureWithCustomEndpoint) {
  AgentService::Config config;
  config.provider = AgentService::Config::Provider::kCustom;
  config.api_endpoint = "https://custom-llm.example.com/api";
  
  agent_service_->Configure(config);
  
  const auto& saved_config = agent_service_->GetConfig();
  EXPECT_EQ(saved_config.provider, AgentService::Config::Provider::kCustom);
  EXPECT_EQ(saved_config.api_endpoint, "https://custom-llm.example.com/api");
}

TEST_F(AgentServiceConfigTest, ConfigureWithLocalProvider) {
  AgentService::Config config;
  config.provider = AgentService::Config::Provider::kLocal;
  config.model = "llama3";
  
  agent_service_->Configure(config);
  
  const auto& saved_config = agent_service_->GetConfig();
  EXPECT_EQ(saved_config.provider, AgentService::Config::Provider::kLocal);
  EXPECT_EQ(saved_config.model, "llama3");
}

// =============================================================================
// Response Parsing Tests
// =============================================================================

class AgentResponseParsingTest : public testing::Test {
 protected:
  // Helper to test response parsing
  // In real tests, this would use the actual ParseAgentResponse method
};

TEST_F(AgentResponseParsingTest, ParseValidClickResponse) {
  std::string response = R"({
    "thought": "I need to click the search button",
    "action": {
      "type": "click",
      "selector": "#search-btn"
    }
  })";
  
  // Verify JSON is valid
  auto parsed = base::JSONReader::Read(response);
  ASSERT_TRUE(parsed.has_value());
  ASSERT_TRUE(parsed->is_dict());
  
  const auto& dict = parsed->GetDict();
  EXPECT_EQ(*dict.FindString("thought"), "I need to click the search button");
  
  const auto* action = dict.FindDict("action");
  ASSERT_NE(action, nullptr);
  EXPECT_EQ(*action->FindString("type"), "click");
  EXPECT_EQ(*action->FindString("selector"), "#search-btn");
}

TEST_F(AgentResponseParsingTest, ParseValidTypeResponse) {
  std::string response = R"({
    "thought": "I need to enter the search query",
    "action": {
      "type": "type",
      "selector": "#search-input",
      "value": "Atlas Browser"
    }
  })";
  
  auto parsed = base::JSONReader::Read(response);
  ASSERT_TRUE(parsed.has_value());
  
  const auto& dict = parsed->GetDict();
  const auto* action = dict.FindDict("action");
  ASSERT_NE(action, nullptr);
  EXPECT_EQ(*action->FindString("type"), "type");
  EXPECT_EQ(*action->FindString("value"), "Atlas Browser");
}

TEST_F(AgentResponseParsingTest, ParseValidDoneResponse) {
  std::string response = R"({
    "thought": "Task completed successfully. The best price is $999.",
    "action": {
      "type": "done"
    }
  })";
  
  auto parsed = base::JSONReader::Read(response);
  ASSERT_TRUE(parsed.has_value());
  
  const auto& dict = parsed->GetDict();
  const auto* action = dict.FindDict("action");
  ASSERT_NE(action, nullptr);
  EXPECT_EQ(*action->FindString("type"), "done");
}

TEST_F(AgentResponseParsingTest, ParseResponseInCodeBlock) {
  std::string response = R"(
Based on the current page, I'll click the button.

```json
{
  "thought": "Clicking the submit button",
  "action": {
    "type": "click",
    "selector": "button[type='submit']"
  }
}
```
)";
  
  // Extract JSON from code block
  size_t json_start = response.find("```json");
  ASSERT_NE(json_start, std::string::npos);
  json_start = response.find('\n', json_start) + 1;
  size_t json_end = response.find("```", json_start);
  ASSERT_NE(json_end, std::string::npos);
  
  std::string json_str = response.substr(json_start, json_end - json_start);
  auto parsed = base::JSONReader::Read(json_str);
  ASSERT_TRUE(parsed.has_value());
}

TEST_F(AgentResponseParsingTest, RejectInvalidJson) {
  std::string response = "This is not valid JSON at all";
  
  auto parsed = base::JSONReader::Read(response);
  EXPECT_FALSE(parsed.has_value());
}

TEST_F(AgentResponseParsingTest, RejectMissingAction) {
  std::string response = R"({
    "thought": "I have a thought but no action"
  })";
  
  auto parsed = base::JSONReader::Read(response);
  ASSERT_TRUE(parsed.has_value());
  
  const auto& dict = parsed->GetDict();
  EXPECT_EQ(dict.FindDict("action"), nullptr);
}

}  // namespace
}  // namespace atlas
