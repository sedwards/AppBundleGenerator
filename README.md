# AppBundleGenerator

Modern macOS Application Bundle Creator - Version 2.0

A command-line utility for creating macOS Application Bundles (.app) from executables or command strings, with full icon generation, code signing, and modern Info.plist support.

## Features

- ✅ **Modern macOS 12+ Support** - Targets Monterey and later with all modern Info.plist keys
- ✅ **Automatic Icon Conversion** - Converts PNG/SVG to .icns format automatically
- ✅ **Code Signing Integration** - Built-in support for ad-hoc and Developer ID signing
- ✅ **Hardened Runtime** - Full support for hardened runtime and entitlements
- ✅ **Auto-generated Bundle IDs** - No more hardcoded identifiers
- ✅ **Modern APIs** - No deprecated CoreFoundation APIs
- ✅ **Comprehensive CLI** - Easy-to-use command-line interface with extensive options
- ✅ **Backward Compatible** - Supports original simple usage pattern

## Quick Start

### Build

```bash
make
```

Requirements: macOS 12.0+, Xcode Command Line Tools

### Basic Usage

```bash
# Simple bundle creation
./AppBundleGenerator 'My App' /Applications '/path/to/executable'

# With icon
./AppBundleGenerator --icon icon.svg 'My App' /Applications '/path/to/executable'

# With code signing
./AppBundleGenerator --sign - --hardened-runtime 'My App' /Applications '/path/to/executable'
```

## Installation

```bash
sudo make install
```

Installs to `/usr/local/bin/AppBundleGenerator`

## Command-Line Options

### Required Arguments
- `BundleName` - Display name for the application
- `DestinationDir` - Directory where .app bundle will be created
- `ExecutableOrCommand` - Command or path to execute when launched

### Optional Arguments

**Icon Options:**
- `--icon PATH` - Icon file (PNG, SVG, or ICNS format)

**Code Signing:**
- `--sign IDENTITY` - Code signing identity (use `-` for ad-hoc)
- `--hardened-runtime` - Enable hardened runtime
- `--entitlements PATH` - Custom entitlements plist
- `--force-sign` - Replace existing signature

**Info.plist Customization:**
- `--identifier ID` - Custom bundle identifier
- `--min-os VERSION` - Minimum macOS version (default: 12.0)
- `--category TYPE` - App category (default: public.app-category.utilities)
- `--version VER` - Bundle version (default: 1.0.0)

**Entitlement Exceptions:**
- `--allow-jit` - Allow JIT compilation
- `--allow-unsigned` - Allow unsigned executable memory
- `--allow-dyld-vars` - Allow DYLD environment variables

## Examples

### Development Build

```bash
./AppBundleGenerator \
  --icon app.svg \
  --sign - \
  --hardened-runtime \
  'My Development Tool' /tmp '/usr/local/bin/mytool'
```

### Production Build

```bash
./AppBundleGenerator \
  --icon app.svg \
  --sign 'Developer ID Application: Your Name' \
  --hardened-runtime \
  --identifier com.example.myapp \
  --category public.app-category.developer-tools \
  --min-os 12.0 \
  --version 2.0.0 \
  'My Application' /Applications '/usr/local/bin/myapp'
```

### Terminal Launcher

```bash
./AppBundleGenerator \
  --icon Terminal.png \
  'Midnight Commander' /Applications \
  'open -b com.apple.terminal /usr/local/bin/mc'
```

## What's New in Version 2.0

### API Modernization
- Replaced all deprecated CoreFoundation APIs
- Uses modern `CFPropertyListCreateData()` with format parameters
- Proper error handling with `CFErrorRef`
- Binary plist format for better performance

### Enhanced Info.plist
- Auto-generated bundle identifiers (no more "org.darkstar.root"!)
- Modern macOS 12+ keys: `LSMinimumSystemVersion`, `NSHighResolutionCapable`, etc.
- Full Retina display support
- Proper Gatekeeper integration with `LSApplicationCategoryType`

