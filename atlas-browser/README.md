# 🌐 Atlas Browser

> AI-powered Chromium browser with built-in agent capabilities

[![Build Linux](https://github.com/atlas-browser/atlas-browser/actions/workflows/build-linux.yml/badge.svg)](https://github.com/atlas-browser/atlas-browser/actions/workflows/build-linux.yml)
[![Build macOS](https://github.com/atlas-browser/atlas-browser/actions/workflows/build-macos.yml/badge.svg)](https://github.com/atlas-browser/atlas-browser/actions/workflows/build-macos.yml)
[![License](https://img.shields.io/badge/license-BSD--3-blue.svg)](LICENSE)

---

## 🚀 Features

### 🤖 AI Agent
- **Natural Language Tasks**: Tell the agent what you want to accomplish
- **DOM Understanding**: Intelligent extraction of page structure
- **Visual Recognition**: Screenshot-based understanding with vision models
- **Multi-step Automation**: Complete complex workflows automatically

### 🔒 Privacy First
- **Tracker Blocking**: Built-in protection against tracking scripts
- **Fingerprint Protection**: Resist browser fingerprinting
- **No Telemetry**: Your data stays on your device

### ⚡ Performance
- **Chromium-based**: Full compatibility with the modern web
- **Optimized Builds**: Release builds with LTO and CFI
- **Local LLM Support**: Run agents with Ollama for complete privacy

---

## 📸 Screenshots

```
┌─────────────────────────────────────────────────────────────────┐
│ ← → ↻  [═══════════════════ URL ════════════════════]  🤖 ⚙️  │
├─────────────────────────────────────────────────────────────────┤
│                                           ┌───────────────────┐ │
│                                           │   🤖 AI Agent     │ │
│                                           ├───────────────────┤ │
│                                           │ What should I do? │ │
│         W E B   C O N T E N T            │ ┌───────────────┐ │ │
│                                           │ │ Find best...  │ │ │
│                                           │ └───────────────┘ │ │
│                                           │ [▶️ Start]        │ │
│                                           │                   │ │
│                                           │ ─── Action Log ── │ │
│                                           │ ✓ Clicked search  │ │
│                                           │ ✓ Typed query     │ │
│                                           │ → Analyzing...    │ │
│                                           └───────────────────┘ │
└─────────────────────────────────────────────────────────────────┘
```

---

## 📦 Installation

### Pre-built Binaries

Download the latest release from the [Releases](https://github.com/atlas-browser/atlas-browser/releases) page.

#### Linux
```bash
# Download and extract
tar -xzf atlas-browser-*.tar.gz
cd atlas-browser-*

# Run
./atlas
```

#### macOS
```bash
# Download DMG and drag to Applications
open AtlasBrowser-*.dmg
```

### Build from Source

See [Building](#-building) section below.

---

## 🔨 Building

### Prerequisites

- **OS**: Linux (Ubuntu 20.04+) or macOS (11+)
- **Disk**: 100GB+ free space
- **RAM**: 16GB minimum, 32GB+ recommended
- **CPU**: 8+ cores recommended

### Quick Start

```bash
# Clone Atlas Browser
git clone https://github.com/atlas-browser/atlas-browser.git
cd atlas-browser

# Run setup (installs dependencies, fetches Chromium)
./scripts/setup.sh

# Build
./scripts/build.sh
```

### Manual Build

```bash
# 1. Install depot_tools
git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git ~/depot_tools
export PATH="$HOME/depot_tools:$PATH"

# 2. Fetch Chromium (~30GB)
mkdir ~/chromium && cd ~/chromium
fetch --nohooks --no-history chromium
cd src
gclient runhooks

# 3. Link Atlas Browser
ln -sf /path/to/atlas-browser ~/chromium/src/atlas-browser

# 4. Generate build files
gn gen out/AtlasDebug --args='import("//atlas-browser/args.gn") is_debug=true'

# 5. Build
autoninja -C out/AtlasDebug atlas_browser

# 6. Run
./out/AtlasDebug/atlas
```

### Build Options

```bash
# Debug build (faster compile, larger binary)
./scripts/build.sh --debug

# Release build (slower compile, optimized)
./scripts/build.sh --release

# Clean build
./scripts/build.sh --clean

# Parallel jobs
./scripts/build.sh --jobs 32
```

---

## 🤖 Using the AI Agent

### Starting a Task

1. Click the 🤖 button in the toolbar (or press `Cmd+Shift+A`)
2. Enter a task description in natural language
3. Click **Start**
4. Watch as the agent navigates and interacts with the page

### Example Tasks

```
"Find the cheapest flight from NYC to Paris in December"

"Fill out this form with my saved information"

"Find all products under $50 and add the best-rated to cart"

"Search for 'machine learning' and summarize the top 3 results"
```

### Configuration

Open Settings → Agent to configure:

- **LLM Provider**: Anthropic, OpenAI, or Local (Ollama)
- **Model**: claude-sonnet-4-20250514, gpt-4o, llama3, etc.
- **Vision**: Enable/disable screenshot understanding
- **Max Steps**: Limit automation steps (default: 50)

### Using Local LLM

For complete privacy, use a local model:

```bash
# Install Ollama
curl -fsSL https://ollama.com/install.sh | sh

# Pull a model
ollama pull llama3

# Configure Atlas to use local
# Settings → Agent → Provider: Local
# Model: llama3
```

---

## 🏗️ Architecture

```
atlas-browser/
├── src/atlas_browser/
│   ├── browser/                 # Browser process code
│   │   ├── atlas_main.cc       # Entry point
│   │   ├── atlas_main_delegate.cc
│   │   └── ui/views/           # UI components
│   ├── features/
│   │   ├── llm_agent/          # AI Agent system
│   │   │   ├── browser/        # Browser-side (AgentService, LLMClient)
│   │   │   ├── renderer/       # Renderer-side (DOMExtractor)
│   │   │   └── public/mojom/   # Mojo interfaces
│   │   └── privacy/            # Privacy features
│   └── resources/              # Icons, strings, etc.
├── branding/                    # Product branding
├── scripts/                     # Build scripts
└── .github/workflows/           # CI/CD
```

### Key Components

| Component | Description |
|-----------|-------------|
| `AgentService` | Manages agent tasks and coordinates LLM |
| `LLMClient` | Handles API calls to Anthropic/OpenAI/Local |
| `DOMExtractor` | Extracts interactive elements from pages |
| `AgentInjector` | Renderer-side action execution |
| `AgentPanelView` | UI for controlling the agent |
| `TrackerBlocker` | Blocks known tracking domains |

---

## 🧪 Development

### Running Tests

```bash
# Unit tests
autoninja -C out/AtlasDebug atlas_browser_unittests
./out/AtlasDebug/atlas_browser_unittests

# Browser tests
autoninja -C out/AtlasDebug atlas_browser_browsertests
./out/AtlasDebug/atlas_browser_browsertests
```

### Code Style

We follow the [Chromium C++ Style Guide](https://chromium.googlesource.com/chromium/src/+/main/styleguide/c++/c++.md).

```bash
# Format code
git cl format
```

### Debugging

```bash
# Run with verbose logging
./out/AtlasDebug/atlas --enable-logging=stderr --v=1

# Debug with gdb
gdb ./out/AtlasDebug/atlas
```

---

## 🤝 Contributing

We welcome contributions! Please see [CONTRIBUTING.md](docs/CONTRIBUTING.md) for guidelines.

### Areas We Need Help

- [ ] Windows build support
- [ ] Extension API for agent
- [ ] More LLM provider integrations
- [ ] Improved DOM extraction
- [ ] Accessibility improvements
- [ ] Internationalization

---

## 📄 License

Atlas Browser is licensed under the BSD 3-Clause License. See [LICENSE](LICENSE) for details.

Chromium and its components are licensed under their respective licenses.

---

## 🙏 Acknowledgments

- [Chromium Project](https://www.chromium.org/) - The browser engine
- [Anthropic](https://www.anthropic.com/) - Claude AI models
- [Brave](https://brave.com/) - Inspiration for privacy features

---

## 📞 Support

- **Issues**: [GitHub Issues](https://github.com/atlas-browser/atlas-browser/issues)
- **Discussions**: [GitHub Discussions](https://github.com/atlas-browser/atlas-browser/discussions)
- **Email**: support@atlasbrowser.com

---

<p align="center">
  Made with ❤️ by the Atlas Browser Team
</p>
