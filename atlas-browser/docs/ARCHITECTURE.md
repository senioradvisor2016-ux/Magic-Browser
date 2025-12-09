# Atlas Browser Architecture

## Overview

Atlas Browser is a Chromium-based browser with integrated AI agent capabilities. This document describes the technical architecture.

## System Architecture

```
┌─────────────────────────────────────────────────────────────────────────┐
│                           ATLAS BROWSER                                  │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│  ┌────────────────────────────────────────────────────────────────────┐ │
│  │                      BROWSER PROCESS                                │ │
│  │                                                                     │ │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────────────┐ │ │
│  │  │   Browser   │  │   Agent     │  │         LLM Client          │ │ │
│  │  │   UI/UX     │  │   Service   │  │                             │ │ │
│  │  │             │  │             │  │  ┌────────┐ ┌────────────┐  │ │ │
│  │  │ - Toolbar   │  │ - Tasks     │  │  │Anthropic│ │  OpenAI   │  │ │ │
│  │  │ - Panel     │  │ - State     │  │  └────────┘ └────────────┘  │ │ │
│  │  │ - Settings  │  │ - Actions   │  │  ┌────────┐ ┌────────────┐  │ │ │
│  │  │             │  │             │  │  │ Ollama │ │   Custom   │  │ │ │
│  │  └─────────────┘  └──────┬──────┘  │  └────────┘ └────────────┘  │ │ │
│  │                          │         └─────────────────────────────┘ │ │
│  │                          │                                         │ │
│  │                    ┌─────▼─────┐                                   │ │
│  │                    │   Mojo    │                                   │ │
│  │                    │    IPC    │                                   │ │
│  │                    └─────┬─────┘                                   │ │
│  │                          │                                         │ │
│  └──────────────────────────┼─────────────────────────────────────────┘ │
│                             │                                            │
│  ┌──────────────────────────▼─────────────────────────────────────────┐ │
│  │                    RENDERER PROCESS (per tab)                       │ │
│  │                                                                     │ │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────────────┐ │ │
│  │  │   Blink     │  │    V8       │  │      Agent Injector         │ │ │
│  │  │  (Layout)   │  │ (JavaScript)│  │                             │ │ │
│  │  │             │  │             │  │  ┌────────────────────────┐ │ │ │
│  │  │ - DOM Tree  │  │ - Execution │  │  │    DOM Extractor       │ │ │ │
│  │  │ - CSS       │  │ - Events    │  │  │  - Interactive elems   │ │ │ │
│  │  │ - Paint     │  │             │  │  │  - Selectors           │ │ │ │
│  │  │             │  │             │  │  │  - Page state          │ │ │ │
│  │  └─────────────┘  └─────────────┘  │  └────────────────────────┘ │ │ │
│  │                                    │  ┌────────────────────────┐ │ │ │
│  │                                    │  │   Action Executor      │ │ │ │
│  │                                    │  │  - Click               │ │ │ │
│  │                                    │  │  - Type                │ │ │ │
│  │                                    │  │  - Scroll              │ │ │ │
│  │                                    │  └────────────────────────┘ │ │ │
│  │                                    └─────────────────────────────┘ │ │
│  └────────────────────────────────────────────────────────────────────┘ │
│                                                                          │
└─────────────────────────────────────────────────────────────────────────┘
```

## Process Model

Atlas Browser follows Chromium's multi-process architecture:

### Browser Process
- Single instance per browser
- Manages UI, tabs, extensions
- Hosts `AgentService` (coordinates LLM interactions)
- Handles network requests via `LLMClient`

### Renderer Process
- One per tab (site isolation)
- Executes web content (Blink + V8)
- Hosts `AgentInjector` (DOM access, action execution)

### GPU Process
- Hardware acceleration
- Screenshot capture for vision models

## Agent System

### Agent Loop

