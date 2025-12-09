#!/bin/bash
# =============================================================================
# Atlas Browser Build Script
# =============================================================================
#
# Usage:
#   ./scripts/build.sh [options]
#
# Options:
#   --release     Build release version (default)
#   --debug       Build debug version
#   --clean       Clean build directory before building
#   --jobs N      Number of parallel jobs (default: auto)
#   --platform X  Target platform (linux, mac, win)
#   --help        Show this help message
#
# =============================================================================

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Default values
BUILD_TYPE="Release"
CLEAN=false
JOBS=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
PLATFORM=$(uname -s | tr '[:upper:]' '[:lower:]')

# Script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
CHROMIUM_SRC="${CHROMIUM_SRC:-$HOME/chromium/src}"

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --release)
            BUILD_TYPE="Release"
            shift
            ;;
        --debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        --clean)
            CLEAN=true
            shift
            ;;
        --jobs)
            JOBS="$2"
            shift 2
            ;;
        --platform)
            PLATFORM="$2"
            shift 2
            ;;
        --help)
            head -30 "$0" | tail -25
            exit 0
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            exit 1
            ;;
    esac
done

# Functions
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

check_prerequisites() {
    log_info "Checking prerequisites..."

    # Check for depot_tools
    if ! command -v gn &> /dev/null; then
        log_error "gn not found. Please install depot_tools."
        log_info "Run: git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git"
        log_info "Add to PATH: export PATH=\"\$HOME/depot_tools:\$PATH\""
        exit 1
    fi

    # Check for autoninja
    if ! command -v autoninja &> /dev/null; then
        log_error "autoninja not found. Please install depot_tools."
        exit 1
    fi

    # Check for Chromium source
    if [ ! -d "$CHROMIUM_SRC" ]; then
        log_error "Chromium source not found at $CHROMIUM_SRC"
        log_info "Set CHROMIUM_SRC environment variable or fetch Chromium:"
        log_info "  mkdir ~/chromium && cd ~/chromium"
        log_info "  fetch --nohooks chromium"
        exit 1
    fi

    log_success "Prerequisites OK"
}

setup_symlinks() {
    log_info "Setting up Atlas Browser symlinks..."

    # Create symlink in Chromium src
    ATLAS_LINK="$CHROMIUM_SRC/atlas-browser"
    if [ -L "$ATLAS_LINK" ]; then
        rm "$ATLAS_LINK"
    fi
    ln -sf "$PROJECT_DIR" "$ATLAS_LINK"

    log_success "Symlinks created"
}

generate_build_files() {
    log_info "Generating build files for $BUILD_TYPE..."

    BUILD_DIR="$CHROMIUM_SRC/out/Atlas$BUILD_TYPE"

    # Create args.gn
    mkdir -p "$BUILD_DIR"

    if [ "$BUILD_TYPE" == "Debug" ]; then
        cat > "$BUILD_DIR/args.gn" << EOF
# Atlas Browser Debug Build
import("//atlas-browser/args.gn")

is_debug = true
is_component_build = true
symbol_level = 1
dcheck_always_on = true
EOF
    else
        cat > "$BUILD_DIR/args.gn" << EOF
# Atlas Browser Release Build
import("//atlas-browser/args.gn")

is_debug = false
is_component_build = false
is_official_build = true
symbol_level = 0
EOF
    fi

    # Platform-specific args
    case $PLATFORM in
        darwin)
            echo "target_os = \"mac\"" >> "$BUILD_DIR/args.gn"
            ;;
        linux)
            echo "target_os = \"linux\"" >> "$BUILD_DIR/args.gn"
            echo "use_sysroot = true" >> "$BUILD_DIR/args.gn"
            ;;
        *)
            log_warning "Unknown platform: $PLATFORM"
            ;;
    esac

    # Run GN
    cd "$CHROMIUM_SRC"
    gn gen "$BUILD_DIR"

    log_success "Build files generated"
}

build() {
    log_info "Building Atlas Browser ($BUILD_TYPE) with $JOBS jobs..."

    BUILD_DIR="$CHROMIUM_SRC/out/Atlas$BUILD_TYPE"
    cd "$CHROMIUM_SRC"

    if $CLEAN; then
        log_info "Cleaning build directory..."
        rm -rf "$BUILD_DIR"
        generate_build_files
    fi

    # Build
    START_TIME=$(date +%s)

    autoninja -C "$BUILD_DIR" atlas_browser -j "$JOBS"

    END_TIME=$(date +%s)
    DURATION=$((END_TIME - START_TIME))
    MINUTES=$((DURATION / 60))
    SECONDS=$((DURATION % 60))

    log_success "Build complete in ${MINUTES}m ${SECONDS}s"
    log_info "Binary location: $BUILD_DIR/atlas"
}

main() {
    echo ""
    echo "=============================================="
    echo "       Atlas Browser Build Script"
    echo "=============================================="
    echo ""
    echo "Build type:   $BUILD_TYPE"
    echo "Platform:     $PLATFORM"
    echo "Jobs:         $JOBS"
    echo "Chromium:     $CHROMIUM_SRC"
    echo ""

    check_prerequisites
    setup_symlinks
    generate_build_files
    build

    echo ""
    log_success "Atlas Browser built successfully!"
    echo ""
}

main
