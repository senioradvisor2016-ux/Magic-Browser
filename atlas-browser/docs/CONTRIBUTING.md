# Contributing to Atlas Browser

Thank you for your interest in contributing to Atlas Browser! This document provides guidelines and information for contributors.

## 🚀 Getting Started

### Prerequisites

1. Read the [README](../README.md) and [ARCHITECTURE](ARCHITECTURE.md)
2. Set up the development environment
3. Familiarize yourself with Chromium development

### Development Setup

```bash
# Clone the repository
git clone https://github.com/atlas-browser/atlas-browser.git
cd atlas-browser

# Run setup script
./scripts/setup.sh

# Build debug version
./scripts/build.sh --debug
```

## 📝 Code Style

### C++ Style

We follow the [Chromium C++ Style Guide](https://chromium.googlesource.com/chromium/src/+/main/styleguide/c++/c++.md).

Key points:
- 2 spaces for indentation
- 80 character line limit
- Use `nullptr` not `NULL`
- Use `std::unique_ptr` for ownership
- Use `raw_ptr<T>` for non-owning pointers in classes

```cpp
// Good
class MyClass {
 public:
  MyClass();
  ~MyClass();
  
  void DoSomething();

 private:
  std::unique_ptr<Helper> helper_;
  raw_ptr<Observer> observer_;
};

// Bad
class MyClass {
public:
    MyClass();
    void DoSomething();
private:
    Helper* helper;  // Unclear ownership
};
```

### Formatting

Use `git cl format` to auto-format before committing:

```bash
git cl format
```

### Naming Conventions

| Type | Convention | Example |
|------|------------|---------|
| Classes | PascalCase | `AgentService` |
| Functions | PascalCase | `GetActiveTab()` |
| Variables | snake_case | `task_id` |
| Constants | kPascalCase | `kMaxSteps` |
| Member variables | snake_case_ | `browser_context_` |
| Namespaces | lowercase | `atlas` |

## 🔧 Making Changes

### Workflow

1. **Fork** the repository
2. **Create a branch** for your feature/fix
3. **Make changes** with clear commits
4. **Test** your changes
5. **Submit a Pull Request**

### Branch Naming

```
feature/agent-multi-tab
fix/memory-leak-screenshot
docs/update-readme
refactor/llm-client
```

### Commit Messages

Follow conventional commits:

```
feat(agent): add support for form filling

- Implement FillForm() in AgentInjector
- Add form field detection to DOMExtractor
- Update Mojo interface with new method

Fixes #123
```

Types:
- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation
- `refactor`: Code refactoring
- `test`: Adding tests
- `chore`: Maintenance

## 🧪 Testing

### Running Tests

```bash
# Build tests
autoninja -C out/AtlasDebug atlas_browser_unittests

# Run unit tests
./out/AtlasDebug/atlas_browser_unittests

# Run specific test
./out/AtlasDebug/atlas_browser_unittests --gtest_filter="AgentServiceTest.*"

# Run browser tests
autoninja -C out/AtlasDebug atlas_browser_browsertests
./out/AtlasDebug/atlas_browser_browsertests
```

### Writing Tests

```cpp
// Unit test example
class AgentServiceTest : public testing::Test {
 protected:
  void SetUp() override {
    agent_service_ = std::make_unique<AgentService>(browser_context());
  }
  
  std::unique_ptr<AgentService> agent_service_;
};

TEST_F(AgentServiceTest, StartTask_CreatesTaskWithCorrectId) {
  auto task_id = agent_service_->StartTask(
      web_contents(), "test task", base::DoNothing());
  
  EXPECT_FALSE(task_id.empty());
  EXPECT_EQ(agent_service_->GetTaskStatus(task_id),
            agent::mojom::TaskStatus::kPending);
}
```

## 📋 Pull Request Process

### Before Submitting

- [ ] Code follows style guide
- [ ] Tests pass locally
- [ ] New code has tests
- [ ] Documentation updated
- [ ] Commit messages are clear

### PR Description Template

```markdown
## Description
Brief description of changes.

## Type of Change
- [ ] Bug fix
- [ ] New feature
- [ ] Breaking change
- [ ] Documentation update

## Testing
How was this tested?

## Screenshots (if applicable)

## Related Issues
Fixes #123
```

### Review Process

1. Automated checks must pass
2. At least one maintainer approval
3. All comments addressed
4. Squash and merge

## 🐛 Reporting Issues

### Bug Reports

Use the bug report template:

```markdown
**Describe the bug**
Clear description of the bug.

**To Reproduce**
1. Go to '...'
2. Click on '...'
3. See error

**Expected behavior**
What you expected to happen.

**Screenshots**
If applicable.

**Environment:**
- OS: [e.g., Ubuntu 22.04]
- Atlas Version: [e.g., 1.0.0]
- Chromium Version: [e.g., 128.0.6613.84]

**Additional context**
Any other information.
```

### Feature Requests

```markdown
**Is your feature request related to a problem?**
Description of the problem.

**Describe the solution you'd like**
What you want to happen.

**Describe alternatives you've considered**
Other solutions you've thought about.

**Additional context**
Any other information.
```

## 🏗️ Architecture Guidelines

### Adding New Features

1. **Design first**: Create an issue or discussion
2. **Start small**: MVP implementation
3. **Iterate**: Based on feedback

### Directory Structure

```
src/atlas_browser/
├── browser/          # Browser process code
│   └── ui/views/    # UI components
├── features/         # Feature modules
│   ├── llm_agent/   # AI agent feature
│   │   ├── browser/ # Browser-side
│   │   ├── renderer/# Renderer-side
│   │   └── public/  # Public interfaces
│   └── privacy/     # Privacy features
└── resources/        # Assets
```

### Adding a New Feature Module

1. Create directory under `features/`
2. Add `BUILD.gn`
3. Implement browser/renderer split if needed
4. Add Mojo interfaces if cross-process
5. Add tests
6. Update documentation

## 📚 Resources

### Chromium Development

- [Chromium Getting Started](https://chromium.googlesource.com/chromium/src/+/main/docs/get_the_code.md)
- [Chromium Design Documents](https://www.chromium.org/developers/design-documents/)
- [Mojo Documentation](https://chromium.googlesource.com/chromium/src/+/main/mojo/README.md)

### Tools

- [GN Build System](https://gn.googlesource.com/gn/+/main/docs/)
- [Ninja Build](https://ninja-build.org/)
- [depot_tools](https://commondatastorage.googleapis.com/chrome-infra-docs/flat/depot_tools/docs/html/depot_tools.html)

## 💬 Communication

- **GitHub Issues**: Bug reports and feature requests
- **GitHub Discussions**: Questions and ideas
- **Discord**: Real-time chat (coming soon)

## 📄 License

By contributing, you agree that your contributions will be licensed under the BSD 3-Clause License.

---

Thank you for contributing to Atlas Browser! 🎉