```
┌─────────────┐
│  User Task  │
│  "Find..."  │
└──────┬──────┘
       │
       ▼
┌──────────────┐     ┌─────────────┐
│ AgentService │────►│ Screenshot  │
│              │     │  Capturer   │
└──────┬───────┘     └──────┬──────┘
       │                    │
       │                    ▼
       │            ┌──────────────┐
       │            │    Image     │
       │            │   (base64)   │
       │            └──────┬───────┘
       │                   │
       ▼                   │
┌──────────────┐           │
│ DOM Extractor│◄──────────┘
│  (renderer)  │
└──────┬───────┘
       │
       ▼
┌──────────────┐
│  Page State  │
│    (JSON)    │
└──────┬───────┘
       │
       ▼
┌──────────────┐
│  LLMClient   │──────► API Call
│              │        (Claude/GPT)
└──────┬───────┘
       │
       ▼
┌──────────────┐
│   Response   │
│  { action }  │
└──────┬───────┘
       │
       ▼
┌──────────────┐
│   Execute    │
│   Action     │
└──────┬───────┘
       │
       ▼
    [Loop]
```

### Components

#### AgentService (Browser)
```cpp
// Key responsibilities:
// - Task lifecycle management
// - LLM coordination
// - Observer pattern for UI updates

class AgentService : public KeyedService {
  std::string StartTask(WebContents*, task, callback);
  void PauseTask(task_id);
  void ResumeTask(task_id);
  void CancelTask(task_id);
};
```

#### LLMClient (Browser)
```cpp
// Supports multiple providers:
// - Anthropic (Claude)
// - OpenAI (GPT-4)
// - Local (Ollama)
// - Custom endpoint

void GetCompletion(prompt, image_base64, callback);
```

#### DOMExtractor (Renderer)
```cpp
// Extracts interactive elements for LLM context:
// - Buttons, links, inputs
// - ARIA labels and roles
// - Bounding boxes
// - CSS selectors

std::string GetAccessibleDOM(WebLocalFrame*);
std::vector<ElementInfo> GetInteractiveElements();
```

#### AgentInjector (Renderer)
```cpp
// Executes actions on the page:
// - Click, type, scroll
// - Form filling
// - Element highlighting

ActionResult PerformAction(AgentAction);
```

## IPC (Mojo)

### Interface Definition

```mojom
// Browser → Renderer
interface AgentRendererClient {
  GetAccessibleDOM() => (string dom, array<ElementInfo> elements);
  PerformAction(AgentAction action) => (ActionResult result);
  HighlightElement(string selector, bool show);
};

// Renderer → Browser
interface AgentBrowserHost {
  OnPageStateChanged(PageState state);
  OnNavigationCompleted(Url url, bool success);
  LogAgentStep(AgentStep step);
};
```

## Privacy Features

### Tracker Blocker

```cpp
class TrackerBlocker {
  bool ShouldBlock(const GURL& url);
  void AddBlockedDomain(domain);
  
  // Default blocked:
  // - google-analytics.com
  // - doubleclick.net
  // - facebook tracking
  // - etc.
};
```

## Extension Points

### Agent API (for extensions)

```javascript
// Planned API for extensions
atlas.agent.runTask("Find the best price", {
  maxSteps: 20,
  onProgress: (step) => console.log(step)
}).then(result => {
  console.log(result.answer);
});
```

## Build System

### GN Targets

```gn
# Main executable
atlas_browser_exe

# Agent feature
//atlas-browser/src/atlas_browser/features/llm_agent

# Privacy feature  
//atlas-browser/src/atlas_browser/features/privacy

# Tests
atlas_browser_unittests
atlas_browser_browsertests
```

## Performance Considerations

### LLM Latency
- Screenshot capture: ~50ms
- DOM extraction: ~20ms
- API roundtrip: 500-2000ms
- Action execution: ~100ms

### Memory Usage
- Base browser: ~300MB
- Per tab: ~50-200MB
- Agent active: +50MB

## Security Model

### Sandboxing
- Renderer processes are sandboxed
- Agent actions go through IPC
- No direct DOM access from browser process

### API Security
- API keys stored securely
- Local LLM option for sensitive data
- No telemetry by default

## Future Architecture

### Planned Improvements
1. **Multi-tab agent**: Coordinate across tabs
2. **Persistent memory**: Remember past interactions
3. **Plugin system**: Custom action types
4. **Streaming responses**: Real-time LLM output
