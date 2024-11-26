#!/bin/bash
# create-icon.sh

# Create necessary directories
mkdir -p packaging/resources/icons.iconset

# You'll need to create a 1024x1024 PNG file first named icon_1024.png
# Place it in the packaging/resources directory
# Then this script will generate all required sizes

# Generate different icon sizes
sips -z 16 16 packaging/resources/icon_1024.png --out packaging/resources/icons.iconset/icon_16x16.png
sips -z 32 32 packaging/resources/icon_1024.png --out packaging/resources/icons.iconset/icon_16x16@2x.png
sips -z 32 32 packaging/resources/icon_1024.png --out packaging/resources/icons.iconset/icon_32x32.png
sips -z 64 64 packaging/resources/icon_1024.png --out packaging/resources/icons.iconset/icon_32x32@2x.png
sips -z 128 128 packaging/resources/icon_1024.png --out packaging/resources/icons.iconset/icon_128x128.png
sips -z 256 256 packaging/resources/icon_1024.png --out packaging/resources/icons.iconset/icon_128x128@2x.png
sips -z 256 256 packaging/resources/icon_1024.png --out packaging/resources/icons.iconset/icon_256x256.png
sips -z 512 512 packaging/resources/icon_1024.png --out packaging/resources/icons.iconset/icon_256x256@2x.png
sips -z 512 512 packaging/resources/icon_1024.png --out packaging/resources/icons.iconset/icon_512x512.png
sips -z 1024 1024 packaging/resources/icon_1024.png --out packaging/resources/icons.iconset/icon_512x512@2x.png

# Convert to icns file
iconutil -c icns packaging/resources/icons.iconset -o packaging/resources/SecureViewer.icns

# Clean up the iconset directory
rm -rf packaging/resources/icons.iconset
