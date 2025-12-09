// Copyright 2025 Atlas Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license.

#include "atlas_browser/features/llm_agent/browser/llm_client.h"

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace atlas {
namespace {

class LLMClientTest : public testing::Test {
 protected:
  void SetUp() override {
    client_ = std::make_unique<LLMClient>(nullptr);
  }

  std::unique_ptr<LLMClient> client_;
};

// =============================================================================
// Configuration Tests
// =============================================================================

TEST_F(LLMClientTest, DefaultConfiguration) {
  LLMClient::Config config;
  
  EXPECT_EQ(config.provider, LLMClient::Provider::kAnthropic);
  EXPECT_EQ(config.model, "claude-sonnet-4-20250514");
  EXPECT_TRUE(config.api_key.empty());
  EXPECT_EQ(config.max_tokens, 4096);
  EXPECT_FLOAT_EQ(config.temperature, 0.0f);
}

TEST_F(LLMClientTest, ConfigureAnthropicProvider) {
  LLMClient::Config config;
  config.provider = LLMClient::Provider::kAnthropic;
  config.api_key = "test-key";
  config.model = "claude-sonnet-4-20250514";
  
  client_->Configure(config);
  
  EXPECT_EQ(client_->GetProvider(), LLMClient::Provider::kAnthropic);
}

TEST_F(LLMClientTest, ConfigureOpenAIProvider) {
  LLMClient::Config config;
  config.provider = LLMClient::Provider::kOpenAI;
  config.api_key = "sk-test";
  config.model = "gpt-4o";
  
  client_->Configure(config);
  
  EXPECT_EQ(client_->GetProvider(), LLMClient::Provider::kOpenAI);
}

TEST_F(LLMClientTest, ConfigureLocalProvider) {
  LLMClient::Config config;
  config.provider = LLMClient::Provider::kLocal;
  config.api_endpoint = "http://localhost:11434";
  config.model = "llama3";
  
  client_->Configure(config);
  
  EXPECT_EQ(client_->GetProvider(), LLMClient::Provider::kLocal);
}

// =============================================================================
// API Key Validation Tests
// =============================================================================

TEST_F(LLMClientTest, ValidateAnthropicApiKey) {
  // Valid Anthropic key format
  EXPECT_TRUE(LLMClient::ValidateApiKey(
      LLMClient::Provider::kAnthropic, "sk-ant-api03-xxxxx"));
  
  // Invalid format
  EXPECT_FALSE(LLMClient::ValidateApiKey(
      LLMClient::Provider::kAnthropic, "invalid"));
}

TEST_F(LLMClientTest, ValidateOpenAIApiKey) {
  // Valid OpenAI key format
  EXPECT_TRUE(LLMClient::ValidateApiKey(
      LLMClient::Provider::kOpenAI, "sk-proj-xxxxx"));
  EXPECT_TRUE(LLMClient::ValidateApiKey(
      LLMClient::Provider::kOpenAI, "sk-xxxxx"));
  
  // Invalid format
  EXPECT_FALSE(LLMClient::ValidateApiKey(
      LLMClient::Provider::kOpenAI, "invalid"));
}

TEST_F(LLMClientTest, ValidateLocalNoKeyRequired) {
  // Local provider doesn't require API key
  EXPECT_TRUE(LLMClient::ValidateApiKey(
      LLMClient::Provider::kLocal, ""));
}

// =============================================================================
// Request Building Tests
// =============================================================================

class RequestBuildingTest : public testing::Test {
 protected:
  // Helper to build Anthropic request body
  std::string BuildAnthropicRequest(const std::string& system,
                                     const std::string& user,
                                     const std::string& model = "claude-sonnet-4-20250514",
                                     int max_tokens = 4096) {
    base::Value::Dict request;
    request.Set("model", model);
    request.Set("max_tokens", max_tokens);
    request.Set("system", system);
    
    base::Value::List messages;
    base::Value::Dict user_msg;
    user_msg.Set("role", "user");
    user_msg.Set("content", user);
    messages.Append(std::move(user_msg));
    
    request.Set("messages", std::move(messages));
    
    std::string json;
    base::JSONWriter::Write(request, &json);
    return json;
  }

  // Helper to build OpenAI request body
  std::string BuildOpenAIRequest(const std::string& system,
                                  const std::string& user,
                                  const std::string& model = "gpt-4o",
                                  int max_tokens = 4096) {
    base::Value::Dict request;
    request.Set("model", model);
    request.Set("max_tokens", max_tokens);
    
    base::Value::List messages;
    
    base::Value::Dict system_msg;
    system_msg.Set("role", "system");
    system_msg.Set("content", system);
    messages.Append(std::move(system_msg));
    
    base::Value::Dict user_msg;
    user_msg.Set("role", "user");
    user_msg.Set("content", user);
    messages.Append(std::move(user_msg));
    
    request.Set("messages", std::move(messages));
    
    std::string json;
    base::JSONWriter::Write(request, &json);
    return json;
  }
};

TEST_F(RequestBuildingTest, BuildAnthropicRequestBasic) {
  std::string request = BuildAnthropicRequest(
      "You are a helpful assistant",
      "Hello, world!");
  
  auto parsed = base::JSONReader::Read(request);
  ASSERT_TRUE(parsed.has_value());
  ASSERT_TRUE(parsed->is_dict());
  
  const auto& dict = parsed->GetDict();
  EXPECT_EQ(*dict.FindString("model"), "claude-sonnet-4-20250514");
  EXPECT_EQ(dict.FindInt("max_tokens").value_or(0), 4096);
  EXPECT_EQ(*dict.FindString("system"), "You are a helpful assistant");
  
  const auto* messages = dict.FindList("messages");
  ASSERT_NE(messages, nullptr);
  ASSERT_EQ(messages->size(), 1u);
  
  const auto& user_msg = (*messages)[0].GetDict();
  EXPECT_EQ(*user_msg.FindString("role"), "user");
  EXPECT_EQ(*user_msg.FindString("content"), "Hello, world!");
}

TEST_F(RequestBuildingTest, BuildOpenAIRequestBasic) {
  std::string request = BuildOpenAIRequest(
      "You are a helpful assistant",
      "Hello, world!");
  
  auto parsed = base::JSONReader::Read(request);
  ASSERT_TRUE(parsed.has_value());
  ASSERT_TRUE(parsed->is_dict());
  
  const auto& dict = parsed->GetDict();
  EXPECT_EQ(*dict.FindString("model"), "gpt-4o");
  
  const auto* messages = dict.FindList("messages");
  ASSERT_NE(messages, nullptr);
  ASSERT_EQ(messages->size(), 2u);  // system + user
  
  const auto& system_msg = (*messages)[0].GetDict();
  EXPECT_EQ(*system_msg.FindString("role"), "system");
  
  const auto& user_msg = (*messages)[1].GetDict();
  EXPECT_EQ(*user_msg.FindString("role"), "user");
}

// =============================================================================
// Response Parsing Tests
// =============================================================================

class ResponseParsingTest : public testing::Test {
 protected:
  // Helper to extract response from Anthropic format
  std::string ParseAnthropicResponse(const std::string& response) {
    auto parsed = base::JSONReader::Read(response);
    if (!parsed || !parsed->is_dict()) return "";
    
    const auto& dict = parsed->GetDict();
    const auto* content = dict.FindList("content");
    if (!content || content->empty()) return "";
    
    const auto& first = (*content)[0].GetDict();
    const auto* text = first.FindString("text");
    return text ? *text : "";
  }

  // Helper to extract response from OpenAI format
  std::string ParseOpenAIResponse(const std::string& response) {
    auto parsed = base::JSONReader::Read(response);
    if (!parsed || !parsed->is_dict()) return "";
    
    const auto& dict = parsed->GetDict();
    const auto* choices = dict.FindList("choices");
    if (!choices || choices->empty()) return "";
    
    const auto& choice = (*choices)[0].GetDict();
    const auto* message = choice.FindDict("message");
    if (!message) return "";
    
    const auto* content = message->FindString("content");
    return content ? *content : "";
  }
};

TEST_F(ResponseParsingTest, ParseAnthropicSuccessResponse) {
  std::string response = R"({
    "id": "msg_123",
    "type": "message",
    "role": "assistant",
    "content": [
      {
        "type": "text",
        "text": "Hello! How can I help you?"
      }
    ],
    "model": "claude-sonnet-4-20250514",
    "stop_reason": "end_turn"
  })";
  
  std::string content = ParseAnthropicResponse(response);
  EXPECT_EQ(content, "Hello! How can I help you?");
}

