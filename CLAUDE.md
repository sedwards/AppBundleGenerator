# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Overview

AppBundleGenerator is a macOS-specific C utility that creates macOS Application Bundles (.app) from executables or command strings. Version 2.0 has been modernized for macOS 12+ (Monterey and later) with full icon generation, code signing, and modern Info.plist support.

Originally developed for Wine integration on macOS, this tool enables wrapping command-line utilities or scripts as double-clickable macOS applications with proper icons and code signing.

## Build System

Build using modern Make with clang:

```bash
# Build the AppBundleGenerator executable
make

# Debug build with symbols and verbose output
make debug

# Clean build artifacts
make clean

# Install to /usr/local/bin (requires sudo)
sudo make install

# Check for deprecated APIs
make check-deprecated

# Show build information
make info
```

**Compiler:** clang (not gcc) with modern security flags:
- `-Wall -Wextra -Wpedantic` - All warnings enabled
- `-Werror=deprecated-declarations` - Treat deprecated APIs as errors
- `-fstack-protector-strong` - Stack protection
- `-D_FORTIFY_SOURCE=2` - Buffer overflow protection
- Target: macOS 12.0+ deployment

**Dependencies:**
- CoreFoundation framework (linked automatically)
- macOS utilities: `sips`, `iconutil`, `qlmanage` (for icon generation)

## Usage

### Basic Usage (Backward Compatible)

```bash
./AppBundleGenerator 'AppName' /Applications '/path/to/executable'
```

### Modern Usage with Options

```bash
./AppBundleGenerator [options] BundleName DestinationDir ExecutableOrCommand
```

### Common Examples

**1. Basic bundle:**
```bash
./AppBundleGenerator 'My App' /tmp '/usr/local/bin/myapp'
```

**2. With PNG icon:**
```bash
./AppBundleGenerator --icon icon.png 'My App' /tmp '/usr/local/bin/myapp'
```

**3. With SVG icon (auto-converts to ICNS):**
```bash
./AppBundleGenerator --icon icon.svg 'My App' /tmp '/usr/local/bin/myapp'
```

**4. Ad-hoc signed for development:**
```bash
./AppBundleGenerator --sign - --hardened-runtime 'My App' /tmp '/usr/local/bin/myapp'
```

**5. Production build with all features:**
```bash
./AppBundleGenerator \
  --icon app.svg \
  --sign 'Developer ID Application: Your Name' \
  --hardened-runtime \
  --identifier com.example.myapp \
  --category public.app-category.developer-tools \
  --min-os 12.0 \
  --version 2.0.0 \
  'My Development Tool' /Applications '/usr/local/bin/devtool'
```

### Command-Line Options

**Icon Options:**
- `--icon PATH` - Icon file (PNG, SVG, or ICNS format). Automatically converts PNG/SVG to .icns

**Code Signing Options:**
- `--sign IDENTITY` - Code signing identity (use `-` for ad-hoc signing)
- `--hardened-runtime` - Enable hardened runtime (recommended for distribution)
- `--entitlements PATH` - Custom entitlements plist file
- `--force-sign` - Replace existing signature

**Info.plist Options:**
- `--identifier ID` - Custom bundle identifier (default: auto-generated)
- `--min-os VERSION` - Minimum macOS version (default: 12.0)
- `--category TYPE` - App category (default: public.app-category.utilities)
- `--version VER` - Bundle version (default: 1.0.0)

**Entitlement Exceptions (for hardened runtime):**
- `--allow-jit` - Allow JIT compilation
- `--allow-unsigned` - Allow unsigned executable memory
- `--allow-dyld-vars` - Allow DYLD environment variables

## Architecture

### Core Components

**main.c** - CLI interface and orchestration (369 lines)
- Modern `getopt_long` argument parsing
- `parse_arguments()` - Comprehensive option parsing with validation
- `usage()` - Detailed help with examples
- `main()` - Orchestrates bundle creation, icon conversion, and code signing
- Error handling with `print_error()` and `error_code_to_string()`
- Helper utilities: `heap_printf()`, `create_directories()`

**appbundler.c** - Bundle generation engine (577 lines)
- `build_app_bundle()` - Main orchestrator accepting `AppBundleOptions` struct
- `CreateMyDictionary()` - Generates complete Info.plist with modern keys
- `generate_bundle_identifier()` - Auto-generates unique bundle IDs from app names
- `sanitize_bundle_name()` - Converts names to valid identifiers
- `generate_plist()` - Creates Info.plist using modern CoreFoundation APIs
- `generate_pkginfo_file()` - Creates PkgInfo file
- `generate_bundle_script()` - Creates executable shell wrapper
- `add_icns_for_bundle()` - Icon conversion dispatcher
- `codesign_bundle()` - Code signing with configurable options
- `verify_codesign()` - Signature verification
- Uses binary plist format for faster parsing (Info.plist)

