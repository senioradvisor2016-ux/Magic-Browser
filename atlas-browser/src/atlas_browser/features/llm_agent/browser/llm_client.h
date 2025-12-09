// Copyright 2025 Atlas Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license.

#ifndef ATLAS_BROWSER_FEATURES_LLM_AGENT_BROWSER_LLM_CLIENT_H_
#define ATLAS_BROWSER_FEATURES_LLM_AGENT_BROWSER_LLM_CLIENT_H_

#include <memory>
#include <string>
#include <queue>

#include "base/callback.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "services/network/public/cpp/simple_url_loader.h"

namespace network {
class SharedURLLoaderFactory;
}

namespace atlas {

// LLMClient handles communication with LLM providers (Anthropic, OpenAI, local).
// It supports both text-only and vision (multimodal) requests.
class LLMClient {
 public:
  enum class Provider {
    kLocal,      // Local models (Ollama, llama.cpp)
    kOpenAI,     // OpenAI API
    kAnthropic,  // Anthropic Claude API
    kCustom,     // Custom endpoint
  };

  struct Config {
    Provider provider = Provider::kAnthropic;
    std::string model = "claude-sonnet-4-20250514";
    std::string api_key;
    std::string endpoint;  // Custom endpoint URL
    
    bool enable_vision = true;
    int max_tokens = 4096;
    float temperature = 0.0f;  // Deterministic for agents
    
    // Rate limiting
    int max_requests_per_minute = 60;
    int retry_count = 3;
    int retry_delay_ms = 1000;
  };

  LLMClient();
  ~LLMClient();

  LLMClient(const LLMClient&) = delete;
  LLMClient& operator=(const LLMClient&) = delete;

  // Configure the client
  void Configure(const Config& config);

  // Callback for completion results
  using CompletionCallback = base::OnceCallback<void(const std::string& result)>;

  // Get a completion from the LLM
  // If image_base64 is provided and vision is enabled, sends a multimodal request
  void GetCompletion(const std::string& prompt,
                     const std::string& image_base64,
                     CompletionCallback callback);

  // Get a completion without an image
  void GetCompletion(const std::string& prompt, CompletionCallback callback);

  // Check if the client is configured and ready
  bool IsReady() const;

  // Get current configuration
  const Config& GetConfig() const { return config_; }

 private:
  // Provider-specific request methods
  void SendAnthropicRequest(const std::string& prompt,
                            const std::string& image_base64,
                            CompletionCallback callback);

  void SendOpenAIRequest(const std::string& prompt,
                         const std::string& image_base64,
                         CompletionCallback callback);

  void SendLocalRequest(const std::string& prompt,
                        const std::string& image_base64,
                        CompletionCallback callback);

  void SendCustomRequest(const std::string& prompt,
                         const std::string& image_base64,
                         CompletionCallback callback);

  // Response handlers
  void OnAnthropicResponse(CompletionCallback callback,
                           std::unique_ptr<std::string> response);

  void OnOpenAIResponse(CompletionCallback callback,
                        std::unique_ptr<std::string> response);

  void OnLocalResponse(CompletionCallback callback,
                       std::unique_ptr<std::string> response);

  // Helper to create URL loader
  std::unique_ptr<network::SimpleURLLoader> CreateURLLoader(
      const std::string& url,
      const std::string& method,
      const std::string& body,
      const std::map<std::string, std::string>& headers);

  // Get URL loader factory
  scoped_refptr<network::SharedURLLoaderFactory> GetURLLoaderFactory();

  Config config_;
  
  // Active URL loaders (one per request)
  std::vector<std::unique_ptr<network::SimpleURLLoader>> active_loaders_;

  // URL loader factory
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;

  base::WeakPtrFactory<LLMClient> weak_factory_{this};
};

}  // namespace atlas

#endif  // ATLAS_BROWSER_FEATURES_LLM_AGENT_BROWSER_LLM_CLIENT_H_
