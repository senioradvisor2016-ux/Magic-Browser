# 🌐 Lager 3: Full Custom Browser - Komplett Guide

> Bygga en Chromium-fork med LLM-agentläge på Brave/Arc/Strawberry-nivå

---

## 📋 Innehåll

1. [Översikt & Arkitektur](#översikt--arkitektur)
2. [Systemkrav & Setup](#systemkrav--setup)
3. [Hämta Chromium](#hämta-chromium)
4. [Branding & Konfiguration](#branding--konfiguration)
5. [LLM Agent-integration](#llm-agent-integration)
6. [Custom UI](#custom-ui)
7. [Build & Distribution](#build--distribution)
8. [Underhåll & Uppdateringar](#underhåll--uppdateringar)

---

## 🏗️ Översikt & Arkitektur

### Varför Chromium-fork?

| Browser | Bas | Typ |
|---------|-----|-----|
| **Brave** | Chromium | Full fork, privacy-fokus |
| **Vivaldi** | Chromium | Fork + React UI |
| **Arc** | Chromium | Fork + Swift UI (macOS) |
| **Strawberry** | Chromium | Fork + AI Agent |
| **Edge** | Chromium | Microsoft's fork |

### Arkitektur-översikt

```
┌─────────────────────────────────────────────────────────────────────┐
│                        ATLAS BROWSER                                │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│  ┌──────────────────────────────────────────────────────────────┐  │
│  │                    BROWSER PROCESS                            │  │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐   │  │
│  │  │   UI Host   │  │   Agent     │  │   Extension Host    │   │  │
│  │  │  (Views)    │  │  Service    │  │                     │   │  │
│  │  └──────┬──────┘  └──────┬──────┘  └──────────┬──────────┘   │  │
│  │         │                │                     │              │  │
│  │         └────────────────┼─────────────────────┘              │  │
│  │                          │                                    │  │
│  │                    ┌─────▼─────┐                              │  │
│  │                    │   Mojo    │  ← IPC                       │  │
│  │                    │   Bus     │                              │  │
│  │                    └─────┬─────┘                              │  │
│  └──────────────────────────┼───────────────────────────────────┘  │
│                             │                                       │
│  ┌──────────────────────────▼───────────────────────────────────┐  │
│  │                   RENDERER PROCESS (per tab)                  │  │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐   │  │
│  │  │   Blink     │  │   V8        │  │   Agent Injector    │   │  │
│  │  │  (Layout)   │  │   (JS)      │  │   (DOM Access)      │   │  │
│  │  └─────────────┘  └─────────────┘  └─────────────────────┘   │  │
│  └──────────────────────────────────────────────────────────────┘  │
│                                                                     │
│  ┌──────────────────────────────────────────────────────────────┐  │
│  │                     LLM SERVICE                               │  │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐   │  │
│  │  │   Local     │  │   API       │  │   Vision Model      │   │  │
│  │  │   (Ollama)  │  │   (Claude)  │  │   (Screenshots)     │   │  │
│  │  └─────────────┘  └─────────────┘  └─────────────────────┘   │  │
│  └──────────────────────────────────────────────────────────────┘  │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```

### Chromium Process-modell

```
┌─────────────────┐
│ Browser Process │ ← En per browser-instans
│  - UI           │
│  - Storage      │
│  - Network      │
└────────┬────────┘
         │
    ┌────┴────┬─────────────┬─────────────┐
    ▼         ▼             ▼             ▼
┌───────┐ ┌───────┐    ┌───────┐    ┌───────┐
│Render │ │Render │    │ GPU   │    │Network│
│Process│ │Process│    │Process│    │Service│
│(Tab 1)│ │(Tab 2)│    │       │    │       │
└───────┘ └───────┘    └───────┘    └───────┘
```

---

## 💻 Systemkrav & Setup

### Hårdvarukrav

| Komponent | Minimum | Rekommenderat |
|-----------|---------|---------------|
| **Disk** | 100 GB | 250 GB SSD |
| **RAM** | 16 GB | 64 GB |
| **CPU** | 8 kärnor | 32+ kärnor |
| **OS** | Ubuntu 20.04 | Ubuntu 22.04 |

### Installera byggverktyg

```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install -y \
    git python3 python3-pip \
    build-essential clang lld \
    libnss3-dev libgtk-3-dev \
    libglib2.0-dev libpango1.0-dev \
    libatk1.0-dev libcairo2-dev \
    libfreetype6-dev libfontconfig1-dev \
    libdbus-1-dev libxi-dev \
    libxrandr-dev libxss-dev \
    libpulse-dev libudev-dev \
    libdrm-dev mesa-common-dev \
    ninja-build

# Installera depot_tools
cd ~
git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
echo 'export PATH="$HOME/depot_tools:$PATH"' >> ~/.bashrc
source ~/.bashrc
```

### macOS Setup

```bash
# Xcode command line tools
xcode-select --install

# depot_tools
git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
export PATH="$PWD/depot_tools:$PATH"
```

---

## 📥 Hämta Chromium

### Initial checkout (~30 GB, 1-3 timmar)

```bash
mkdir ~/chromium && cd ~/chromium
fetch --nohooks --no-history chromium

# Eller med full historik (större, men bättre för utveckling)
fetch --nohooks chromium

cd src

# Synka beroenden
gclient runhooks
```

### Viktiga mappar att förstå

```
chromium/src/
│
├── chrome/                      # 🎯 Chrome-specifik kod
│   ├── browser/                 # Browser-process
│   │   ├── ui/                 # Alla UI-komponenter
│   │   │   ├── views/          # Desktop UI (Windows/Linux)
│   │   │   ├── cocoa/          # macOS UI
│   │   │   └── webui/          # Web-baserad UI
│   │   ├── extensions/         # Extension system
│   │   ├── profiles/           # Användarprofiler
│   │   └── prefs/              # Inställningar
│   ├── renderer/               # Renderer-process
│   └── common/                 # Delad kod
│
├── content/                     # 🎯 Core browser engine
│   ├── browser/                # Browser-side content
│   ├── renderer/               # Renderer-side content
│   ├── public/                 # Public APIs
│   └── shell/                  # Minimal browser shell
│
├── third_party/
│   ├── blink/                  # 🎯 Rendering engine
│   │   ├── renderer/
│   │   │   ├── core/          # DOM, CSS, Layout
│   │   │   └── modules/       # Web APIs
│   │   └── public/
│   └── v8/                     # 🎯 JavaScript engine
│
├── components/                  # 🎯 Återanvändbara komponenter
│   ├── autofill/
│   ├── password_manager/
│   └── omnibox/
│
├── ui/                          # UI toolkit
│   ├── views/                  # Cross-platform views
│   ├── gfx/                    # Graphics primitives
│   └── base/                   # UI base classes
│
├── net/                         # Nätverk
├── base/                        # Bas-bibliotek
├── mojo/                        # IPC system
└── services/                    # Service layer
```

---

## 🎨 Branding & Konfiguration

### Steg 1: Skapa din browser-mapp

```bash
cd ~/chromium/src
mkdir -p atlas_browser/{browser,features,extensions,resources,branding}
```

### Steg 2: Branding-filer

```gn
# atlas_browser/branding/branding.gni

declare_args() {
  # Produktnamn
  atlas_product_name = "Atlas Browser"
  atlas_product_short_name = "Atlas"
  
  # Företag
  atlas_company_name = "Atlas Software AB"
  atlas_company_short_name = "Atlas"
  
  # Version
  atlas_version_major = 1
  atlas_version_minor = 0
  atlas_version_patch = 0
  
  # Identifiers
  atlas_app_id = "com.atlas.browser"
  
  # URLs
  atlas_homepage_url = "https://atlasbrowser.com"
  atlas_update_url = "https://updates.atlasbrowser.com"
}
```

### Steg 3: Huvudsaklig BUILD.gn

```gn
# atlas_browser/BUILD.gn

import("//build/config/chrome_build.gni")
import("//atlas_browser/branding/branding.gni")

group("atlas_browser") {
  deps = [
    ":atlas_browser_exe",
    "//atlas_browser/features:all_features",
  ]
}

executable("atlas_browser_exe") {
  output_name = "atlas"
  
  sources = [
    "browser/atlas_main.cc",
    "browser/atlas_browser_main_parts.cc",
    "browser/atlas_browser_main_parts.h",
    "browser/atlas_content_browser_client.cc",
    "browser/atlas_content_browser_client.h",
  ]
  
  deps = [
    "//base",
    "//chrome:chrome_initial",
    "//chrome/browser",
    "//content/public/app",
    "//content/public/browser",
    "//ui/views",
    
    # Custom features
    "//atlas_browser/features/llm_agent",
    "//atlas_browser/features/privacy",
  ]
  
  if (is_win) {
    sources += [ "browser/atlas_main_win.cc" ]
    deps += [ "//chrome/install_static:install_static_util" ]
  }
  
  if (is_mac) {
    sources += [ "browser/atlas_main_mac.mm" ]
    deps += [ "//chrome/browser/mac" ]
  }
}
```

### Steg 4: Browser Main Entry Point

```cpp
// atlas_browser/browser/atlas_main.cc

#include "chrome/app/chrome_main_delegate.h"
#include "content/public/app/content_main.h"
#include "atlas_browser/browser/atlas_main_delegate.h"

#if BUILDFLAG(IS_WIN)
int APIENTRY wWinMain(HINSTANCE instance, 
                      HINSTANCE prev_instance,
                      wchar_t* command_line, 
                      int show_command) {
  content::ContentMainParams params(
      new atlas::AtlasMainDelegate());
  params.instance = instance;
  return content::ContentMain(std::move(params));
}
#else
int main(int argc, const char** argv) {
  content::ContentMainParams params(
      new atlas::AtlasMainDelegate());
  params.argc = argc;
  params.argv = argv;
  return content::ContentMain(std::move(params));
}
#endif
```

### Steg 5: Main Delegate

```cpp
// atlas_browser/browser/atlas_main_delegate.h

#ifndef ATLAS_BROWSER_ATLAS_MAIN_DELEGATE_H_
#define ATLAS_BROWSER_ATLAS_MAIN_DELEGATE_H_

#include "chrome/app/chrome_main_delegate.h"

namespace atlas {

class AtlasMainDelegate : public ChromeMainDelegate {
 public:
  AtlasMainDelegate();
  ~AtlasMainDelegate() override;
  
  // ContentMainDelegate overrides
  bool BasicStartupComplete(int* exit_code) override;
  void PreSandboxStartup() override;
  content::ContentBrowserClient* CreateContentBrowserClient() override;
  content::ContentRendererClient* CreateContentRendererClient() override;

 private:
  std::unique_ptr<AtlasContentBrowserClient> browser_client_;
  std::unique_ptr<AtlasContentRendererClient> renderer_client_;
};

}  // namespace atlas

#endif  // ATLAS_BROWSER_ATLAS_MAIN_DELEGATE_H_
```

```cpp
// atlas_browser/browser/atlas_main_delegate.cc

#include "atlas_browser/browser/atlas_main_delegate.h"
#include "atlas_browser/browser/atlas_content_browser_client.h"
#include "atlas_browser/common/atlas_content_renderer_client.h"
#include "base/command_line.h"
#include "base/logging.h"

namespace atlas {

AtlasMainDelegate::AtlasMainDelegate() = default;
AtlasMainDelegate::~AtlasMainDelegate() = default;

bool AtlasMainDelegate::BasicStartupComplete(int* exit_code) {
  // Kör först Chrome's startup
  if (ChromeMainDelegate::BasicStartupComplete(exit_code)) {
    return true;
  }
  
  // Atlas-specifik initiering
  LOG(INFO) << "Atlas Browser starting up...";
  
  // Registrera Atlas-specifika features
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  
  // Aktivera våra features
  command_line->AppendSwitch("enable-atlas-agent");
  
  return false;  // Fortsätt startup
}

void AtlasMainDelegate::PreSandboxStartup() {
  ChromeMainDelegate::PreSandboxStartup();
  
  // Ladda Atlas-resurser
  InitializeAtlasResources();
}

content::ContentBrowserClient* 
AtlasMainDelegate::CreateContentBrowserClient() {
  browser_client_ = std::make_unique<AtlasContentBrowserClient>();
  return browser_client_.get();
}

content::ContentRendererClient* 
AtlasMainDelegate::CreateContentRendererClient() {
  renderer_client_ = std::make_unique<AtlasContentRendererClient>();
  return renderer_client_.get();
}

}  // namespace atlas
```

---

## 🤖 LLM Agent-integration

### Arkitektur

```
┌─────────────────────────────────────────────────────────────────┐
│                     AGENT SYSTEM                                │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  Browser Process                                                │
│  ┌───────────────────────────────────────────────────────────┐ │
│  │                  AgentService                              │ │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────────┐    │ │
│  │  │ TaskManager │  │ LLMClient   │  │ ActionExecutor  │    │ │
│  │  │             │  │             │  │                 │    │ │
│  │  │ - queue     │  │ - local     │  │ - click         │    │ │
│  │  │ - state     │  │ - api       │  │ - type          │    │ │
│  │  │ - history   │  │ - vision    │  │ - scroll        │    │ │
│  │  └─────────────┘  └─────────────┘  └─────────────────┘    │ │
│  └───────────────────────────────────────────────────────────┘ │
│                              │                                  │
│                         Mojo │ IPC                              │
│                              ▼                                  │
│  Renderer Process                                               │
│  ┌───────────────────────────────────────────────────────────┐ │
│  │                  AgentInjector                             │ │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────────┐    │ │
│  │  │DOMExtractor │  │ Highlighter │  │ EventSimulator  │    │ │
│  │  │             │  │             │  │                 │    │ │
│  │  │ - structure │  │ - overlay   │  │ - mouse         │    │ │
│  │  │ - elements  │  │ - labels    │  │ - keyboard      │    │ │
│  │  │ - a11y tree │  │             │  │ - focus         │    │ │
│  │  └─────────────┘  └─────────────┘  └─────────────────┘    │ │
│  └───────────────────────────────────────────────────────────┘ │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### BUILD.gn för Agent Feature

```gn
# atlas_browser/features/llm_agent/BUILD.gn

import("//mojo/public/tools/bindings/mojom.gni")

component("llm_agent") {
  sources = [
    # Browser-side
    "browser/agent_service.cc",
    "browser/agent_service.h",
    "browser/llm_client.cc",
    "browser/llm_client.h",
    "browser/action_executor.cc",
    "browser/action_executor.h",
    "browser/screenshot_capturer.cc",
    "browser/screenshot_capturer.h",
    "browser/task_manager.cc",
    "browser/task_manager.h",
    
    # Renderer-side
    "renderer/agent_injector.cc",
    "renderer/agent_injector.h",
    "renderer/dom_extractor.cc",
    "renderer/dom_extractor.h",
    "renderer/element_highlighter.cc",
    "renderer/element_highlighter.h",
    "renderer/event_simulator.cc",
    "renderer/event_simulator.h",
  ]
  
  deps = [
    ":agent_mojom",
    "//base",
    "//content/public/browser",
    "//content/public/renderer",
    "//mojo/public/cpp/bindings",
    "//services/network/public/cpp",
    "//third_party/blink/public:blink",
    "//ui/gfx",
    "//ui/gfx/codec",
  ]
  
  public_deps = [
    ":agent_mojom",
  ]
}

mojom("agent_mojom") {
  sources = [
    "public/mojom/agent.mojom",
  ]
  
  public_deps = [
    "//mojo/public/mojom/base",
    "//ui/gfx/geometry/mojom",
    "//url/mojom:url_mojom_gurl",
  ]
}
```

### Mojo Interface Definition

```mojom
// atlas_browser/features/llm_agent/public/mojom/agent.mojom

module atlas.agent.mojom;

import "mojo/public/mojom/base/string16.mojom";
import "ui/gfx/geometry/mojom/geometry.mojom";
import "url/mojom/url.mojom";

// ============================================
// Enums
// ============================================

enum ActionType {
  kClick,
  kType,
  kScroll,
  kHover,
  kPressKey,
  kNavigate,
  kWait,
  kScreenshot,
  kDone,
};

enum TaskStatus {
  kPending,
  kRunning,
  kCompleted,
  kFailed,
  kCancelled,
};

// ============================================
// Data Structures
// ============================================

struct ElementInfo {
  string unique_id;
  string tag_name;
  string? id;
  string? class_name;
  string? text_content;
  string? aria_label;
  string? placeholder;
  string? href;
  string? value;
  string css_selector;
  string xpath;
  gfx.mojom.Rect bounding_box;
  bool is_visible;
  bool is_clickable;
  bool is_editable;
  bool is_focusable;
  int32 tab_index;
  array<ElementInfo> children;
};

struct PageState {
  url.mojom.Url url;
  string title;
  bool is_loading;
  int32 scroll_x;
  int32 scroll_y;
  int32 viewport_width;
  int32 viewport_height;
  int32 document_width;
  int32 document_height;
};

struct AgentAction {
  ActionType type;
  string? selector;
  string? value;
  int32? x;
  int32? y;
  string? key;
  int32? delay_ms;
};

struct ActionResult {
  bool success;
  string? error_message;
  string? screenshot_base64;
  PageState? new_state;
};

struct AgentStep {
  int32 step_number;
  string thought;
  AgentAction action;
  ActionResult result;
  double timestamp;
};

struct TaskResult {
  TaskStatus status;
  string? final_answer;
  array<AgentStep> steps;
  double duration_seconds;
};

// ============================================
// Interfaces: Browser → Renderer
// ============================================

interface AgentRendererClient {
  // Hämta DOM-struktur för LLM
  GetAccessibleDOM() => (string dom_json, array<ElementInfo> elements);
  
  // Hämta endast interaktiva element
  GetInteractiveElements() => (array<ElementInfo> elements);
  
  // Hämta page state
  GetPageState() => (PageState state);
  
  // Utför action
  PerformAction(AgentAction action) => (ActionResult result);
  
  // Highlight element för debugging
  HighlightElement(string selector, bool show);
  
  // Highlight alla interaktiva element med labels
  ShowElementLabels(bool show);
  
  // Scroll till element
  ScrollToElement(string selector) => (bool success);
  
  // Simulera keyboard event
  SimulateKeyPress(string key, bool ctrl, bool shift, bool alt);
};

// ============================================
// Interfaces: Renderer → Browser
// ============================================

interface AgentBrowserHost {
  // Rapportera navigation
  OnNavigationStarted(url.mojom.Url url);
  OnNavigationCompleted(url.mojom.Url url, bool success);
  
  // Rapportera page updates
  OnPageStateChanged(PageState state);
  OnDOMChanged();
  
  // Begär LLM-beslut
  RequestAgentDecision(
    string task,
    string current_state_json,
    string? screenshot_base64
  ) => (AgentAction action);
  
  // Log agent action
  LogAgentStep(AgentStep step);
};
```

### Agent Service Implementation

```cpp
// atlas_browser/features/llm_agent/browser/agent_service.h

#ifndef ATLAS_BROWSER_FEATURES_LLM_AGENT_BROWSER_AGENT_SERVICE_H_
#define ATLAS_BROWSER_FEATURES_LLM_AGENT_BROWSER_AGENT_SERVICE_H_

#include <memory>
#include <queue>
#include <string>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "components/keyed_service/core/keyed_service.h"
#include "atlas_browser/features/llm_agent/public/mojom/agent.mojom.h"

namespace content {
class WebContents;
}

namespace atlas {

class LLMClient;
class TaskManager;
class ScreenshotCapturer;

// Observer för att lyssna på agent-events
class AgentServiceObserver : public base::CheckedObserver {
 public:
  virtual void OnTaskStarted(const std::string& task_id) {}
  virtual void OnTaskProgress(const std::string& task_id, 
                              const agent::mojom::AgentStep& step) {}
  virtual void OnTaskCompleted(const std::string& task_id,
                               const agent::mojom::TaskResult& result) {}
  virtual void OnTaskFailed(const std::string& task_id,
                            const std::string& error) {}
};

class AgentService : public KeyedService {
 public:
  // Singleton access
  static AgentService* GetForBrowserContext(
      content::BrowserContext* context);
  
  AgentService();
  ~AgentService() override;
  
  AgentService(const AgentService&) = delete;
  AgentService& operator=(const AgentService&) = delete;
  
  // Observer management
  void AddObserver(AgentServiceObserver* observer);
  void RemoveObserver(AgentServiceObserver* observer);
  
  // ============================================
  // Task Management
  // ============================================
  
  // Starta en ny agent-task
  using TaskCallback = base::OnceCallback<void(agent::mojom::TaskResult)>;
  std::string StartTask(content::WebContents* web_contents,
                        const std::string& task_description,
                        TaskCallback callback);
  
  // Pausa pågående task
  void PauseTask(const std::string& task_id);
  
  // Återuppta pausad task
  void ResumeTask(const std::string& task_id);
  
  // Avbryt task
  void CancelTask(const std::string& task_id);
  
  // Hämta task status
  agent::mojom::TaskStatus GetTaskStatus(const std::string& task_id) const;
  
  // ============================================
  // One-shot Actions
  // ============================================
  
  // Utför en enskild action
  using ActionCallback = base::OnceCallback<void(agent::mojom::ActionResult)>;
  void PerformAction(content::WebContents* web_contents,
                     const agent::mojom::AgentAction& action,
                     ActionCallback callback);
  
  // ============================================
  // DOM Queries
  // ============================================
  
  // Hämta DOM-struktur
  using DOMCallback = base::OnceCallback<void(
      const std::string& dom_json,
      std::vector<agent::mojom::ElementInfoPtr> elements)>;
  void GetDOM(content::WebContents* web_contents,
              DOMCallback callback);
  
  // Query med natural language
  using QueryCallback = base::OnceCallback<void(const std::string& answer)>;
  void QueryPage(content::WebContents* web_contents,
                 const std::string& question,
                 QueryCallback callback);
  
  // ============================================
  // Configuration
  // ============================================
  
  struct Config {
    enum class ModelProvider {
      kLocal,    // Ollama, llama.cpp
      kOpenAI,   // GPT-4
      kAnthropic,// Claude
      kCustom,   // Custom endpoint
    };
    
    ModelProvider provider = ModelProvider::kAnthropic;
    std::string model_name = "claude-sonnet-4-20250514";
    std::string api_key;
    std::string api_endpoint;
    
    int max_steps = 50;
    int step_timeout_ms = 30000;
    bool enable_vision = true;
    bool enable_element_labels = true;
    
    // Safety
    bool allow_form_submission = true;
    bool allow_payments = false;
    bool confirm_destructive_actions = true;
  };
  
  void Configure(const Config& config);
  const Config& GetConfig() const { return config_; }

 private:
  // Internal helpers
  void ExecuteAgentLoop(const std::string& task_id);
  void OnLLMResponse(const std::string& task_id,
                     const std::string& response);
  void OnActionComplete(const std::string& task_id,
                        agent::mojom::ActionResultPtr result);
  void OnScreenshotReady(const std::string& task_id,
                         const std::string& screenshot_base64);
  
  // Notify observers
  void NotifyTaskStarted(const std::string& task_id);
  void NotifyTaskProgress(const std::string& task_id,
                          const agent::mojom::AgentStep& step);
  void NotifyTaskCompleted(const std::string& task_id,
                           const agent::mojom::TaskResult& result);
  void NotifyTaskFailed(const std::string& task_id,
                        const std::string& error);
  
  Config config_;
  std::unique_ptr<LLMClient> llm_client_;
  std::unique_ptr<TaskManager> task_manager_;
  std::unique_ptr<ScreenshotCapturer> screenshot_capturer_;
  
  base::ObserverList<AgentServiceObserver> observers_;
  base::WeakPtrFactory<AgentService> weak_factory_{this};
};

}  // namespace atlas

#endif  // ATLAS_BROWSER_FEATURES_LLM_AGENT_BROWSER_AGENT_SERVICE_H_
```

```cpp
// atlas_browser/features/llm_agent/browser/agent_service.cc

#include "atlas_browser/features/llm_agent/browser/agent_service.h"

#include "base/guid.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/task/thread_pool.h"
#include "content/public/browser/web_contents.h"
#include "atlas_browser/features/llm_agent/browser/llm_client.h"
#include "atlas_browser/features/llm_agent/browser/screenshot_capturer.h"
#include "atlas_browser/features/llm_agent/browser/task_manager.h"

namespace atlas {

AgentService::AgentService()
    : llm_client_(std::make_unique<LLMClient>()),
      task_manager_(std::make_unique<TaskManager>()),
      screenshot_capturer_(std::make_unique<ScreenshotCapturer>()) {}

AgentService::~AgentService() = default;

void AgentService::Configure(const Config& config) {
  config_ = config;
  
  LLMClient::Config llm_config;
  llm_config.provider = static_cast<LLMClient::Provider>(config.provider);
  llm_config.model = config.model_name;
  llm_config.api_key = config.api_key;
  llm_config.endpoint = config.api_endpoint;
  llm_config.enable_vision = config.enable_vision;
  
  llm_client_->Configure(llm_config);
}

std::string AgentService::StartTask(
    content::WebContents* web_contents,
    const std::string& task_description,
    TaskCallback callback) {
  
  // Generera unikt task ID
  std::string task_id = base::GenerateGUID();
  
  // Skapa task
  TaskManager::Task task;
  task.id = task_id;
  task.description = task_description;
  task.web_contents = web_contents;
  task.callback = std::move(callback);
  task.status = agent::mojom::TaskStatus::kPending;
  task.start_time = base::Time::Now();
  
  task_manager_->AddTask(std::move(task));
  
  NotifyTaskStarted(task_id);
  
  // Starta agent loop
  ExecuteAgentLoop(task_id);
  
  return task_id;
}

void AgentService::ExecuteAgentLoop(const std::string& task_id) {
  auto* task = task_manager_->GetTask(task_id);
  if (!task || task->status == agent::mojom::TaskStatus::kCancelled) {
    return;
  }
  
  // Uppdatera status
  task->status = agent::mojom::TaskStatus::kRunning;
  
  // Kolla max steps
  if (task->steps.size() >= static_cast<size_t>(config_.max_steps)) {
    FinishTask(task_id, "Max steps reached");
    return;
  }
  
  // Hämta screenshot om vision är aktiverat
  if (config_.enable_vision) {
    screenshot_capturer_->Capture(
        task->web_contents,
        base::BindOnce(&AgentService::OnScreenshotReady,
                       weak_factory_.GetWeakPtr(),
                       task_id));
  } else {
    OnScreenshotReady(task_id, "");
  }
}

void AgentService::OnScreenshotReady(
    const std::string& task_id,
    const std::string& screenshot_base64) {
  
  auto* task = task_manager_->GetTask(task_id);
  if (!task) return;
  
  // Hämta DOM via Mojo
  GetDOM(task->web_contents,
         base::BindOnce(&AgentService::OnDOMReady,
                        weak_factory_.GetWeakPtr(),
                        task_id,
                        screenshot_base64));
}

void AgentService::OnDOMReady(
    const std::string& task_id,
    const std::string& screenshot_base64,
    const std::string& dom_json,
    std::vector<agent::mojom::ElementInfoPtr> elements) {
  
  auto* task = task_manager_->GetTask(task_id);
  if (!task) return;
  
  // Bygg prompt för LLM
  std::string prompt = BuildAgentPrompt(
      task->description,
      dom_json,
      task->steps);
  
  // Skicka till LLM
  llm_client_->GetCompletion(
      prompt,
      screenshot_base64,
      base::BindOnce(&AgentService::OnLLMResponse,
                     weak_factory_.GetWeakPtr(),
                     task_id));
}

void AgentService::OnLLMResponse(
    const std::string& task_id,
    const std::string& response) {
  
  auto* task = task_manager_->GetTask(task_id);
  if (!task) return;
  
  // Parsa LLM response
  auto parsed = ParseAgentResponse(response);
  if (!parsed) {
    NotifyTaskFailed(task_id, "Failed to parse LLM response");
    return;
  }
  
  // Skapa step
  agent::mojom::AgentStep step;
  step.step_number = task->steps.size() + 1;
  step.thought = parsed->thought;
  step.action = std::move(parsed->action);
  step.timestamp = base::Time::Now().ToDoubleT();
  
  // Kolla om vi är klara
  if (step.action.type == agent::mojom::ActionType::kDone) {
    task->steps.push_back(std::move(step));
    FinishTask(task_id, parsed->thought);
    return;
  }
  
  // Utför action
  PerformAction(
      task->web_contents,
      step.action,
      base::BindOnce(&AgentService::OnActionComplete,
                     weak_factory_.GetWeakPtr(),
                     task_id,
                     std::move(step)));
}

void AgentService::OnActionComplete(
    const std::string& task_id,
    agent::mojom::AgentStep step,
    agent::mojom::ActionResultPtr result) {
  
  auto* task = task_manager_->GetTask(task_id);
  if (!task) return;
  
  step.result = std::move(*result);
  task->steps.push_back(std::move(step));
  
  // Notifiera progress
  NotifyTaskProgress(task_id, task->steps.back());
  
  // Fortsätt loop
  if (step.result.success) {
    // Vänta kort innan nästa steg
    base::SequencedTaskRunner::GetCurrentDefault()->PostDelayedTask(
        FROM_HERE,
        base::BindOnce(&AgentService::ExecuteAgentLoop,
                       weak_factory_.GetWeakPtr(),
                       task_id),
        base::Milliseconds(500));
  } else {
    // Action misslyckades, låt LLM försöka igen
    ExecuteAgentLoop(task_id);
  }
}

std::string AgentService::BuildAgentPrompt(
    const std::string& task,
    const std::string& dom_json,
    const std::vector<agent::mojom::AgentStep>& history) {
  
  std::ostringstream prompt;
  
  prompt << R"(You are an AI browser agent. Complete the user's task by interacting with the webpage.

## Task
)" << task << R"(

## Current Page DOM (Interactive Elements)
)" << dom_json << R"(

## Previous Actions
)";

  for (const auto& step : history) {
    prompt << "Step " << step.step_number << ": "
           << "Thought: " << step.thought << "\n"
           << "Action: " << ActionToString(step.action) << "\n"
           << "Result: " << (step.result.success ? "Success" : "Failed") << "\n\n";
  }

  prompt << R"(
## Instructions
1. Analyze the current page state
2. Decide on the next action to progress toward the goal
3. Respond with JSON in this exact format:

```json
{
  "thought": "Your reasoning about what to do next",
  "action": {
    "type": "click|type|scroll|navigate|wait|done",
    "selector": "CSS selector for the target element",
    "value": "Text to type or URL to navigate to (if applicable)"
  }
}
```

Action types:
- "click": Click on element matching selector
- "type": Type value into element matching selector  
- "scroll": Scroll to element matching selector (or "up"/"down" for page scroll)
- "navigate": Go to URL in value field
- "wait": Wait for page to load (use after navigation)
- "done": Task is complete, thought contains the final answer

Important:
- Use specific, unique selectors
- If an action fails, try a different approach
- Always verify the result before marking as done
)";

  return prompt.str();
}

}  // namespace atlas
```

### LLM Client Implementation

```cpp
// atlas_browser/features/llm_agent/browser/llm_client.h

#ifndef ATLAS_BROWSER_FEATURES_LLM_AGENT_BROWSER_LLM_CLIENT_H_
#define ATLAS_BROWSER_FEATURES_LLM_AGENT_BROWSER_LLM_CLIENT_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "services/network/public/cpp/simple_url_loader.h"

namespace network {
class SharedURLLoaderFactory;
}

namespace atlas {

class LLMClient {
 public:
  enum class Provider {
    kLocal,
    kOpenAI,
    kAnthropic,
    kCustom,
  };
  
  struct Config {
    Provider provider = Provider::kAnthropic;
    std::string model = "claude-sonnet-4-20250514";
    std::string api_key;
    std::string endpoint;
    bool enable_vision = true;
    int max_tokens = 4096;
    float temperature = 0.0f;  // Deterministic for agents
  };
  
  LLMClient();
  ~LLMClient();
  
  void Configure(const Config& config);
  
  using CompletionCallback = base::OnceCallback<void(const std::string&)>;
  
  void GetCompletion(const std::string& prompt,
                     const std::string& image_base64,
                     CompletionCallback callback);

 private:
  void SendAnthropicRequest(const std::string& prompt,
                            const std::string& image_base64,
                            CompletionCallback callback);
  
  void SendOpenAIRequest(const std::string& prompt,
                         const std::string& image_base64,
                         CompletionCallback callback);
  
  void SendLocalRequest(const std::string& prompt,
                        const std::string& image_base64,
                        CompletionCallback callback);
  
  void OnResponse(CompletionCallback callback,
                  std::unique_ptr<std::string> response);
  
  Config config_;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  std::unique_ptr<network::SimpleURLLoader> url_loader_;
  base::WeakPtrFactory<LLMClient> weak_factory_{this};
};

}  // namespace atlas

#endif
```

```cpp
// atlas_browser/features/llm_agent/browser/llm_client.cc

#include "atlas_browser/features/llm_agent/browser/llm_client.h"

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "net/base/load_flags.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/mojom/url_response_head.mojom.h"

namespace atlas {

LLMClient::LLMClient() = default;
LLMClient::~LLMClient() = default;

void LLMClient::Configure(const Config& config) {
  config_ = config;
}

void LLMClient::GetCompletion(
    const std::string& prompt,
    const std::string& image_base64,
    CompletionCallback callback) {
  
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
    default:
      std::move(callback).Run("");
  }
}

void LLMClient::SendAnthropicRequest(
    const std::string& prompt,
    const std::string& image_base64,
    CompletionCallback callback) {
  
  auto request = std::make_unique<network::ResourceRequest>();
  request->url = GURL("https://api.anthropic.com/v1/messages");
  request->method = "POST";
  request->load_flags = net::LOAD_DISABLE_CACHE;
  
  // Headers
  request->headers.SetHeader("x-api-key", config_.api_key);
  request->headers.SetHeader("anthropic-version", "2023-06-01");
  request->headers.SetHeader("content-type", "application/json");
  
  // Build request body
  base::Value::Dict body;
  body.Set("model", config_.model);
  body.Set("max_tokens", config_.max_tokens);
  body.Set("temperature", config_.temperature);
  
  // Messages array
  base::Value::List messages;
  base::Value::Dict message;
  message.Set("role", "user");
  
  // Content array (for multi-modal)
  base::Value::List content;
  
  // Add image if present
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
  
  // Serialize
  std::string body_json;
  base::JSONWriter::Write(body, &body_json);
  
  // Create URL loader
  url_loader_ = network::SimpleURLLoader::Create(
      std::move(request),
      MISSING_TRAFFIC_ANNOTATION);
  
  url_loader_->AttachStringForUpload(body_json, "application/json");
  
  url_loader_->DownloadToString(
      url_loader_factory_.get(),
      base::BindOnce(&LLMClient::OnResponse,
                     weak_factory_.GetWeakPtr(),
                     std::move(callback)),
      1024 * 1024);  // 1MB max response
}

void LLMClient::OnResponse(
    CompletionCallback callback,
    std::unique_ptr<std::string> response) {
  
  if (!response || response->empty()) {
    std::move(callback).Run("");
    return;
  }
  
  // Parse response
  auto json = base::JSONReader::Read(*response);
  if (!json || !json->is_dict()) {
    std::move(callback).Run("");
    return;
  }
  
  // Extract content from Anthropic response
  const auto& dict = json->GetDict();
  const auto* content = dict.FindList("content");
  if (content && !content->empty()) {
    const auto& first = (*content)[0];
    if (first.is_dict()) {
      const auto* text = first.GetDict().FindString("text");
      if (text) {
        std::move(callback).Run(*text);
        return;
      }
    }
  }
  
  std::move(callback).Run("");
}

}  // namespace atlas
```

### DOM Extractor (Renderer Side)

```cpp
// atlas_browser/features/llm_agent/renderer/dom_extractor.cc

#include "atlas_browser/features/llm_agent/renderer/dom_extractor.h"

#include "base/json/json_writer.h"
#include "third_party/blink/public/web/web_document.h"
#include "third_party/blink/public/web/web_element.h"
#include "third_party/blink/public/web/web_element_collection.h"
#include "third_party/blink/public/web/web_form_element.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_node.h"
#include "third_party/blink/renderer/core/dom/element.h"

namespace atlas {

DOMExtractor::DOMExtractor() = default;
DOMExtractor::~DOMExtractor() = default;

std::string DOMExtractor::GetAccessibleDOM(blink::WebLocalFrame* frame) {
  if (!frame)
    return "{}";
  
  blink::WebDocument doc = frame->GetDocument();
  if (doc.IsNull())
    return "{}";
  
  base::Value::Dict result;
  
  // Basic page info
  result.Set("url", doc.Url().GetString().Utf8());
  result.Set("title", doc.Title().Utf8());
  
  // Get viewport info
  blink::WebView* view = frame->View();
  if (view) {
    base::Value::Dict viewport;
    gfx::Size size = view->Size();
    viewport.Set("width", size.width());
    viewport.Set("height", size.height());
    viewport.Set("scrollX", frame->GetScrollOffset().x());
    viewport.Set("scrollY", frame->GetScrollOffset().y());
    result.Set("viewport", std::move(viewport));
  }
  
  // Extract interactive elements
  base::Value::List elements;
  ExtractElements(doc.Body(), &elements, 0);
  result.Set("elements", std::move(elements));
  
  // Serialize
  std::string json;
  base::JSONWriter::WriteWithOptions(
      result,
      base::JSONWriter::OPTIONS_PRETTY_PRINT,
      &json);
  
  return json;
}

void DOMExtractor::ExtractElements(
    const blink::WebElement& element,
    base::Value::List* output,
    int depth) {
  
  if (element.IsNull() || depth > kMaxDepth)
    return;
  
  // Check if element is interesting
  if (IsInteractiveElement(element) && IsVisibleElement(element)) {
    base::Value::Dict elem_data;
    
    // Basic info
    std::string tag = element.TagName().Utf8();
    elem_data.Set("tag", tag);
    
    // ID and classes
    std::string id = element.GetAttribute("id").Utf8();
    if (!id.empty())
      elem_data.Set("id", id);
    
    std::string classes = element.GetAttribute("class").Utf8();
    if (!classes.empty())
      elem_data.Set("class", classes);
    
    // Text content (truncated)
    std::string text = GetVisibleText(element);
    if (!text.empty()) {
      if (text.length() > 100) {
        text = text.substr(0, 100) + "...";
      }
      elem_data.Set("text", text);
    }
    
    // Accessibility attributes
    std::string aria_label = element.GetAttribute("aria-label").Utf8();
    if (!aria_label.empty())
      elem_data.Set("aria-label", aria_label);
    
    std::string placeholder = element.GetAttribute("placeholder").Utf8();
    if (!placeholder.empty())
      elem_data.Set("placeholder", placeholder);
    
    // Link href
    if (tag == "A") {
      std::string href = element.GetAttribute("href").Utf8();
      if (!href.empty())
        elem_data.Set("href", href);
    }
    
    // Input type and value
    if (tag == "INPUT") {
      elem_data.Set("type", element.GetAttribute("type").Utf8());
      elem_data.Set("value", element.GetAttribute("value").Utf8());
      elem_data.Set("name", element.GetAttribute("name").Utf8());
    }
    
    // Generate unique selector
    elem_data.Set("selector", GenerateSelector(element));
    
    // Bounding box
    gfx::Rect rect = element.BoundsInWidget();
    base::Value::Dict bounds;
    bounds.Set("x", rect.x());
    bounds.Set("y", rect.y());
    bounds.Set("width", rect.width());
    bounds.Set("height", rect.height());
    elem_data.Set("bounds", std::move(bounds));
    
    // Role
    std::string role = element.GetAttribute("role").Utf8();
    if (!role.empty()) {
      elem_data.Set("role", role);
    } else {
      // Infer role from tag
      elem_data.Set("role", InferRole(element));
    }
    
    output->Append(std::move(elem_data));
  }
  
  // Recurse through children
  for (blink::WebNode child = element.FirstChild();
       !child.IsNull();
       child = child.NextSibling()) {
    if (child.IsElementNode()) {
      ExtractElements(child.To<blink::WebElement>(), output, depth + 1);
    }
  }
}

bool DOMExtractor::IsInteractiveElement(const blink::WebElement& element) {
  std::string tag = element.TagName().Utf8();
  
  // Standard interactive elements
  static const base::flat_set<std::string> kInteractiveTags = {
    "A", "BUTTON", "INPUT", "SELECT", "TEXTAREA",
    "DETAILS", "SUMMARY", "DIALOG"
  };
  
  if (kInteractiveTags.contains(tag))
    return true;
  
  // Elements with interactive attributes
  if (element.HasAttribute("onclick") ||
      element.HasAttribute("onmousedown") ||
      element.HasAttribute("onmouseup") ||
      element.HasAttribute("href") ||
      element.HasAttribute("tabindex")) {
    return true;
  }
  
  // Elements with interactive roles
  std::string role = element.GetAttribute("role").Utf8();
  static const base::flat_set<std::string> kInteractiveRoles = {
    "button", "link", "menuitem", "option", "tab",
    "checkbox", "radio", "switch", "textbox", "combobox"
  };
  
  if (kInteractiveRoles.contains(role))
    return true;
  
  // Clickable by cursor style
  // (Would need computed style access)
  
  return false;
}

bool DOMExtractor::IsVisibleElement(const blink::WebElement& element) {
  gfx::Rect bounds = element.BoundsInWidget();
  
  // Check if has size
  if (bounds.width() <= 0 || bounds.height() <= 0)
    return false;
  
  // Check common hidden attributes
  if (element.HasAttribute("hidden"))
    return false;
  
  std::string style = element.GetAttribute("style").Utf8();
  if (style.find("display: none") != std::string::npos ||
      style.find("display:none") != std::string::npos ||
      style.find("visibility: hidden") != std::string::npos) {
    return false;
  }
  
  // Check aria-hidden
  if (element.GetAttribute("aria-hidden").Utf8() == "true")
    return false;
  
  return true;
}

std::string DOMExtractor::GenerateSelector(const blink::WebElement& element) {
  // Try ID first (most specific)
  std::string id = element.GetAttribute("id").Utf8();
  if (!id.empty() && id.find(' ') == std::string::npos) {
    return "#" + id;
  }
  
  // Build path selector
  std::vector<std::string> path;
  blink::WebElement current = element;
  
  while (!current.IsNull() && path.size() < 5) {
    std::string part = current.TagName().Utf8();
    
    // Add ID if present
    std::string curr_id = current.GetAttribute("id").Utf8();
    if (!curr_id.empty() && curr_id.find(' ') == std::string::npos) {
      part = "#" + curr_id;
      path.insert(path.begin(), part);
      break;  // ID is unique enough
    }
    
    // Add unique class if present
    std::string classes = current.GetAttribute("class").Utf8();
    if (!classes.empty()) {
      // Find first class that might be unique
      size_t space = classes.find(' ');
      std::string first_class = (space != std::string::npos)
          ? classes.substr(0, space)
          : classes;
      if (!first_class.empty()) {
        part += "." + first_class;
      }
    }
    
    // Add nth-child if needed for uniqueness
    int index = 1;
    blink::WebNode sibling = current.PreviousSibling();
    while (!sibling.IsNull()) {
      if (sibling.IsElementNode() &&
          sibling.To<blink::WebElement>().TagName() == current.TagName()) {
        index++;
      }
      sibling = sibling.PreviousSibling();
    }
    if (index > 1) {
      part += ":nth-of-type(" + base::NumberToString(index) + ")";
    }
    
    path.insert(path.begin(), part);
    
    blink::WebNode parent = current.ParentNode();
    if (parent.IsNull() || !parent.IsElementNode())
      break;
    current = parent.To<blink::WebElement>();
  }
  
  return base::JoinString(path, " > ");
}

std::string DOMExtractor::GetVisibleText(const blink::WebElement& element) {
  // Get text content, but clean it up
  std::string text = element.TextContent().Utf8();
  
  // Trim and collapse whitespace
  text = base::TrimWhitespaceASCII(text, base::TRIM_ALL);
  
  // Collapse multiple spaces
  std::string result;
  bool last_was_space = false;
  for (char c : text) {
    if (std::isspace(c)) {
      if (!last_was_space) {
        result += ' ';
        last_was_space = true;
      }
    } else {
      result += c;
      last_was_space = false;
    }
  }
  
  return result;
}

std::string DOMExtractor::InferRole(const blink::WebElement& element) {
  std::string tag = element.TagName().Utf8();
  
  static const base::flat_map<std::string, std::string> kTagToRole = {
    {"A", "link"},
    {"BUTTON", "button"},
    {"INPUT", "textbox"},
    {"SELECT", "combobox"},
    {"TEXTAREA", "textbox"},
    {"IMG", "img"},
    {"NAV", "navigation"},
    {"MAIN", "main"},
    {"HEADER", "banner"},
    {"FOOTER", "contentinfo"},
    {"ASIDE", "complementary"},
    {"ARTICLE", "article"},
    {"SECTION", "region"},
  };
  
  auto it = kTagToRole.find(tag);
  return it != kTagToRole.end() ? it->second : "";
}

}  // namespace atlas
```

---

## 🖥️ Custom UI

### Agent Panel View

```cpp
// atlas_browser/browser/ui/views/agent_panel_view.cc

#include "atlas_browser/browser/ui/views/agent_panel_view.h"

#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/views/background.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/scroll_view.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"

namespace atlas {

AgentPanelView::AgentPanelView(Browser* browser)
    : browser_(browser) {
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      gfx::Insets(16),
      12));
  
  SetBackground(views::CreateThemedSolidBackground(
      ui::kColorDialogBackground));
  
  SetPreferredSize(gfx::Size(400, 600));
  
  // Header
  auto* header = AddChildView(std::make_unique<views::Label>(
      u"🤖 AI Agent",
      views::Label::CustomFont{
          gfx::FontList().DeriveWithSizeDelta(4)
              .DeriveWithWeight(gfx::Font::Weight::BOLD)}));
  header->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  
  // Task input section
  auto* task_label = AddChildView(std::make_unique<views::Label>(
      u"What would you like me to do?"));
  task_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  
  task_input_ = AddChildView(std::make_unique<views::Textfield>());
  task_input_->SetPlaceholderText(
      u"e.g., Find the cheapest flight to Paris next month");
  task_input_->SetController(this);
  
  // Button row
  auto* button_row = AddChildView(std::make_unique<views::View>());
  button_row->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal,
      gfx::Insets(),
      8));
  
  start_button_ = button_row->AddChildView(
      std::make_unique<views::MdTextButton>(
          base::BindRepeating(&AgentPanelView::OnStartClicked,
                              base::Unretained(this)),
          u"▶️ Start"));
  start_button_->SetProminent(true);
  
  pause_button_ = button_row->AddChildView(
      std::make_unique<views::MdTextButton>(
          base::BindRepeating(&AgentPanelView::OnPauseClicked,
                              base::Unretained(this)),
          u"⏸️ Pause"));
  pause_button_->SetEnabled(false);
  
  stop_button_ = button_row->AddChildView(
      std::make_unique<views::MdTextButton>(
          base::BindRepeating(&AgentPanelView::OnStopClicked,
                              base::Unretained(this)),
          u"⏹️ Stop"));
  stop_button_->SetEnabled(false);
  stop_button_->SetStyle(views::MdTextButton::kStyle::kAlert);
  
  // Status
  status_label_ = AddChildView(std::make_unique<views::Label>(u"Ready"));
  status_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  
  // Action log
  auto* log_label = AddChildView(std::make_unique<views::Label>(
      u"Action Log"));
  log_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  
  auto* scroll_view = AddChildView(std::make_unique<views::ScrollView>());
  scroll_view->SetPreferredSize(gfx::Size(368, 300));
  scroll_view->SetBackgroundColor(SK_ColorWHITE);
  
  action_log_container_ = scroll_view->SetContents(
      std::make_unique<views::View>());
  action_log_container_->SetLayoutManager(
      std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical,
          gfx::Insets(8),
          4));
  
  // Register as observer
  AgentService::GetForBrowserContext(browser_->profile())
      ->AddObserver(this);
}

AgentPanelView::~AgentPanelView() {
  if (browser_) {
    AgentService::GetForBrowserContext(browser_->profile())
        ->RemoveObserver(this);
  }
}

void AgentPanelView::OnStartClicked() {
  std::string task = base::UTF16ToUTF8(task_input_->GetText());
  if (task.empty())
    return;
  
  auto* web_contents = browser_->tab_strip_model()
      ->GetActiveWebContents();
  if (!web_contents)
    return;
  
  current_task_id_ = AgentService::GetForBrowserContext(browser_->profile())
      ->StartTask(web_contents, task,
          base::BindOnce(&AgentPanelView::OnTaskComplete,
                         weak_factory_.GetWeakPtr()));
  
  UpdateButtonState(true);
  status_label_->SetText(u"🔄 Running...");
}

void AgentPanelView::OnPauseClicked() {
  if (!current_task_id_.empty()) {
    AgentService::GetForBrowserContext(browser_->profile())
        ->PauseTask(current_task_id_);
    status_label_->SetText(u"⏸️ Paused");
    pause_button_->SetText(u"▶️ Resume");
    is_paused_ = true;
  }
}

void AgentPanelView::OnStopClicked() {
  if (!current_task_id_.empty()) {
    AgentService::GetForBrowserContext(browser_->profile())
        ->CancelTask(current_task_id_);
    current_task_id_.clear();
    UpdateButtonState(false);
    status_label_->SetText(u"⏹️ Stopped");
  }
}

void AgentPanelView::OnTaskProgress(
    const std::string& task_id,
    const agent::mojom::AgentStep& step) {
  
  if (task_id != current_task_id_)
    return;
  
  // Add log entry
  auto* entry = action_log_container_->AddChildView(
      std::make_unique<AgentLogEntryView>(step));
  
  // Scroll to bottom
  action_log_container_->InvalidateLayout();
}

void AgentPanelView::OnTaskComplete(agent::mojom::TaskResult result) {
  current_task_id_.clear();
  UpdateButtonState(false);
  
  if (result.status == agent::mojom::TaskStatus::kCompleted) {
    status_label_->SetText(u"✅ Complete!");
  } else {
    status_label_->SetText(u"❌ Failed");
  }
}

void AgentPanelView::UpdateButtonState(bool is_running) {
  start_button_->SetEnabled(!is_running);
  pause_button_->SetEnabled(is_running);
  stop_button_->SetEnabled(is_running);
  task_input_->SetEnabled(!is_running);
}

BEGIN_METADATA(AgentPanelView, views::View)
END_METADATA

}  // namespace atlas
```

---

## 🔨 Build & Distribution

### Build Script

```bash
#!/bin/bash
# build_atlas.sh