**icon_utils.c** - Icon conversion pipeline (268 lines)
- `detect_icon_format()` - Detects PNG/SVG/ICNS by extension
- `copy_file()` - Simple file copy for .icns files
- `convert_png_to_icns()` - PNG → iconset → .icns pipeline
- `convert_svg_to_icns()` - SVG → PNG → iconset → .icns pipeline
- `generate_iconset_from_png()` - Creates all 10 required icon sizes
  - Sizes: 16, 32, 64, 128, 256, 512, 1024 (1x and 2x)
- Uses macOS utilities: `qlmanage`, `sips`, `iconutil`

**entitlements.c** - Entitlements generation (161 lines)
- `generate_entitlements_file()` - Creates entitlements plist for code signing
- Supports hardened runtime exceptions:
  - `com.apple.security.cs.allow-jit`
  - `com.apple.security.cs.allow-unsigned-executable-memory`
  - `com.apple.security.cs.allow-dyld-environment-variables`
  - `com.apple.security.cs.disable-library-validation`
- Uses XML format (required by codesign, not binary)

**shared.h** - Common definitions (106 lines)
- `AppBundleOptions` - Main configuration structure
- `CodeSignOptions` - Code signing configuration
- `IconFormat` - Icon type enumeration
- `ErrorCode` - Error reporting codes
- Function prototypes for all modules

### Bundle Structure

Generated .app follows standard macOS layout:

```
AppName.app/
  Contents/
    Info.plist          # Binary format, all modern keys
    PkgInfo             # Type code "APPL????"
    MacOS/
      AppName           # Executable shell script (chmod 755)
    Resources/
      icon.icns         # Converted icon (if provided)
      English.lproj/    # Localization directory (empty but required)
    _CodeSignature/     # Created by codesign (if signed)
      CodeResources
```

### Modern Info.plist Keys

All bundles include these modern macOS keys:

**Essential Keys:**
- `CFBundleExecutable` - Executable name
- `CFBundleIdentifier` - Dynamically generated (e.g., com.appbundlegenerator.my-app)
- `CFBundleName` - Display name
- `CFBundleDisplayName` - UI display name
- `CFBundleVersion` - Build version
- `CFBundleShortVersionString` - Marketing version
- `CFBundlePackageType` - "APPL"
- `CFBundleInfoDictionaryVersion` - "6.0"

**Modern macOS 12+ Keys:**
- `LSMinimumSystemVersion` - "12.0" (or custom via --min-os)
- `NSHighResolutionCapable` - true (Retina display support)
- `LSApplicationCategoryType` - App category for Gatekeeper
- `NSPrincipalClass` - "NSApplication"
- `NSSupportsAutomaticGraphicsSwitching` - true (GPU selection)
- `CFBundleDevelopmentRegion` - "en" (modern locale code)

**Icon:**
- `CFBundleIconFile` - "icon.icns"

### API Modernization (Version 2.0)

**Replaced Deprecated APIs:**
1. ~~`CFPropertyListCreateXMLData()`~~ → `CFPropertyListCreateData()` with format parameter
2. ~~`CFURLWriteDataAndPropertiesToResource()`~~ → `CFWriteStreamCreateWithFile()` + `CFWriteStreamWrite()`
3. ~~`CFStringGetSystemEncoding()`~~ → `kCFStringEncodingUTF8`

**Benefits:**
- No compiler warnings with `-Werror=deprecated-declarations`
- Binary plist format for faster parsing
- Proper error handling with `CFErrorRef`
- Future-proof for macOS updates

## Icon Generation Pipeline

### Supported Formats

1. **ICNS** - Direct copy to bundle
2. **PNG** - Convert to iconset, then to .icns
3. **SVG** - Convert to PNG (1024x1024), then iconset, then .icns

### Conversion Process

**PNG → ICNS:**
1. Use `sips` to create 10 different sizes (16-1024px, 1x and 2x)
2. Create temporary `.iconset` directory
3. Use `iconutil -c icns` to convert iconset to .icns
4. Copy to bundle Resources directory
5. Clean up temporary files

**SVG → ICNS:**
1. Use `qlmanage -t -s 1024` to render SVG to high-res PNG
2. Follow PNG → ICNS process above

**Icon Sizes Generated:**
- icon_16x16.png (16x16)
- icon_16x16@2x.png (32x32)
- icon_32x32.png (32x32)
- icon_32x32@2x.png (64x64)
- icon_128x128.png (128x128)
- icon_128x128@2x.png (256x256)
- icon_256x256.png (256x256)
- icon_256x256@2x.png (512x512)
- icon_512x512.png (512x512)
- icon_512x512@2x.png (1024x1024)

## Code Signing Integration

### Signing Process

1. **Entitlements Generation** (if hardened runtime enabled)
   - Auto-generates XML plist with requested entitlements
   - Saves to `/tmp/appbundle_<pid>.entitlements`

2. **Code Signing Execution**
   - Builds `codesign` command with options
   - Flags: `-s identity`, `-o runtime`, `--entitlements`, `--timestamp`, `--force`
   - Executes with proper error capture

3. **Signature Verification**
   - Runs `codesign --verify --verbose=2`
   - Confirms signature validity before completion

### Signing Identities