TEST_F(ResponseParsingTest, ParseOpenAISuccessResponse) {
  std::string response = R"({
    "id": "chatcmpl-123",
    "object": "chat.completion",
    "created": 1234567890,
    "model": "gpt-4o",
    "choices": [
      {
        "index": 0,
        "message": {
          "role": "assistant",
          "content": "Hello! How can I help you?"
        },
        "finish_reason": "stop"
      }
    ]
  })";
  
  std::string content = ParseOpenAIResponse(response);
  EXPECT_EQ(content, "Hello! How can I help you?");
}

TEST_F(ResponseParsingTest, ParseEmptyResponse) {
  EXPECT_EQ(ParseAnthropicResponse(""), "");
  EXPECT_EQ(ParseOpenAIResponse(""), "");
}

TEST_F(ResponseParsingTest, ParseInvalidJson) {
  EXPECT_EQ(ParseAnthropicResponse("not json"), "");
  EXPECT_EQ(ParseOpenAIResponse("not json"), "");
}

// =============================================================================
// Vision/Multimodal Tests
// =============================================================================

class VisionRequestTest : public testing::Test {
 protected:
  // Helper to build multimodal Anthropic request
  base::Value::Dict BuildAnthropicVisionMessage(
      const std::string& text,
      const std::string& image_base64) {
    base::Value::Dict message;
    message.Set("role", "user");
    
    base::Value::List content;
    
    // Image part
    base::Value::Dict image_part;
    image_part.Set("type", "image");
    base::Value::Dict source;
    source.Set("type", "base64");
    source.Set("media_type", "image/png");
    source.Set("data", image_base64);
    image_part.Set("source", std::move(source));
    content.Append(std::move(image_part));
    
    // Text part
    base::Value::Dict text_part;
    text_part.Set("type", "text");
    text_part.Set("text", text);
    content.Append(std::move(text_part));
    
    message.Set("content", std::move(content));
    return message;
  }
};

TEST_F(VisionRequestTest, BuildMultimodalMessage) {
  auto message = BuildAnthropicVisionMessage(
      "What's in this screenshot?",
      "base64encodedimage");
  
  EXPECT_EQ(*message.FindString("role"), "user");
  
  const auto* content = message.FindList("content");
  ASSERT_NE(content, nullptr);
  ASSERT_EQ(content->size(), 2u);
  
  // First is image
  const auto& image = (*content)[0].GetDict();
  EXPECT_EQ(*image.FindString("type"), "image");
  
  // Second is text
  const auto& text = (*content)[1].GetDict();
  EXPECT_EQ(*text.FindString("type"), "text");
  EXPECT_EQ(*text.FindString("text"), "What's in this screenshot?");
}

}  // namespace
}  // namespace atlas