set -e

VERSION="${VERSION:-1.0.0}"
BUILD_TYPE="${BUILD_TYPE:-Release}"
PLATFORM=$(uname -s | tr '[:upper:]' '[:lower:]')
ARCH=$(uname -m)

echo "🔨 Building Atlas Browser v${VERSION}"
echo "   Platform: ${PLATFORM}"
echo "   Arch: ${ARCH}"
echo "   Type: ${BUILD_TYPE}"

cd ~/chromium/src

# Build args
ARGS="
is_debug=false
is_component_build=false
is_official_build=true
symbol_level=0
enable_nacl=false
blink_symbol_level=0
v8_symbol_level=0

# Branding
chrome_pgo_phase=0
proprietary_codecs=true
ffmpeg_branding=\"Chrome\"

# Atlas-specific
enable_atlas_agent=true
atlas_version=\"${VERSION}\"

# Optimizations
use_thin_lto=true
is_cfi=true

# Remove Google services
google_api_key=\"\"
google_default_client_id=\"\"
google_default_client_secret=\"\"
safe_browsing_mode=0
"

# Generate build files
echo "📝 Generating build files..."
gn gen "out/${BUILD_TYPE}" --args="${ARGS}"

# Build
echo "🏗️ Building (this may take a while)..."
autoninja -C "out/${BUILD_TYPE}" atlas_browser

