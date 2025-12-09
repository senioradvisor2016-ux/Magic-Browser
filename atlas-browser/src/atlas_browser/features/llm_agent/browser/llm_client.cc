// Copyright 2025 Atlas Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license.

#include "atlas_browser/features/llm_agent/browser/llm_client.h"

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"
#include "net/base/load_flags.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/mojom/url_response_head.mojom.h"

namespace atlas {

namespace {

constexpr char kAnthropicEndpoint[] = "https://api.anthropic.com/v1/messages";
constexpr char kOpenAIEndpoint[] = "https://api.openai.com/v1/chat/completions";
constexpr char kLocalEndpoint[] = "http://localhost:11434/api/generate";

constexpr int kMaxResponseSize = 10 * 1024 * 1024;  // 10 MB

constexpr net::NetworkTrafficAnnotationTag kTrafficAnnotation =
    net::DefineNetworkTrafficAnnotation("atlas_llm_client", R"(
        semantics {
          sender: "Atlas Browser Agent"
          description:
            "Sends prompts to LLM providers to get agent decisions for "
            "browser automation tasks."
          trigger:
            "User initiates an agent task in Atlas Browser."
          data:
            "Task description, page content, and optionally screenshots."
          destination: OTHER
          destination_other: "LLM API provider (Anthropic, OpenAI, or local)"
        }
        policy {
          cookies_allowed: NO
          setting:
            "Users can configure the LLM provider in Atlas settings."
          policy_exception_justification:
            "Essential for Atlas Browser agent functionality."
        })");

}  // namespace

LLMClient::LLMClient() = default;

LLMClient::~LLMClient() = default;

void LLMClient::Configure(const Config& config) {
  config_ = config;
  LOG(INFO) << "LLMClient configured: provider="
            << static_cast<int>(config.provider) << " model=" << config.model;
}

bool LLMClient::IsReady() const {
  switch (config_.provider) {
    case Provider::kAnthropic:
    case Provider::kOpenAI:
      return !config_.api_key.empty();
    case Provider::kLocal:
      return true;  // Local always "ready" (will fail if server not running)
    case Provider::kCustom:
      return !config_.endpoint.empty();
  }
  return false;
}

void LLMClient::GetCompletion(const std::string& prompt,
                              const std::string& image_base64,
                              CompletionCallback callback) {
  if (!IsReady()) {
    LOG(ERROR) << "LLMClient not configured";
    std::move(callback).Run("");
    return;
  }

  switch (config_.provider) {
    case Provider::kAnthropic:
      SendAnthropicRequest(prompt, image_base64, std::move(callback));
      break;
    case Provider::kOpenAI:
      SendOpenAIRequest(prompt, image_base64, std::move(callback));
      break;
    case Provider::kLocal:
      SendLocalRequest(prompt, image_base64, std::move(callback));
      break;
    case Provider::kCustom:
      SendCustomRequest(prompt, image_base64, std::move(callback));
      break;
  }
}

void LLMClient::GetCompletion(const std::string& prompt,
                              CompletionCallback callback) {
  GetCompletion(prompt, "", std::move(callback));
}

// =============================================================================
// Anthropic API
// =============================================================================

void LLMClient::SendAnthropicRequest(const std::string& prompt,
                                     const std::string& image_base64,
                                     CompletionCallback callback) {
  // Build request body
  base::Value::Dict body;
  body.Set("model", config_.model);
  body.Set("max_tokens", config_.max_tokens);
  body.Set("temperature", static_cast<double>(config_.temperature));

  // Build messages array
  base::Value::List messages;
  base::Value::Dict message;
  message.Set("role", "user");

  // Build content (supports multimodal)
  base::Value::List content;

  // Add image if present and vision is enabled
  if (!image_base64.empty() && config_.enable_vision) {
    base::Value::Dict image_block;
    image_block.Set("type", "image");

    base::Value::Dict source;
    source.Set("type", "base64");
    source.Set("media_type", "image/png");
    source.Set("data", image_base64);
    image_block.Set("source", std::move(source));

    content.Append(std::move(image_block));
  }

  // Add text prompt
  base::Value::Dict text_block;
  text_block.Set("type", "text");
  text_block.Set("text", prompt);
  content.Append(std::move(text_block));

  message.Set("content", std::move(content));
  messages.Append(std::move(message));
  body.Set("messages", std::move(messages));

  // Serialize to JSON
  std::string body_json;
  base::JSONWriter::Write(body, &body_json);

  // Set headers
  std::map<std::string, std::string> headers;
  headers["x-api-key"] = config_.api_key;
  headers["anthropic-version"] = "2023-06-01";
  headers["content-type"] = "application/json";

  // Create and send request
  auto loader = CreateURLLoader(kAnthropicEndpoint, "POST", body_json, headers);
  auto* loader_ptr = loader.get();
  active_loaders_.push_back(std::move(loader));

  loader_ptr->DownloadToString(
      GetURLLoaderFactory().get(),
      base::BindOnce(&LLMClient::OnAnthropicResponse, weak_factory_.GetWeakPtr(),
                     std::move(callback)),
      kMaxResponseSize);
}

void LLMClient::OnAnthropicResponse(CompletionCallback callback,
                                    std::unique_ptr<std::string> response) {
  if (!response || response->empty()) {
    LOG(ERROR) << "Empty response from Anthropic API";
    std::move(callback).Run("");
    return;
  }

  // Parse response JSON
  auto json = base::JSONReader::Read(*response);
  if (!json || !json->is_dict()) {
    LOG(ERROR) << "Invalid JSON from Anthropic API: " << *response;
    std::move(callback).Run("");
    return;
  }

  const auto& dict = json->GetDict();

  // Check for error
  if (const auto* error = dict.FindDict("error")) {
    const std::string* message = error->FindString("message");
    LOG(ERROR) << "Anthropic API error: " << (message ? *message : "unknown");
    std::move(callback).Run("");
    return;
  }

  // Extract content from response
  // Response format: { content: [{ type: "text", text: "..." }] }
  const auto* content = dict.FindList("content");
  if (!content || content->empty()) {
    LOG(ERROR) << "No content in Anthropic response";
    std::move(callback).Run("");
    return;
  }

  // Get text from first content block
  for (const auto& block : *content) {
    if (!block.is_dict())
      continue;
    const auto& block_dict = block.GetDict();
    const std::string* type = block_dict.FindString("type");
    if (type && *type == "text") {
      const std::string* text = block_dict.FindString("text");
      if (text) {
        std::move(callback).Run(*text);
        return;
      }
    }
  }

  LOG(ERROR) << "No text content in Anthropic response";
  std::move(callback).Run("");
}

// =============================================================================
// OpenAI API
// =============================================================================

void LLMClient::SendOpenAIRequest(const std::string& prompt,
                                  const std::string& image_base64,
                                  CompletionCallback callback) {
  // Build request body
  base::Value::Dict body;
  body.Set("model", config_.model);
  body.Set("max_tokens", config_.max_tokens);
  body.Set("temperature", static_cast<double>(config_.temperature));

  // Build messages array
  base::Value::List messages;
  base::Value::Dict message;
  message.Set("role", "user");

  // Build content (supports multimodal for GPT-4V)
  if (!image_base64.empty() && config_.enable_vision) {
    base::Value::List content;

    // Add text
    base::Value::Dict text_part;
    text_part.Set("type", "text");
    text_part.Set("text", prompt);
    content.Append(std::move(text_part));

    // Add image
    base::Value::Dict image_part;
    image_part.Set("type", "image_url");

    base::Value::Dict image_url;
    image_url.Set("url", "data:image/png;base64," + image_base64);
    image_part.Set("image_url", std::move(image_url));
    content.Append(std::move(image_part));

    message.Set("content", std::move(content));
  } else {
    // Text-only
    message.Set("content", prompt);
  }

  messages.Append(std::move(message));
  body.Set("messages", std::move(messages));

  // Serialize to JSON
  std::string body_json;
  base::JSONWriter::Write(body, &body_json);

  // Set headers
  std::map<std::string, std::string> headers;
  headers["Authorization"] = "Bearer " + config_.api_key;
  headers["Content-Type"] = "application/json";

  // Create and send request
  auto loader = CreateURLLoader(kOpenAIEndpoint, "POST", body_json, headers);
  auto* loader_ptr = loader.get();
  active_loaders_.push_back(std::move(loader));

  loader_ptr->DownloadToString(
      GetURLLoaderFactory().get(),
      base::BindOnce(&LLMClient::OnOpenAIResponse, weak_factory_.GetWeakPtr(),
                     std::move(callback)),
      kMaxResponseSize);
}

void LLMClient::OnOpenAIResponse(CompletionCallback callback,
                                 std::unique_ptr<std::string> response) {
  if (!response || response->empty()) {
    LOG(ERROR) << "Empty response from OpenAI API";
    std::move(callback).Run("");
    return;
  }

  // Parse response JSON
  auto json = base::JSONReader::Read(*response);
  if (!json || !json->is_dict()) {
    LOG(ERROR) << "Invalid JSON from OpenAI API";
    std::move(callback).Run("");
    return;
  }

  const auto& dict = json->GetDict();

  // Check for error
  if (const auto* error = dict.FindDict("error")) {
    const std::string* message = error->FindString("message");
    LOG(ERROR) << "OpenAI API error: " << (message ? *message : "unknown");
    std::move(callback).Run("");
    return;
  }

  // Extract content from response
  // Response format: { choices: [{ message: { content: "..." } }] }
  const auto* choices = dict.FindList("choices");
  if (!choices || choices->empty()) {
    LOG(ERROR) << "No choices in OpenAI response";
    std::move(callback).Run("");
    return;
  }

  const auto& first_choice = (*choices)[0];
  if (!first_choice.is_dict()) {
    std::move(callback).Run("");
    return;
  }

  const auto* message_dict = first_choice.GetDict().FindDict("message");
  if (!message_dict) {
    std::move(callback).Run("");
    return;
  }

  const std::string* content = message_dict->FindString("content");
  if (content) {
    std::move(callback).Run(*content);
  } else {
    std::move(callback).Run("");
  }
}

// =============================================================================
// Local LLM (Ollama)
// =============================================================================

void LLMClient::SendLocalRequest(const std::string& prompt,
                                 const std::string& image_base64,
                                 CompletionCallback callback) {
  // Build request body for Ollama API
  base::Value::Dict body;
  body.Set("model", config_.model.empty() ? "llama2" : config_.model);
  body.Set("prompt", prompt);
  body.Set("stream", false);

  // Ollama supports images for multimodal models (llava, etc.)
  if (!image_base64.empty() && config_.enable_vision) {
    base::Value::List images;
    images.Append(image_base64);
    body.Set("images", std::move(images));
  }

  // Options
  base::Value::Dict options;
  options.Set("temperature", static_cast<double>(config_.temperature));
  options.Set("num_predict", config_.max_tokens);
  body.Set("options", std::move(options));

  // Serialize to JSON
  std::string body_json;
  base::JSONWriter::Write(body, &body_json);

  // Set headers
  std::map<std::string, std::string> headers;
  headers["Content-Type"] = "application/json";

  std::string endpoint =
      config_.endpoint.empty() ? kLocalEndpoint : config_.endpoint;

  // Create and send request
  auto loader = CreateURLLoader(endpoint, "POST", body_json, headers);
  auto* loader_ptr = loader.get();
  active_loaders_.push_back(std::move(loader));

  loader_ptr->DownloadToString(
      GetURLLoaderFactory().get(),
      base::BindOnce(&LLMClient::OnLocalResponse, weak_factory_.GetWeakPtr(),
                     std::move(callback)),
      kMaxResponseSize);
}

void LLMClient::OnLocalResponse(CompletionCallback callback,
                                std::unique_ptr<std::string> response) {
  if (!response || response->empty()) {
    LOG(ERROR) << "Empty response from local LLM";
    std::move(callback).Run("");
    return;
  }

  // Parse response JSON
  auto json = base::JSONReader::Read(*response);
  if (!json || !json->is_dict()) {
    LOG(ERROR) << "Invalid JSON from local LLM: " << *response;
    std::move(callback).Run("");
    return;
  }

  const auto& dict = json->GetDict();

  // Check for error
  if (const std::string* error = dict.FindString("error")) {
    LOG(ERROR) << "Local LLM error: " << *error;
    std::move(callback).Run("");
    return;
  }

  // Extract response text
  // Ollama format: { response: "..." }
  const std::string* text = dict.FindString("response");
  if (text) {
    std::move(callback).Run(*text);
  } else {
    std::move(callback).Run("");
  }
}

// =============================================================================
// Custom Endpoint
// =============================================================================

void LLMClient::SendCustomRequest(const std::string& prompt,
                                  const std::string& image_base64,
                                  CompletionCallback callback) {
  // Custom endpoint uses OpenAI-compatible format by default
  SendOpenAIRequest(prompt, image_base64, std::move(callback));
}

// =============================================================================
// Helpers
// =============================================================================

std::unique_ptr<network::SimpleURLLoader> LLMClient::CreateURLLoader(
    const std::string& url,
    const std::string& method,
    const std::string& body,
    const std::map<std::string, std::string>& headers) {
  auto request = std::make_unique<network::ResourceRequest>();
  request->url = GURL(url);
  request->method = method;
  request->load_flags = net::LOAD_DISABLE_CACHE;

  for (const auto& [key, value] : headers) {
    request->headers.SetHeader(key, value);
  }

  auto loader =
      network::SimpleURLLoader::Create(std::move(request), kTrafficAnnotation);

  if (!body.empty()) {
    loader->AttachStringForUpload(body, "application/json");
  }

  // Set timeout
  loader->SetTimeoutDuration(base::Seconds(60));

  return loader;
}

scoped_refptr<network::SharedURLLoaderFactory>
LLMClient::GetURLLoaderFactory() {
  // In a real implementation, this would get the URL loader factory from
  // the browser context's storage partition.
  // For now, return the cached factory or nullptr.
  return url_loader_factory_;
}

}  // namespace atlas
