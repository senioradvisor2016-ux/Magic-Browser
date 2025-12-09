# =============================================================================
# Atlas Browser Makefile
# =============================================================================
#
# Simple wrapper around the build scripts for convenience.
# Use: make [target]
#

.PHONY: all setup build debug release test clean package help

# Default target
all: build

# Help
help:
	@echo "Atlas Browser Build System"
	@echo ""
	@echo "Usage: make [target]"
	@echo ""
	@echo "Targets:"
	@echo "  setup     - Set up development environment (downloads Chromium)"
	@echo "  build     - Build debug version"
	@echo "  debug     - Build debug version"
	@echo "  release   - Build release version"
	@echo "  test      - Run unit tests"
	@echo "  browsertest - Run browser tests"
	@echo "  clean     - Clean build outputs"
	@echo "  package   - Create distributable package"
	@echo "  run       - Run the browser (debug build)"
	@echo "  help      - Show this help message"
	@echo ""
	@echo "Examples:"
	@echo "  make setup    # First time setup"
	@echo "  make build    # Build debug version"
	@echo "  make run      # Run the browser"

# Setup development environment
setup:
	@echo "Setting up Atlas Browser development environment..."
	./scripts/setup.sh

# Build debug version
build: debug

debug:
	@echo "Building Atlas Browser (Debug)..."
	./scripts/build.sh --debug

# Build release version
release:
	@echo "Building Atlas Browser (Release)..."
	./scripts/build.sh --release

# Run unit tests
test:
	@echo "Running Atlas Browser unit tests..."
	./scripts/build.sh --debug --run-tests

# Run browser tests
browsertest:
	@echo "Running Atlas Browser integration tests..."
	@if [ -d "$(HOME)/chromium/src" ]; then \
		cd $(HOME)/chromium/src && \
		./out/AtlasDebug/atlas_browser_browsertests; \
	else \
		echo "Error: Chromium not found. Run 'make setup' first."; \
		exit 1; \
	fi

# Clean build outputs
clean:
	@echo "Cleaning Atlas Browser build outputs..."
	@if [ -d "$(HOME)/chromium/src/out/AtlasDebug" ]; then \
		rm -rf $(HOME)/chromium/src/out/AtlasDebug; \
	fi
	@if [ -d "$(HOME)/chromium/src/out/AtlasRelease" ]; then \
		rm -rf $(HOME)/chromium/src/out/AtlasRelease; \
	fi
	@echo "Clean complete."

# Create distributable package
package:
	@echo "Creating Atlas Browser package..."
	./scripts/package.sh

# Run the browser (debug build)
run:
	@echo "Starting Atlas Browser..."
	@if [ -f "$(HOME)/chromium/src/out/AtlasDebug/atlas" ]; then \
		$(HOME)/chromium/src/out/AtlasDebug/atlas; \
	elif [ -f "$(HOME)/chromium/src/out/AtlasDebug/Atlas Browser.app/Contents/MacOS/Atlas Browser" ]; then \
		open "$(HOME)/chromium/src/out/AtlasDebug/Atlas Browser.app"; \
	else \
		echo "Error: Atlas Browser not built. Run 'make build' first."; \
		exit 1; \
	fi

# Quick rebuild (incremental)
rebuild:
	@echo "Rebuilding Atlas Browser (incremental)..."
	@if [ -d "$(HOME)/chromium/src" ]; then \
		cd $(HOME)/chromium/src && \
		autoninja -C out/AtlasDebug atlas; \
	else \
		echo "Error: Chromium not found. Run 'make setup' first."; \
		exit 1; \
	fi

# Format code
format:
	@echo "Formatting code..."
	@find src -name "*.cc" -o -name "*.h" | xargs clang-format -i
	@echo "Done."

# Check code style
check:
	@echo "Checking code style..."
	@find src -name "*.cc" -o -name "*.h" | xargs clang-format --dry-run --Werror
	@echo "Style check passed."

# Generate compile_commands.json for IDE support
compile-commands:
	@echo "Generating compile_commands.json..."
	@if [ -d "$(HOME)/chromium/src" ]; then \
		cd $(HOME)/chromium/src && \
		gn gen out/AtlasDebug --export-compile-commands; \
		ln -sf $(HOME)/chromium/src/out/AtlasDebug/compile_commands.json .; \
	fi
	@echo "Done."