echo "✅ Build complete!"
echo "   Binary: out/${BUILD_TYPE}/atlas"

# Package
if [ "$PACKAGE" = "true" ]; then
  echo "📦 Creating package..."
  
  if [ "$PLATFORM" = "linux" ]; then
    # Create .deb package
    ./chrome/installer/linux/build_deb.py \
      --output-dir="dist/" \
      --build-dir="out/${BUILD_TYPE}" \
      --product-dir="out/${BUILD_TYPE}"
      
    # Create AppImage
    ./tools/build/appimage/create_appimage.py \
      --build-dir="out/${BUILD_TYPE}" \
      --output="dist/AtlasBrowser-${VERSION}-${ARCH}.AppImage"
      
  elif [ "$PLATFORM" = "darwin" ]; then
    # Create .dmg
    ./chrome/installer/mac/build_dmg.py \
      --build-dir="out/${BUILD_TYPE}" \
      --output="dist/AtlasBrowser-${VERSION}.dmg"
      
    # Sign for notarization
    if [ -n "$APPLE_SIGNING_IDENTITY" ]; then
      codesign --force --deep --sign "$APPLE_SIGNING_IDENTITY" \
        "out/${BUILD_TYPE}/Atlas Browser.app"
    fi
  fi
  
  echo "📦 Package created in dist/"
