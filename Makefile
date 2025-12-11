# Modern macOS Makefile for AppBundleGenerator
# Target: macOS 12+ (Monterey and later)

CC = clang
SDK_PATH = $(shell xcrun --show-sdk-path)
DEPLOYMENT_TARGET = 12.0

# Security and optimization flags
CFLAGS = -Wall -Wextra -Wpedantic \
         -Werror=deprecated-declarations \
         -O2 \
         -mmacosx-version-min=$(DEPLOYMENT_TARGET) \
         -isysroot $(SDK_PATH) \
         -fstack-protector-strong \
         -D_FORTIFY_SOURCE=2 \
         -I.

# Debug build flags
DEBUG_FLAGS = -g -DDEBUG -O0

# Link against CoreFoundation
LDFLAGS = -framework CoreFoundation \
          -mmacosx-version-min=$(DEPLOYMENT_TARGET) \
          -isysroot $(SDK_PATH)

# Source files
SOURCES = main.c appbundler.c icon_utils.c entitlements.c
HEADERS = shared.h
OBJECTS = $(SOURCES:.c=.o)
TARGET = AppBundleGenerator

# Default target
all: $(TARGET)

# Debug build
debug: CFLAGS += $(DEBUG_FLAGS)
debug: clean $(TARGET)

# Link executable
$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^
	@echo "Build complete: $(TARGET)"
	@echo "Target: macOS $(DEPLOYMENT_TARGET)+"

# Compile object files
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c -o $@ $<

# Clean build artifacts
clean:
	rm -f $(OBJECTS) $(TARGET)
	@echo "Clean complete"

# Install to /usr/local/bin (requires sudo)
install: $(TARGET)
	install -m 755 $(TARGET) /usr/local/bin/
	@echo "Installed to /usr/local/bin/$(TARGET)"

# Uninstall
uninstall:
	rm -f /usr/local/bin/$(TARGET)
	@echo "Uninstalled $(TARGET)"

# Check for deprecated APIs
check-deprecated:
	@echo "Checking for deprecated APIs..."
	@grep -n "CFPropertyListCreateXMLData\|CFStringGetSystemEncoding\|CFURLWriteDataAndPropertiesToResource" $(SOURCES) || echo "No deprecated APIs found"

# Show build info
info:
	@echo "Compiler: $(CC)"
	@echo "SDK: $(SDK_PATH)"
	@echo "Deployment target: macOS $(DEPLOYMENT_TARGET)"
	@echo "Sources: $(SOURCES)"

.PHONY: all debug clean install uninstall check-deprecated info
