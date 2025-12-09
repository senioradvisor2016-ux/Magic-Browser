# Changelog

All notable changes to Atlas Browser will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- Initial project structure and build configuration
- AI Agent system with LLM integration
  - Support for Anthropic Claude, OpenAI GPT-4, and local models (Ollama)
  - Visual understanding via screenshots
  - DOM analysis and element extraction
  - Action execution (click, type, scroll, hover, navigate)
- Privacy features
  - Tracker blocking with default blocklist
  - Fingerprint protection (planned)
- Custom UI components
  - Agent panel with task input and action log
  - Atlas toolbar with agent and privacy buttons
  - Status indicator for agent activity
- Settings system
  - Agent configuration (provider, model, API key)
  - Privacy settings (tracker blocking, blocked domains)
  - Update settings (auto-update, channel)
- Extension API (`atlas.agent` namespace)
  - `runTask()` - Execute AI-powered tasks
  - `queryPage()` - Ask questions about pages
  - `performAction()` - Execute individual actions
  - `getElements()` - Get interactive elements
- WebUI pages
  - Settings page (`atlas://settings`)
  - New Tab page with AI agent integration
- Auto-update system
- Keyboard shortcuts
  - `Ctrl+Shift+A` - Toggle agent panel
  - `Ctrl+Shift+S` - Stop agent task
  - `Ctrl+Shift+P` - Toggle privacy mode
- Documentation
  - Architecture documentation
  - Contributing guidelines
  - API documentation
- CI/CD workflows
  - Linux build (x64)
  - macOS build (ARM64)
  - Release automation

### Technical
- Based on Chromium with custom modifications
- Mojo IPC for browser-renderer communication
- GN/Ninja build system integration
- Unit tests for core components
- Browser tests for integration testing

---

## [1.0.0] - Planned

### Features
- Stable AI Agent with Claude Sonnet 4 support
- Full privacy protection suite
- Cross-platform support (Windows, macOS, Linux)
- Extension marketplace integration
- Sync support for settings

---

## Version History

| Version | Date | Notes |
|---------|------|-------|
| 0.1.0 | TBD | Initial alpha release |
| 1.0.0 | TBD | First stable release |

---

## Release Process

1. Update version in `branding/branding.gni`
2. Update this CHANGELOG
3. Create release commit: `git commit -m "Release vX.Y.Z"`
4. Tag release: `git tag vX.Y.Z`
5. Push with tags: `git push origin main --tags`
6. GitHub Actions will build and publish the release

---

## Links

- [GitHub Repository](https://github.com/atlas-browser/atlas-browser)
- [Issue Tracker](https://github.com/atlas-browser/atlas-browser/issues)
- [Documentation](https://atlasbrowser.com/docs)