fi
```

### GN Args File (för reproducerbarhet)

```gn
# args.gn - Fullständig konfiguration

# === Build Type ===
is_debug = false
is_component_build = false
is_official_build = true

# === Symbols ===
symbol_level = 0
blink_symbol_level = 0
v8_symbol_level = 0

# === Optimizations ===
use_thin_lto = true
is_cfi = true
use_cfi_cast = true

# === Features ===
enable_nacl = false
enable_reading_list = false
enable_side_search = false

# === Media ===
proprietary_codecs = true
ffmpeg_branding = "Chrome"
enable_hevc_parser_and_hw_decoder = true

# === Google Services (disabled) ===
google_api_key = ""
google_default_client_id = ""
google_default_client_secret = ""
safe_browsing_mode = 0
enable_hangout_services_extension = false
enable_service_discovery = false
enable_mdns = false
enable_remoting = false

# === Atlas Features ===
enable_atlas_agent = true
atlas_version = "1.0.0"
atlas_enable_local_llm = true
atlas_default_llm_provider = "anthropic"
```

---

## 🔄 Underhåll & Uppdateringar

### Rebase mot upstream Chromium

```bash
#!/bin/bash
# update_chromium.sh

cd ~/chromium/src

# Spara våra ändringar
git stash

# Hämta senaste Chromium
git fetch origin

