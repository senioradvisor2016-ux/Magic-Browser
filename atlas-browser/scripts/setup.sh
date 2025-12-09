#!/bin/bash
# =============================================================================
# Atlas Browser Development Setup Script
# =============================================================================
#
# This script sets up the development environment for Atlas Browser.
# It handles:
#   1. Installing depot_tools
#   2. Fetching Chromium source
#   3. Setting up Atlas Browser symlinks
#   4. Running initial build
#
# Usage:
#   ./scripts/setup.sh [--skip-fetch] [--skip-build]
#
# =============================================================================

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Configuration
SKIP_FETCH=false
SKIP_BUILD=false
CHROMIUM_DIR="${CHROMIUM_DIR:-$HOME/chromium}"

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --skip-fetch)
            SKIP_FETCH=true
            shift
            ;;
        --skip-build)
            SKIP_BUILD=true
            shift
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            exit 1
            ;;
    esac
done

log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

check_system() {
    log_info "Checking system requirements..."

    # Check OS
    OS=$(uname -s)
    case $OS in
        Linux)
            log_info "Detected Linux"
            ;;
        Darwin)
            log_info "Detected macOS"
            ;;
        *)
            log_error "Unsupported OS: $OS"
            exit 1
            ;;
    esac

    # Check disk space (need ~100GB)
    AVAILABLE=$(df -BG "$HOME" | awk 'NR==2 {print $4}' | tr -d 'G')
    if [ "$AVAILABLE" -lt 100 ]; then
        log_warning "Low disk space: ${AVAILABLE}GB available, 100GB+ recommended"
    fi

    # Check RAM (need ~16GB)
    if [ "$OS" == "Linux" ]; then
        RAM=$(free -g | awk '/Mem:/ {print $2}')
    else
        RAM=$(sysctl -n hw.memsize | awk '{print int($1/1024/1024/1024)}')
    fi
    if [ "$RAM" -lt 16 ]; then
        log_warning "Low RAM: ${RAM}GB available, 16GB+ recommended"
    fi

    log_success "System check complete"
}

install_depot_tools() {
    log_info "Setting up depot_tools..."

    DEPOT_TOOLS_DIR="$HOME/depot_tools"

    if [ -d "$DEPOT_TOOLS_DIR" ]; then
        log_info "depot_tools already exists, updating..."
        cd "$DEPOT_TOOLS_DIR"
        git pull
    else
        log_info "Cloning depot_tools..."
        git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git "$DEPOT_TOOLS_DIR"
    fi

    # Add to PATH for this session
    export PATH="$DEPOT_TOOLS_DIR:$PATH"

    # Add to shell config
    SHELL_RC=""
    if [ -f "$HOME/.zshrc" ]; then
        SHELL_RC="$HOME/.zshrc"
    elif [ -f "$HOME/.bashrc" ]; then
        SHELL_RC="$HOME/.bashrc"
    fi

    if [ -n "$SHELL_RC" ]; then
        if ! grep -q "depot_tools" "$SHELL_RC"; then
            echo "" >> "$SHELL_RC"
            echo "# Chromium depot_tools" >> "$SHELL_RC"
            echo "export PATH=\"\$HOME/depot_tools:\$PATH\"" >> "$SHELL_RC"
            log_info "Added depot_tools to $SHELL_RC"
        fi
    fi

    log_success "depot_tools ready"
}

install_dependencies() {
    log_info "Installing build dependencies..."

    OS=$(uname -s)

    if [ "$OS" == "Linux" ]; then
        # Ubuntu/Debian
        if command -v apt-get &> /dev/null; then
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
        fi
    elif [ "$OS" == "Darwin" ]; then
        # macOS
        if ! command -v xcode-select &> /dev/null; then
            xcode-select --install
        fi
    fi

    log_success "Dependencies installed"
}

fetch_chromium() {
    if $SKIP_FETCH; then
        log_info "Skipping Chromium fetch"
        return
    fi

    log_info "Fetching Chromium source (this will take a while)..."

    mkdir -p "$CHROMIUM_DIR"
    cd "$CHROMIUM_DIR"

    if [ -d "src" ]; then
        log_info "Chromium source exists, syncing..."
        cd src
        gclient sync
    else
        log_info "Fetching Chromium (30+ GB download)..."
        fetch --nohooks --no-history chromium
        cd src
        gclient runhooks
    fi

    log_success "Chromium source ready"
}

setup_atlas() {
    log_info "Setting up Atlas Browser..."

    SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
    PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

    # Create symlink
    ATLAS_LINK="$CHROMIUM_DIR/src/atlas-browser"
    if [ -L "$ATLAS_LINK" ]; then
        rm "$ATLAS_LINK"
    fi
    ln -sf "$PROJECT_DIR" "$ATLAS_LINK"

    log_success "Atlas Browser linked to Chromium"
}

initial_build() {
    if $SKIP_BUILD; then
        log_info "Skipping initial build"
        return
    fi

    log_info "Running initial build..."

    SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
    "$SCRIPT_DIR/build.sh" --debug

    log_success "Initial build complete"
}

print_next_steps() {
    echo ""
    echo "=============================================="
    echo "         Setup Complete!"
    echo "=============================================="
    echo ""
    echo "Next steps:"
    echo ""
    echo "1. Reload your shell or run:"
    echo "   source ~/.bashrc  (or ~/.zshrc)"
    echo ""
    echo "2. Build Atlas Browser:"
    echo "   cd atlas-browser"
    echo "   ./scripts/build.sh"
    echo ""
    echo "3. Run Atlas Browser:"
    echo "   ~/chromium/src/out/AtlasDebug/atlas"
    echo ""
    echo "4. For release build:"
    echo "   ./scripts/build.sh --release"
    echo ""
}

main() {
    echo ""
    echo "=============================================="
    echo "    Atlas Browser Development Setup"
    echo "=============================================="
    echo ""

    check_system
    install_depot_tools
    install_dependencies
    fetch_chromium
    setup_atlas
    initial_build
    print_next_steps
}

main
