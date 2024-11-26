#!/bin/bash

# SecureViewer Package Build Script
# This script creates a distributable .pkg installer for SecureViewer

# Exit on error
set -e

# Configuration
APP_NAME="SecureViewer"
APP_VERSION="1.0"
BUNDLE_ID="com.secureviewer.app"
QT_VERSION=$(brew list --versions qt | cut -d' ' -f2)
INSTALL_LOCATION="/Applications"
BUILD_DIR="build"
STAGING_DIR="staging"
PACKAGING_DIR="packaging"
SCRIPTS_DIR="$PACKAGING_DIR/scripts"
RESOURCES_DIR="$PACKAGING_DIR/resources"
QT_FRAMEWORKS_DIR="$(brew --cellar qt)/$QT_VERSION/lib"

# Create necessary directories
mkdir -p "$PACKAGING_DIR"/{scripts,resources,build}
mkdir -p "$STAGING_DIR"

# Create preinstall script
cat >"$SCRIPTS_DIR/preinstall" <<'EOF'
#!/bin/bash
# Remove previous installation if it exists
if [ -d "/Applications/SecureViewer.app" ]; then
    rm -rf "/Applications/SecureViewer.app"
fi
exit 0
EOF

# Create postinstall script
cat >"$SCRIPTS_DIR/postinstall" <<'EOF'
#!/bin/bash
# Fix permissions
chmod -R u+w,go+r-w "/Applications/SecureViewer.app"
chmod +x "/Applications/SecureViewer.app/Contents/MacOS/SecureViewer"
chmod +x "/Applications/SecureViewer.app/Contents/MacOS/bin/senc"

# Update dynamic linker paths for Qt frameworks
/usr/bin/install_name_tool -add_rpath "@executable_path/../Frameworks" "/Applications/SecureViewer.app/Contents/MacOS/SecureViewer"
exit 0
EOF

# Set execute permissions for scripts
chmod +x "$SCRIPTS_DIR"/{preinstall,postinstall}

# Function to copy and fix Qt framework
copy_qt_framework() {
  local framework=$1
  local frameworks_dir="$STAGING_DIR/SecureViewer.app/Contents/Frameworks"

  # Create Frameworks directory if it doesn't exist
  mkdir -p "$frameworks_dir"

  # Copy framework
  cp -R "$QT_FRAMEWORKS_DIR/$framework.framework" "$frameworks_dir/"

  # Remove unnecessary files (debug, headers, etc.)
  find "$frameworks_dir/$framework.framework" -name '*_debug*' -delete
  rm -rf "$frameworks_dir/$framework.framework/Headers"
  rm -rf "$frameworks_dir/$framework.framework/Versions/Current/Headers"

  # Fix framework ID and dependencies
  install_name_tool -id "@rpath/$framework.framework/Versions/Current/$framework" \
    "$frameworks_dir/$framework.framework/Versions/Current/$framework"
}

# Clean any existing build
make clean

# Build the application using the existing Makefile
make all

# Create clean staging area
rm -rf "$STAGING_DIR"
mkdir -p "$STAGING_DIR"

# Copy only the .app bundle to staging
cp -R "$BUILD_DIR/SecureViewer.app" "$STAGING_DIR/"

# Copy Qt frameworks
copy_qt_framework "QtCore"
copy_qt_framework "QtGui"
copy_qt_framework "QtWidgets"
copy_qt_framework "QtMultimedia"
copy_qt_framework "QtMultimediaWidgets"

# Create component pkg for the application
pkgbuild \
  --root "$STAGING_DIR" \
  --scripts "$SCRIPTS_DIR" \
  --identifier "$BUNDLE_ID" \
  --version "$APP_VERSION" \
  --install-location "$INSTALL_LOCATION" \
  "$PACKAGING_DIR/build/$APP_NAME.pkg"

# Create distribution file
cat >"$PACKAGING_DIR/build/distribution.xml" <<EOF
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="1">
    <title>$APP_NAME</title>
    <background file="background.png" alignment="bottomleft" scaling="none"/>
    <welcome file="welcome.html"/>
    <conclusion file="conclusion.html"/>
    <license file="license.txt"/>
    <options customize="never" require-scripts="true" rootVolumeOnly="true"/>
    <allowed-os-versions>
        <os-version min="10.15"/>
    </allowed-os-versions>
    <domains enable_anywhere="false" enable_currentUserHome="false" enable_localSystem="true"/>
    <pkg-ref id="$BUNDLE_ID"
             version="$APP_VERSION"
             auth="root">$APP_NAME.pkg</pkg-ref>
    <choices-outline>
        <line choice="default">
            <line choice="$BUNDLE_ID"/>
        </line>
    </choices-outline>
    <choice id="default"/>
    <choice id="$BUNDLE_ID" visible="false">
        <pkg-ref id="$BUNDLE_ID"/>
    </choice>
</installer-gui-script>
EOF

# Create welcome.html
cat >"$RESOURCES_DIR/welcome.html" <<EOF
<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <title>Welcome to $APP_NAME, Alexia :)</title>
</head>
<body>
    <h1>Welcome to $APP_NAME $APP_VERSION</h1>
    <p>This installer will guide you through the installation of $APP_NAME.</p>
</body>
</html>
EOF

# Create conclusion.html
cat >"$RESOURCES_DIR/conclusion.html" <<EOF
<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <title>Installation Complete</title>
</head>
<body>
    <h1>Installation Complete</h1>
    <p>$APP_NAME has been successfully installed.</p>
</body>
</html>
EOF

# Create license.txt (replace with your actual license)
cat >"$RESOURCES_DIR/license.txt" <<EOF
Copyright (c) $(date +%Y)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
EOF

# Create the final product installer
productbuild \
  --distribution "$PACKAGING_DIR/build/distribution.xml" \
  --resources "$RESOURCES_DIR" \
  --package-path "$PACKAGING_DIR/build" \
  "$APP_NAME-$APP_VERSION.pkg"

# Clean up staging directory
rm -rf "$STAGING_DIR"

echo "Package created successfully: $APP_NAME-$APP_VERSION.pkg"
