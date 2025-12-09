#!/bin/bash
# =============================================================================
# Atlas Browser Packaging Script
# =============================================================================
#
# Creates distributable packages for Atlas Browser.
#
# Usage:
#   ./scripts/package.sh [--platform linux|mac|win] [--version X.Y.Z]
#
# =============================================================================

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Defaults
PLATFORM=$(uname -s | tr '[:upper:]' '[:lower:]')
VERSION="${VERSION:-1.0.0}"
CHROMIUM_SRC="${CHROMIUM_SRC:-$HOME/chromium/src}"
BUILD_DIR="$CHROMIUM_SRC/out/AtlasRelease"

# Script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
DIST_DIR="$PROJECT_DIR/dist"

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --platform)
            PLATFORM="$2"
            shift 2
            ;;
        --version)
            VERSION="$2"
            shift 2
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

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

package_linux() {
    log_info "Packaging for Linux..."

    ARCH=$(uname -m)
    PACKAGE_NAME="atlas-browser-${VERSION}-${ARCH}"

    # Create package directory
    PACKAGE_DIR="$DIST_DIR/$PACKAGE_NAME"
    mkdir -p "$PACKAGE_DIR"

    # Copy binary and resources
    cp "$BUILD_DIR/atlas" "$PACKAGE_DIR/"
    cp -r "$BUILD_DIR/locales" "$PACKAGE_DIR/"
    cp "$BUILD_DIR"/*.pak "$PACKAGE_DIR/" 2>/dev/null || true
    cp "$BUILD_DIR"/*.dat "$PACKAGE_DIR/" 2>/dev/null || true
    cp "$BUILD_DIR/icudtl.dat" "$PACKAGE_DIR/" 2>/dev/null || true

    # Copy resources
    if [ -d "$BUILD_DIR/resources" ]; then
        cp -r "$BUILD_DIR/resources" "$PACKAGE_DIR/"
    fi

    # Create .desktop file
    cat > "$PACKAGE_DIR/atlas-browser.desktop" << EOF
[Desktop Entry]
Name=Atlas Browser
Comment=AI-powered browser with built-in agent
Exec=/opt/atlas-browser/atlas %U
Icon=atlas-browser
Type=Application
Categories=Network;WebBrowser;
MimeType=text/html;text/xml;application/xhtml+xml;x-scheme-handler/http;x-scheme-handler/https;
EOF

    # Create tarball
    cd "$DIST_DIR"
    tar -czvf "${PACKAGE_NAME}.tar.gz" "$PACKAGE_NAME"

    # Create .deb package
    create_deb_package "$PACKAGE_NAME"

    log_success "Linux packages created"
}

create_deb_package() {
    local PACKAGE_NAME=$1
    local DEB_DIR="$DIST_DIR/deb"

    mkdir -p "$DEB_DIR/DEBIAN"
    mkdir -p "$DEB_DIR/opt/atlas-browser"
    mkdir -p "$DEB_DIR/usr/share/applications"
    mkdir -p "$DEB_DIR/usr/share/icons/hicolor/256x256/apps"

    # Copy files
    cp -r "$DIST_DIR/$PACKAGE_NAME"/* "$DEB_DIR/opt/atlas-browser/"
    cp "$DIST_DIR/$PACKAGE_NAME/atlas-browser.desktop" "$DEB_DIR/usr/share/applications/"

    # Control file
    cat > "$DEB_DIR/DEBIAN/control" << EOF
Package: atlas-browser
Version: $VERSION
Section: web
Priority: optional
Architecture: amd64
Maintainer: Atlas Software <support@atlasbrowser.com>
Description: AI-powered browser with built-in agent
 Atlas Browser is a Chromium-based browser with integrated
 AI agent capabilities for browser automation.
Depends: libnss3, libgtk-3-0, libglib2.0-0, libpango-1.0-0
EOF

    # Build .deb
    dpkg-deb --build "$DEB_DIR" "$DIST_DIR/atlas-browser_${VERSION}_amd64.deb"

    # Cleanup
    rm -rf "$DEB_DIR"
}

package_mac() {
    log_info "Packaging for macOS..."

    APP_NAME="Atlas Browser.app"
    DMG_NAME="AtlasBrowser-${VERSION}.dmg"

    # Create app bundle
    APP_DIR="$DIST_DIR/$APP_NAME"
    mkdir -p "$APP_DIR/Contents/MacOS"
    mkdir -p "$APP_DIR/Contents/Resources"
    mkdir -p "$APP_DIR/Contents/Frameworks"

    # Copy binary
    cp "$BUILD_DIR/Atlas Browser.app/Contents/MacOS/"* "$APP_DIR/Contents/MacOS/"
    cp -r "$BUILD_DIR/Atlas Browser.app/Contents/Frameworks/"* "$APP_DIR/Contents/Frameworks/" 2>/dev/null || true
    cp -r "$BUILD_DIR/Atlas Browser.app/Contents/Resources/"* "$APP_DIR/Contents/Resources/" 2>/dev/null || true

    # Create Info.plist
    cat > "$APP_DIR/Contents/Info.plist" << EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleName</key>
    <string>Atlas Browser</string>
    <key>CFBundleDisplayName</key>
    <string>Atlas Browser</string>
    <key>CFBundleIdentifier</key>
    <string>com.atlas.browser</string>
    <key>CFBundleVersion</key>
    <string>$VERSION</string>
    <key>CFBundleShortVersionString</key>
    <string>$VERSION</string>
    <key>CFBundleExecutable</key>
    <string>atlas</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>LSMinimumSystemVersion</key>
    <string>10.15</string>
    <key>NSHighResolutionCapable</key>
    <true/>
</dict>
</plist>
EOF

    # Code sign (if identity available)
    if [ -n "$APPLE_SIGNING_IDENTITY" ]; then
        log_info "Code signing app bundle..."
        codesign --force --deep --sign "$APPLE_SIGNING_IDENTITY" "$APP_DIR"
    else
        log_info "Skipping code signing (no APPLE_SIGNING_IDENTITY set)"
    fi

    # Create DMG
    log_info "Creating DMG..."
    hdiutil create -volname "Atlas Browser" \
        -srcfolder "$APP_DIR" \
        -ov -format UDZO \
        "$DIST_DIR/$DMG_NAME"

    log_success "macOS package created: $DMG_NAME"
}

main() {
    echo ""
    echo "=============================================="
    echo "    Atlas Browser Packaging"
    echo "=============================================="
    echo ""
    echo "Platform:  $PLATFORM"
    echo "Version:   $VERSION"
    echo ""

    # Check build exists
    if [ ! -f "$BUILD_DIR/atlas" ] && [ ! -d "$BUILD_DIR/Atlas Browser.app" ]; then
        log_error "Release build not found. Run: ./scripts/build.sh --release"
        exit 1
    fi

    # Create dist directory
    mkdir -p "$DIST_DIR"

    case $PLATFORM in
        linux)
            package_linux
            ;;
        darwin|mac)
            package_mac
            ;;
        *)
            log_error "Unsupported platform: $PLATFORM"
            exit 1
            ;;
    esac

    echo ""
    log_success "Packaging complete!"
    echo "Packages available in: $DIST_DIR"
    echo ""
}

main
