# Makefile for SecureViewer Application

# Compiler and flags
CXX := clang++
CXXFLAGS := -std=c++17 -Wall -Wextra -Wpedantic -O2
DEFINES := -DQT_NO_DEBUG -DQT_WIDGETS_LIB -DQT_GUI_LIB -DQT_CORE_LIB

# Qt specific setup
QTDIR := $(shell brew --cellar qt)/$(shell brew list --versions qt | cut -d' ' -f2)
MOC := $(QTDIR)/share/qt/libexec/moc

# Qt includes
QTINC := -I$(QTDIR)/include \
         -I$(QTDIR)/include/QtCore \
         -I$(QTDIR)/include/QtGui \
         -I$(QTDIR)/include/QtWidgets \
         -I$(QTDIR)/include/QtMultimedia \
         -I$(QTDIR)/include/QtMultimediaWidgets \
         -I$(QTDIR)/include/QtPdf \
         -I$(QTDIR)/include/QtPdfWidgets

# Framework paths and libraries
QTLIB := -F$(QTDIR)/lib \
         -framework QtWidgets \
         -framework QtGui \
         -framework QtCore \
         -framework QtMultimedia \
         -framework QtMultimediaWidgets \
         -framework QtPdf \
         -framework QtPdfWidgets

# Include paths
INCLUDES := $(QTINC) \
            -I. \
            -I./build \
            -I$(QTDIR)/mkspecs/macx-clang

# Library paths
LIBS := $(QTLIB) \
        -framework AppKit \
        -framework CoreFoundation

# Source files
SOURCES := SecureViewer.cpp
MOC_HEADERS := SecureViewer.h
MOC_SOURCES := $(MOC_HEADERS:%.h=build/moc_%.cpp)

# Object files
OBJECTS := $(SOURCES:%.cpp=build/%.o) $(MOC_SOURCES:%.cpp=%.o)

# Target executable
TARGET := build/SecureViewer.app/Contents/MacOS/SecureViewer

# Default target
all: dirs $(TARGET)

# Create necessary directories
dirs:
	@mkdir -p build
	@mkdir -p build/SecureViewer.app/Contents/MacOS
	@mkdir -p build/SecureViewer.app/Contents/Resources
	@mkdir -p build/SecureViewer.app/Contents/MacOS/bin
	@$(MAKE) info-plist
	@cp bin/senc build/SecureViewer.app/Contents/MacOS/bin/
	@chmod +x build/SecureViewer.app/Contents/MacOS/bin/senc

# Generate Info.plist
info-plist:
	@echo '<?xml version="1.0" encoding="UTF-8"?>' > build/SecureViewer.app/Contents/Info.plist
	@echo '<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">' >> build/SecureViewer.app/Contents/Info.plist
	@echo '<plist version="1.0">' >> build/SecureViewer.app/Contents/Info.plist
	@echo '<dict>' >> build/SecureViewer.app/Contents/Info.plist
	@echo '    <key>CFBundleExecutable</key>' >> build/SecureViewer.app/Contents/Info.plist
	@echo '    <string>SecureViewer</string>' >> build/SecureViewer.app/Contents/Info.plist
	@echo '    <key>CFBundleIdentifier</key>' >> build/SecureViewer.app/Contents/Info.plist
	@echo '    <string>com.secureviewer.app</string>' >> build/SecureViewer.app/Contents/Info.plist
	@echo '    <key>CFBundleName</key>' >> build/SecureViewer.app/Contents/Info.plist
	@echo '    <string>SecureViewer</string>' >> build/SecureViewer.app/Contents/Info.plist
	@echo '    <key>CFBundlePackageType</key>' >> build/SecureViewer.app/Contents/Info.plist
	@echo '    <string>APPL</string>' >> build/SecureViewer.app/Contents/Info.plist
	@echo '    <key>CFBundleShortVersionString</key>' >> build/SecureViewer.app/Contents/Info.plist
	@echo '    <string>1.0</string>' >> build/SecureViewer.app/Contents/Info.plist
	@echo '    <key>LSMinimumSystemVersion</key>' >> build/SecureViewer.app/Contents/Info.plist
	@echo '    <string>10.15</string>' >> build/SecureViewer.app/Contents/Info.plist
	@echo '    <key>CFBundleSupportedPlatforms</key>' >> build/SecureViewer.app/Contents/Info.plist
	@echo '    <array>' >> build/SecureViewer.app/Contents/Info.plist
	@echo '        <string>MacOSX</string>' >> build/SecureViewer.app/Contents/Info.plist
	@echo '    </array>' >> build/SecureViewer.app/Contents/Info.plist
	@echo '</dict>' >> build/SecureViewer.app/Contents/Info.plist
	@echo '</plist>' >> build/SecureViewer.app/Contents/Info.plist

# Generate moc source files
build/moc_%.cpp: %.h
	$(MOC) $(DEFINES) $(INCLUDES) $< -o $@

# Compile source files
build/%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(DEFINES) $(INCLUDES) -c $< -o $@

# Compile moc sources
build/moc_%.o: build/moc_%.cpp
	$(CXX) $(CXXFLAGS) $(DEFINES) $(INCLUDES) -c $< -o $@

# Link the application
$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) $(LIBS) -o $(TARGET)
	@echo "Build complete: $(TARGET)"

# Clean build files
clean:
	sudo rm -rf build/
	rm -rf /packaging/

# Install Qt dependencies using Homebrew
deps:
	@echo "Checking for Homebrew..."
	@which brew > /dev/null || (echo "Please install Homebrew first" && exit 1)
	@echo "Installing Qt..."
	brew install qt cmake ninja

# Show Qt paths (for debugging)
qtpaths:
	@echo "Qt directory: $(QTDIR)"
	@echo "moc location: $(MOC)"
	@ls -l $(MOC) 2>/dev/null || echo "moc not found at specified location"
	@echo "\nQt include directories:"
	@echo "$(QTINC)"
	@echo "\nFramework paths:"
	@echo "$(QTLIB)"

# Run the application
run: $(TARGET)
	@echo "Running SecureViewer..."
	@$(TARGET)

.PHONY: all clean deps run dirs info-plist qtpaths
