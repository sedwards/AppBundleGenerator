/*
 * Entitlements Generation for AppBundleGenerator
 * Creates entitlements plist files for code signing with hardened runtime
 */

#include <stdio.h>
#include <CoreFoundation/CoreFoundation.h>

#include "shared.h"

/* External function from appbundler.c */
extern void WriteMyPropertyListToFile(CFPropertyListRef propertyList, CFURLRef fileURL);

/*
 * Generate an entitlements plist file for code signing
 *
 * Parameters:
 *   output_path - Path where the entitlements plist will be written
 *   hardened_runtime - Whether hardened runtime is enabled (TRUE if using -o runtime flag)
 *   allow_jit - Allow JIT compilation (com.apple.security.cs.allow-jit)
 *   allow_unsigned_memory - Allow unsigned executable memory
 *   allow_dyld_vars - Allow DYLD environment variables
 *
 * Returns: TRUE on success, FALSE on failure
 */
BOOL generate_entitlements_file(const char *output_path, BOOL hardened_runtime,
                                BOOL allow_jit, BOOL allow_unsigned_memory,
                                BOOL allow_dyld_vars)
{
    CFMutableDictionaryRef dict;
    CFURLRef fileURL;
    CFStringRef pathStr;

    if (!output_path) {
        DEBUG_PRINT("Invalid output path for entitlements\n");
        return FALSE;
    }

    DEBUG_PRINT("Generating entitlements file: %s\n", output_path);

    /* Create a mutable dictionary to hold entitlements */
    dict = CFDictionaryCreateMutable(
        kCFAllocatorDefault,
        0,
        &kCFTypeDictionaryKeyCallBacks,
        &kCFTypeDictionaryValueCallBacks
    );

    if (!dict) {
        DEBUG_PRINT("Failed to create entitlements dictionary\n");
        return FALSE;
    }

    /* Add entitlements for hardened runtime exceptions */
    if (hardened_runtime) {
        DEBUG_PRINT("Adding hardened runtime entitlements\n");

        /* Allow JIT compilation (needed for some scripting languages) */
        if (allow_jit) {
            DEBUG_PRINT("  - Allowing JIT compilation\n");
            CFDictionarySetValue(dict,
                CFSTR("com.apple.security.cs.allow-jit"),
                kCFBooleanTrue
            );
        }

        /* Allow unsigned executable memory (needed for some runtime code generation) */
        if (allow_unsigned_memory) {
            DEBUG_PRINT("  - Allowing unsigned executable memory\n");
            CFDictionarySetValue(dict,
                CFSTR("com.apple.security.cs.allow-unsigned-executable-memory"),
                kCFBooleanTrue
            );
        }

        /* Allow DYLD environment variables (needed for wrapper scripts) */
        if (allow_dyld_vars) {
            DEBUG_PRINT("  - Allowing DYLD environment variables\n");
            CFDictionarySetValue(dict,
                CFSTR("com.apple.security.cs.allow-dyld-environment-variables"),
                kCFBooleanTrue
            );
        }

        /* Disable library validation for wrapper scripts
         * This allows the script to load libraries that aren't signed by the same team
         */
        DEBUG_PRINT("  - Disabling library validation (for wrapper scripts)\n");
        CFDictionarySetValue(dict,
            CFSTR("com.apple.security.cs.disable-library-validation"),
            kCFBooleanTrue
        );
    }

    /* If no entitlements were added, add a placeholder to create a valid (but empty) plist */
    if (CFDictionaryGetCount(dict) == 0) {
        DEBUG_PRINT("No specific entitlements requested, creating minimal file\n");
        /* Some tools require at least one entitlement, so we add a harmless one */
        CFDictionarySetValue(dict,
            CFSTR("com.apple.security.get-task-allow"),
            kCFBooleanTrue
        );
    }

    /* Convert path to CFString and create URL */
    pathStr = CFStringCreateWithCString(NULL, output_path, kCFStringEncodingUTF8);
    if (!pathStr) {
        DEBUG_PRINT("Failed to create path string\n");
        CFRelease(dict);
        return FALSE;
    }

    fileURL = CFURLCreateWithFileSystemPath(
        kCFAllocatorDefault,
        pathStr,
        kCFURLPOSIXPathStyle,
        false  /* not a directory */
    );

    if (!fileURL) {
        DEBUG_PRINT("Failed to create file URL\n");
        CFRelease(pathStr);
        CFRelease(dict);
        return FALSE;
    }

    /* Write the dictionary to the plist file in XML format (required for codesign) */
    /* Note: Entitlements must be XML, not binary format */
    CFDataRef xmlData;
    CFWriteStreamRef stream;

    xmlData = CFPropertyListCreateData(
        kCFAllocatorDefault,
        dict,
        kCFPropertyListXMLFormat_v1_0,  /* XML format required for entitlements */
        0,
        NULL
    );

    if (!xmlData) {
        DEBUG_PRINT("Failed to create XML data for entitlements\n");
        CFRelease(dict);
        CFRelease(pathStr);
        CFRelease(fileURL);
        return FALSE;
    }

    stream = CFWriteStreamCreateWithFile(kCFAllocatorDefault, fileURL);
    if (stream && CFWriteStreamOpen(stream)) {
        CFWriteStreamWrite(stream, CFDataGetBytePtr(xmlData), CFDataGetLength(xmlData));
        CFWriteStreamClose(stream);
        CFRelease(stream);
    }

    CFRelease(xmlData);

    /* Cleanup */
    CFRelease(dict);
    CFRelease(pathStr);
    CFRelease(fileURL);

    DEBUG_PRINT("Successfully generated entitlements file\n");

    return TRUE;
}