### Icon Generation
- Automatic PNG → ICNS conversion
- Automatic SVG → ICNS conversion (via PNG intermediate)
- All 10 required icon sizes generated (16px to 1024px, 1x and 2x)
- Uses macOS utilities: `sips`, `iconutil`, `qlmanage`

### Code Signing
- Built-in code signing with `codesign` integration
- Automatic entitlements generation for hardened runtime
- Support for ad-hoc and Developer ID signing
- Signature verification after signing

### Build System
- Modern clang compiler (not gcc)
- Security flags: stack protection, fortify source
- Strict warnings including deprecated API errors
- macOS 12.0+ deployment target

## Architecture

The tool is written in pure C and consists of:

- **main.c** (369 lines) - CLI interface and orchestration
- **appbundler.c** (577 lines) - Bundle generation engine
- **icon_utils.c** (268 lines) - Icon conversion pipeline
- **entitlements.c** (161 lines) - Entitlements generation
- **shared.h** (106 lines) - Common definitions

Total: ~1,500 lines of modern C code.

## Requirements

**Build:**
- macOS 12.0 or later
- Xcode Command Line Tools (`xcode-select --install`)
- clang compiler

**Runtime:**
- macOS 12.0 or later
- Standard macOS utilities: `sips`, `iconutil`, `qlmanage`
- `codesign` (for code signing features)

## Testing

```bash
# Run all tests
make clean && make
make check-deprecated

# Test basic bundle
./AppBundleGenerator 'Test' /tmp '/bin/echo Hello'

# Validate
plutil -lint /tmp/Test.app/Contents/Info.plist
open /tmp/Test.app
```

## Known Limitations

- English localization only (English.lproj)
- Simple launcher script (no complex environment setup)
- macOS-only (requires CoreFoundation framework)
- No automatic notarization (must run `xcrun notarytool` separately)
- No file association support (UTI declarations)

## Future Enhancements

- Automatic notarization workflow
- DMG creation for distribution
- File association/UTI support
- Multiple localization support
- Framework bundling
- Template system for common app types

## Troubleshooting

### Build Issues

**Command not found: clang**
```bash
xcode-select --install
```

**Cannot find CoreFoundation**
```bash
xcode-select -p  # Should show Xcode path
export SDKROOT=$(xcrun --show-sdk-path)
```

### Icon Issues

**SVG conversion fails**
- Try converting SVG to PNG manually first
- Use PNG input instead of SVG

**Icon doesn't appear**
- Check `/tmp/appbundle_*.iconset` wasn't cleaned up prematurely
- Verify icon file exists in `Bundle.app/Contents/Resources/icon.icns`

### Code Signing Issues

**No identity found**
- Use ad-hoc signing: `--sign -`
- List available identities: `security find-identity -p codesigning -v`

**Bundle won't open on macOS 10.15+**
- Bundle must be code signed (even ad-hoc)
- Use `--sign -` for development
- Right-click → Open to bypass Gatekeeper temporarily

## Contributing

This is a mature, stable tool. Contributions welcome for:
- Bug fixes
- Documentation improvements
- New features (see Future Enhancements)

Please maintain:
- Pure C code (C99+)
- CoreFoundation APIs only (no Objective-C)
- Backward compatibility
- Comprehensive testing

## License

See source code headers for licensing terms.

Original development for Wine integration on macOS.

## Author

Steven Edwards (winehacker@gmail.com)

## Links

- [macOS Bundle Programming Guide](https://developer.apple.com/library/archive/documentation/CoreFoundation/Conceptual/CFBundles/Introduction/Introduction.html)
- [Info.plist Keys](https://developer.apple.com/library/archive/documentation/General/Reference/InfoPlistKeyReference/Introduction/Introduction.html)
- [Code Signing Guide](https://developer.apple.com/library/archive/documentation/Security/Conceptual/CodeSigningGuide/Introduction/Introduction.html)
