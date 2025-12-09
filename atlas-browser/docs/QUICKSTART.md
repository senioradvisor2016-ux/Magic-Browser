# Atlas Browser Quick Start Guide

Get up and running with Atlas Browser in minutes!

## 🚀 Installation

### Option 1: Pre-built Binaries (Recommended)

Download the latest release for your platform:

- **Windows**: `AtlasBrowserSetup.exe`
- **macOS**: `AtlasBrowser.dmg`
- **Linux**: `atlas-browser.deb` or `atlas-browser.tar.gz`

### Option 2: Build from Source

```bash
# Clone the repository
git clone https://github.com/atlas-browser/atlas-browser.git
cd atlas-browser

# Set up (downloads Chromium - takes ~1 hour)
make setup

# Build (takes ~2-4 hours first time)
make build

# Run
make run
```

## ⚙️ Initial Setup

### 1. Configure AI Agent

1. Open Atlas Browser
2. Go to `atlas://settings` or press `Ctrl+,`
3. Select your LLM provider:
   - **Anthropic** (Claude) - Recommended
   - **OpenAI** (GPT-4)
   - **Local** (Ollama)
4. Enter your API key
5. Click "Save"

### 2. Get API Keys

**Anthropic Claude:**
1. Go to https://console.anthropic.com/
2. Create an account
3. Generate an API key
4. Copy and paste into Atlas settings

**OpenAI:**
1. Go to https://platform.openai.com/
2. Create an account
3. Generate an API key
4. Copy and paste into Atlas settings

**Local (Ollama):**
1. Install Ollama: https://ollama.ai/
2. Run: `ollama pull llama3`
3. Select "Local" in Atlas settings
4. No API key needed!

## 🤖 Using the AI Agent

### Method 1: Agent Panel

1. Click the 🤖 button in the toolbar (or press `Ctrl+Shift+A`)
2. Type your task: "Find the cheapest flight from NYC to LA"
3. Click "Start" and watch the agent work!

### Method 2: Omnibox Commands

Type in the URL bar:

```
@agent Search for the best rated restaurants near me
@ask What is this page about?
@summarize
@translate
```

### Method 3: Context Menu

1. Right-click on any page
2. Select "Atlas Agent" submenu
3. Choose an action:
   - Summarize Page
   - Extract Data
   - Explain Selection
   - And more...

## 📝 Example Tasks

### Shopping
```
Find the best price for iPhone 16 Pro Max across Amazon, 
Best Buy, and Apple Store
```

### Research
```
Search for recent news about AI regulations, summarize the 
top 5 articles, and create a brief report
```

### Productivity
```
Go to my Gmail, find all unread emails from this week, 
and summarize them
```

### Data Extraction
```
Extract all product names, prices, and ratings from this page 
as a JSON file
```

## ⌨️ Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| `Ctrl+Shift+A` | Toggle Agent Panel |
| `Ctrl+Shift+S` | Stop Agent Task |
| `Ctrl+Shift+P` | Toggle Privacy Mode |
| `Ctrl+,` | Open Settings |
| `Escape` | Cancel Current Task |

## 🔒 Privacy Features

Atlas Browser includes built-in privacy protection:

- **Tracker Blocking**: Blocks 50+ known trackers by default
- **Fingerprint Protection**: Prevents browser fingerprinting
- **No Telemetry**: Your data stays on your device

View blocked trackers: Click the 🔒 button in the toolbar

## 🛠️ Troubleshooting

### Agent not responding?
1. Check your API key in settings
2. Verify internet connection
3. Try a different LLM provider

### Page not loading?
1. Check if tracker blocking is too aggressive
2. Add the site to the allowed list in settings

### Build errors?
```bash
# Clean and rebuild
make clean
make build
```

## 📚 Learn More

- [Full Documentation](https://atlasbrowser.com/docs)
- [API Reference](API.md)
- [Architecture Guide](ARCHITECTURE.md)
- [Contributing Guide](CONTRIBUTING.md)

## 💬 Get Help

- **GitHub Issues**: Report bugs and request features
- **Discord**: Join our community
- **Email**: support@atlasbrowser.com

---

Happy browsing with Atlas! 🌐🤖