- **Ad-hoc signing:** `--sign -` (development only, shows security warnings)
- **Developer ID:** `--sign 'Developer ID Application: Your Name'` (distribution)
- Find your identity: `security find-identity -p codesigning -v`

### Hardened Runtime

Enable with `--hardened-runtime` flag. Creates stricter security environment required for:
- macOS 10.14.5+ distribution
- Notarization
- Gatekeeper approval

**Common Entitlement Exceptions:**
- `--allow-dyld-vars` - For wrapper scripts that set environment variables
- `--allow-unsigned` - For JIT compilers or runtime code generation
- `--allow-jit` - For JavaScript engines, interpreters

## Development Guidelines

### Adding New Features

1. **Update shared.h** - Add any new structures/enums/prototypes
2. **Implement in appropriate module** - Keep separation of concerns
3. **Update main.c** - Add CLI options and orchestration
4. **Test thoroughly** - Basic, icon, and code signing scenarios
5. **Update documentation** - This file and usage() function

### Code Style

- Pure C (C99+), no C++ or Objective-C
- Use `BOOL` type (defined as int) not bool
- Use CoreFoundation types (`CFStringRef`, `CFDictionaryRef`, etc.)
- Always check return values and handle errors
- Use `DEBUG_PRINT()` macro for debug output (enabled with `-DDEBUG`)
- Free all allocated memory (no leaks)
- Use `heap_printf()` for dynamic string formatting

### Testing Checklist

```bash
# 1. Clean build
make clean && make

# 2. Check for deprecated APIs
make check-deprecated

# 3. Basic bundle
./AppBundleGenerator 'Test' /tmp '/bin/echo Hi'

# 4. Validate Info.plist
plutil -lint /tmp/Test.app/Contents/Info.plist

# 5. Test icon conversion (PNG)
./AppBundleGenerator --icon test.png 'Test' /tmp '/bin/echo Hi'

# 6. Test icon conversion (SVG)
./AppBundleGenerator --icon test.svg 'Test' /tmp '/bin/echo Hi'

# 7. Test ad-hoc signing
./AppBundleGenerator --sign - --hardened-runtime 'Test' /tmp '/bin/echo Hi'

# 8. Verify signature
codesign -dvvv /tmp/Test.app

# 9. Launch bundle
open /tmp/Test.app
```

## Known Limitations

1. **English only** - No localization support beyond English.lproj
2. **Simple launcher script** - No complex environment setup or argument passing
3. **macOS only** - Requires macOS 12.0+ to build and run
4. **No file associations** - Cannot register UTI or document types
5. **No notarization** - Must manually notarize with `xcrun notarytool` after creation
6. **No complex entitlements** - Only supports common hardened runtime exceptions

## Future Enhancements

Potential improvements for future versions:

1. **Automatic notarization** - Integrate `xcrun notarytool` workflow
2. **DMG creation** - Wrap bundles in distributable disk images
3. **File association support** - Add UTI declarations to Info.plist
4. **Multiple localization** - Support for multiple .lproj directories
5. **Framework bundling** - Copy dependent libraries/frameworks into bundle
6. **Direct ImageIO** - Replace sips/iconutil with CoreGraphics APIs
7. **Template system** - Predefined templates for common app types
8. **GUI wrapper** - Simple GUI for non-technical users

## Troubleshooting

### Build Errors

**"command not found: clang"**
- Install Xcode Command Line Tools: `xcode-select --install`

**"cannot find -framework CoreFoundation"**
- Ensure Xcode is installed: `xcode-select -p`
- Set correct SDK: `export SDKROOT=$(xcrun --show-sdk-path)`

### Icon Conversion Errors

**"qlmanage failed for SVG"**
- SVG file may be corrupted or invalid
- Try converting SVG to PNG manually first
- Use PNG input instead

**"iconutil failed"**
- Check that all icon sizes were created in iconset directory
- Verify permissions on /tmp directory

### Code Signing Errors

**"No identity found"**
- Use ad-hoc signing: `--sign -`
- Or install Developer ID certificate in Keychain

**"entitlements failed"**
- Check entitlements XML is valid
- Try without custom entitlements first

**"timestamp server unavailable"**
- Network connection required for timestamp
- Timestamp is required for distribution builds

### Bundle Won't Launch

**"App is damaged and can't be opened"**
- Bundle is not code signed (macOS 10.15+ requirement)
- Use `--sign -` for ad-hoc signing
- Or right-click → Open to bypass Gatekeeper

**"No executable found"**
- Check launcher script has execute permissions (chmod 755)
- Verify executable path in script is correct

## macOS-Specific Dependencies

This tool requires macOS and will NOT build or run on Linux/Windows:

**Build-time:**
- Xcode Command Line Tools (clang, SDK)
- CoreFoundation framework

**Runtime:**
- macOS 12.0+ (Monterey or later)
- `sips` - Scriptable Image Processing System
- `iconutil` - Icon composition utility
- `qlmanage` - Quick Look thumbnail generation
- `codesign` - Code signing tool (optional)

## Contact

Questions, comments, and contributions:
- Author: Steven Edwards (winehacker@gmail.com)
- License: See source code headers for licensing terms