# Hitta senaste stabila versionen
LATEST=$(git tag -l "1*" | sort -V | tail -1)
echo "Latest Chromium: ${LATEST}"

# Checkout och rebase
git checkout main
git rebase origin/main

# Återställ våra ändringar
git stash pop

# Uppdatera beroenden
gclient sync

# Bygg om
autoninja -C out/Release atlas_browser
```

### Auto-update System

```cpp
// atlas_browser/browser/update/update_client.cc

namespace atlas {

class AtlasUpdateClient {
 public:
  struct UpdateInfo {
    std::string version;
    std::string download_url;
    std::string sha256;
    std::string release_notes;
    bool is_critical;
  };
  
  void CheckForUpdates() {
    auto request = std::make_unique<network::ResourceRequest>();
    request->url = GURL(kUpdateCheckUrl);
    request->method = "GET";
    
    url_loader_->DownloadToString(
        url_loader_factory_.get(),
        base::BindOnce(&AtlasUpdateClient::OnUpdateCheckComplete,
                       weak_factory_.GetWeakPtr()),
        kMaxResponseSize);
  }
  
  void OnUpdateCheckComplete(std::unique_ptr<std::string> response) {
    if (!response)
      return;
    
    auto info = ParseUpdateInfo(*response);
    if (!info)
      return;
    
    // Jämför versioner
    if (IsNewerVersion(info->version, GetCurrentVersion())) {
      NotifyUpdateAvailable(*info);
    }
  }
  
  void DownloadUpdate(const UpdateInfo& info) {
    // Delta-updates om möjligt
    // Bakgrundsdownload
    // Verifiera SHA256
    // Atomic swap vid installation
  }

 private:
  static constexpr char kUpdateCheckUrl[] = 
      "https://updates.atlasbrowser.com/api/v1/check";
};

}  // namespace atlas
```

---

## 📊 Sammanfattning

### Resurskrav

| Fas | Team | Tid | Budget |
|-----|------|-----|--------|
| **MVP** | 3-5 devs | 6-12 mån | ~$500K |
| **Beta** | 5-10 devs | 12-18 mån | ~$1-2M |
| **Production** | 10-20 devs | 18-24 mån | ~$3-5M |
| **Maintenance** | 5-10 devs | Ongoing | ~$1M/år |

### Kritiska framgångsfaktorer

1. **Chromium-expertis** - Teamet måste förstå Chromium's arkitektur
2. **CI/CD pipeline** - Automatiserade builds krävs (tar timmar)
3. **Update infrastructure** - Säkra auto-updates
4. **Testing** - Omfattande test coverage
5. **Security audits** - Regelbundna säkerhetsgranskningar

### Alternativ för mindre team

| Approach | Komplexitet | Kontroll |
|----------|-------------|----------|
| **Electron** | Låg | Låg |
| **CEF** | Medium | Medium |
| **Chromium fork** | Hög | Full |
| **WebView2** | Låg | Låg |

---

## 🔗 Referenser

- [Chromium Getting Started](https://chromium.googlesource.com/chromium/src/+/main/docs/get_the_code.md)
- [GN Build System](https://gn.googlesource.com/gn/+/main/docs/)
- [Mojo IPC](https://chromium.googlesource.com/chromium/src/+/main/mojo/README.md)
- [Brave Repository](https://github.com/brave/brave-browser)
- [Views UI Toolkit](https://chromium.googlesource.com/chromium/src/+/main/docs/ui/views/)
