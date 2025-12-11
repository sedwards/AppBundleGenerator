/*
 * Simple Mac OS X Application Bundler
 *
 * Copyright 2009-2015 Steven Edwards
 *
 * This program is Free Software 
 * Subject to the following License Terms: 
 *  (1.0) If you provide this application in binary form, You must make an offer 
 * to provide the source code for this application, and the offer must be in
 * some location that is easy for the consumer to find:
 * (help message, about dialog, physical package, etc) 
 *  (2.0) By distributing this application, you grant non-exclusive patent rights to any
 * patents you may own which describe the functionality of this program, for example: 
 * (You own a patent on building some sort of ApplicationBundle or icon conversion)
 *  (2.1) This agreement in no way forces the license of any other patented technology
 *  (3.0) As a special exception to this license agreement, you may chose to incorporate parts
 * of this application under the terms of the (L)GPLv2 and GPLv2.1 or any later version.
 *  (3.1) If you want to license under any other terms, please contact the author.
 *  (4.0) There is NO WARRANTY express or implied in any part of this software
 *
 *
 * NOTES: An Application Bundle generally has the following layout
 *
 * foo.app/Contents
 * foo.app/Contents/Info.plist
 * foo.app/Contents/MacOS/foo (can be script or real binary)
 * foo.app/Contents/Resources/appIcon.icns (Apple Icon format)
 * foo.app/Contents/Resources/English.lproj/infoPlist.strings
 * foo.app/Contents/Resources/English.lproj/MainMenu.nib (Menu Layout)
 *
 * There can be more to a bundle depending on the target, what resources
 * it contains and what the target platform but this simplifed format
 * is all we really need for now for Wine.
 * 
 * TODO:
 * - Add support for writing bundles to the Desktop
 * - Convert to using CoreFoundation API rather than standard unix file ops
 * - Fix up the extract_icon routine from main source file and see if we
 *   can convert to *.icns or use png directly
 * - See if there is anything else in the rsrc section of the target that
 *   we might want to dump in a *.plist. Version information for the target
 *   and or Wine Version information come to mind.
 * - Association Support
 * - sha1hash of target application in bundle plist
 */ 


#include <stdio.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <CoreFoundation/CoreFoundation.h>

#include "shared.h"

extern char *mac_desktop_dir;
char* heap_printf(const char *format, ...);
BOOL create_directories(char *directory);

CFPropertyListRef CreateMyPropertyListFromFile(CFURLRef fileURL);
void WriteMyPropertyListToFile(CFPropertyListRef propertyList, CFURLRef fileURL );


/* Sanitize bundle name for use in identifier: lowercase, replace spaces with hyphens, remove special chars */
static char* sanitize_bundle_name(const char *name)
{
    size_t len = strlen(name);
    char *sanitized = malloc(len + 1);
    size_t j = 0;

    if (!sanitized) return NULL;

    for (size_t i = 0; i < len; i++) {
        char c = name[i];

        /* Convert to lowercase */
        if (c >= 'A' && c <= 'Z') {
            sanitized[j++] = c + 32;  /* Convert to lowercase */
        }
        /* Replace spaces with hyphens */
        else if (c == ' ') {
            sanitized[j++] = '-';
        }
        /* Keep alphanumeric and hyphens */
        else if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-') {
            sanitized[j++] = c;
        }
        /* Skip other characters */
    }

    sanitized[j] = '\0';
    return sanitized;
}

/* Generate a unique bundle identifier from the bundle name */
static CFStringRef generate_bundle_identifier(const char *linkname)
{
    char *sanitized;
    char *identifier;
    CFStringRef result;

    sanitized = sanitize_bundle_name(linkname);
    if (!sanitized) {
        /* Fallback to generic identifier if sanitization fails */
        return CFStringCreateWithCString(NULL, "com.appbundlegenerator.app", kCFStringEncodingUTF8);
    }

    /* Format: com.appbundlegenerator.<sanitized_name> */
    identifier = heap_printf("com.appbundlegenerator.%s", sanitized);
    free(sanitized);

    if (!identifier) {
        return CFStringCreateWithCString(NULL, "com.appbundlegenerator.app", kCFStringEncodingUTF8);
    }

    result = CFStringCreateWithCString(NULL, identifier, kCFStringEncodingUTF8);
    free(identifier);

    return result;
}


CFDictionaryRef CreateMyDictionary(const char *linkname, const char *category,
                                   const char *min_os_version, const char *version,
                                   const char *custom_identifier)
{
   CFMutableDictionaryRef dict;
   CFStringRef linkstr;
   CFStringRef bundleId;
   CFStringRef minOsVer;
   CFStringRef catStr;
   CFStringRef verStr;

   linkstr = CFStringCreateWithCString(NULL, linkname, kCFStringEncodingUTF8);

   /* Generate or use custom bundle identifier */
   if (custom_identifier) {
       bundleId = CFStringCreateWithCString(NULL, custom_identifier, kCFStringEncodingUTF8);
   } else {
       bundleId = generate_bundle_identifier(linkname);
   }

   /* Create a dictionary that will hold the data. */
   dict = CFDictionaryCreateMutable( kCFAllocatorDefault,
            0,
            &kCFTypeDictionaryKeyCallBacks,
            &kCFTypeDictionaryValueCallBacks );

   /* ========== EXISTING KEYS (modernized) ========== */

   /* Use modern locale code "en" instead of "English" */
   CFDictionarySetValue( dict, CFSTR("CFBundleDevelopmentRegion"), CFSTR("en") );

   CFDictionarySetValue( dict, CFSTR("CFBundleExecutable"), linkstr );

   /* Use dynamically generated identifier instead of hardcoded */
   CFDictionarySetValue( dict, CFSTR("CFBundleIdentifier"), bundleId );

   CFDictionarySetValue( dict, CFSTR("CFBundleInfoDictionaryVersion"), CFSTR("6.0") );

   CFDictionarySetValue( dict, CFSTR("CFBundleName"), linkstr );

   /* Add display name for better UI appearance */
   CFDictionarySetValue( dict, CFSTR("CFBundleDisplayName"), linkstr );

   CFDictionarySetValue( dict, CFSTR("CFBundlePackageType"), CFSTR("APPL") );

   /* Use provided version or default */
   if (version) {
       verStr = CFStringCreateWithCString(NULL, version, kCFStringEncodingUTF8);
       CFDictionarySetValue( dict, CFSTR("CFBundleShortVersionString"), verStr );
       CFDictionarySetValue( dict, CFSTR("CFBundleVersion"), verStr );
       CFRelease(verStr);
   } else {
       CFDictionarySetValue( dict, CFSTR("CFBundleShortVersionString"), CFSTR("1.0.0") );
       CFDictionarySetValue( dict, CFSTR("CFBundleVersion"), CFSTR("1") );
   }

   /* Signature is deprecated but kept for compatibility */
   CFDictionarySetValue( dict, CFSTR("CFBundleSignature"), CFSTR("????") );

   CFDictionarySetValue( dict, CFSTR("CFBundleIconFile"), CFSTR("icon.icns") );

   /* ========== NEW KEYS for macOS 12+ ========== */

   /* LSMinimumSystemVersion - CRITICAL for macOS 12+ compatibility */
   if (min_os_version) {
       minOsVer = CFStringCreateWithCString(NULL, min_os_version, kCFStringEncodingUTF8);
       CFDictionarySetValue( dict, CFSTR("LSMinimumSystemVersion"), minOsVer );
       CFRelease(minOsVer);
   } else {
       CFDictionarySetValue( dict, CFSTR("LSMinimumSystemVersion"), CFSTR("12.0") );
   }

   /* NSHighResolutionCapable - Retina display support */
   CFDictionarySetValue( dict, CFSTR("NSHighResolutionCapable"), kCFBooleanTrue );

   /* LSApplicationCategoryType - Required for Gatekeeper */
   if (category) {
       catStr = CFStringCreateWithCString(NULL, category, kCFStringEncodingUTF8);
       CFDictionarySetValue( dict, CFSTR("LSApplicationCategoryType"), catStr );
       CFRelease(catStr);
   } else {
       CFDictionarySetValue( dict, CFSTR("LSApplicationCategoryType"),
                           CFSTR("public.app-category.utilities") );
   }

   /* NSSupportsAutomaticGraphicsSwitching - GPU selection on MacBook Pros */
   CFDictionarySetValue( dict, CFSTR("NSSupportsAutomaticGraphicsSwitching"), kCFBooleanTrue );

   /* NSPrincipalClass - Required for modern apps */
   CFDictionarySetValue( dict, CFSTR("NSPrincipalClass"), CFSTR("NSApplication") );

   /* Cleanup */
   CFRelease(linkstr);
   CFRelease(bundleId);

   return dict;
}
 
void WriteMyPropertyListToFile( CFPropertyListRef propertyList, CFURLRef fileURL )
{
   CFDataRef data;
   CFErrorRef error = NULL;
   CFWriteStreamRef stream;

   /* Convert the property list into binary data using modern API */
   data = CFPropertyListCreateData(
       kCFAllocatorDefault,
       propertyList,
       kCFPropertyListBinaryFormat_v1_0,  /* Binary format for faster parsing */
       0,
       &error
   );

   if (!data) {
       if (error) {
           CFStringRef errorDesc = CFErrorCopyDescription(error);
           if (errorDesc) {
               DEBUG_PRINT("Property list creation failed: %s\n",
                          CFStringGetCStringPtr(errorDesc, kCFStringEncodingUTF8));
               CFRelease(errorDesc);
           }
           CFRelease(error);
       }
       return;
   }

   /* Write using CFWriteStream instead of deprecated API */
   stream = CFWriteStreamCreateWithFile(kCFAllocatorDefault, fileURL);
   if (!stream) {
       CFRelease(data);
       return;
   }

   if (CFWriteStreamOpen(stream)) {
       CFIndex dataLength = CFDataGetLength(data);
       CFIndex bytesWritten = CFWriteStreamWrite(
           stream,
           CFDataGetBytePtr(data),
           dataLength
       );

       if (bytesWritten != dataLength) {
           DEBUG_PRINT("Warning: Incomplete write to plist file\n");
       }

       CFWriteStreamClose(stream);
   }

   CFRelease(stream);
   CFRelease(data);
}

static BOOL generate_plist(const char *path_to_bundle_contents, const AppBundleOptions *options)
{
    char *plist_path;
    static const char info_dot_plist_file[] = "Info.plist";
    CFPropertyListRef propertyList;
    CFStringRef pathstr;
    CFURLRef fileURL;

    /* Append all of the filename and path stuff and shove it in to CFStringRef */
    plist_path = heap_printf("%s/%s", path_to_bundle_contents, info_dot_plist_file);
    pathstr = CFStringCreateWithCString(NULL, plist_path, kCFStringEncodingUTF8);

    /* Construct a complex dictionary object with all options */
    propertyList = CreateMyDictionary(
        options->bundle_name,
        options->app_category,
        options->min_os_version,
        options->version,
        options->bundle_identifier
    );
 
    /* Create a URL that specifies the file we will create to hold the XML data. */
    fileURL = CFURLCreateWithFileSystemPath( kCFAllocatorDefault,
                                             pathstr,
                                             kCFURLPOSIXPathStyle,
                                             false );

    /* Write the property list to the file */
    WriteMyPropertyListToFile( propertyList, fileURL );
    CFRelease(propertyList);
    CFRelease(fileURL);

    DEBUG_PRINT("Creating Bundle Info.plist at %s\n", wine_dbgstr_a(plist_path));

    return TRUE;
}

/* TODO: If I understand this file correctly, it is used for associations */
static BOOL generate_pkginfo_file(const char* path_to_bundle_contents)
{
    FILE *file;
    char *bundle_and_pkginfo;
    static const char pkginfo_file[] = "PkgInfo";

    bundle_and_pkginfo = heap_printf("%s/%s", path_to_bundle_contents, pkginfo_file);

    DEBUG_PRINT("Creating Bundle PkgInfo at %s\n", wine_dbgstr_a(bundle_and_pkginfo));

    file = fopen(bundle_and_pkginfo, "w");
    if (file == NULL)
        return FALSE;

    fprintf(file, "APPL????");

    fclose(file);
    return TRUE;
}


/* inspired by write_desktop_entry() in xdg support code */
static BOOL generate_bundle_script(const char *path_to_bundle_macos, const char *path,
                                   const char *args __attribute__((unused)), const char *linkname)
{
    FILE *file;
    char *bundle_and_script;

    bundle_and_script = heap_printf("%s/%s", path_to_bundle_macos, linkname);

    DEBUG_PRINT("Creating Bundle helper script at %s\n", wine_dbgstr_a(bundle_and_script));

    file = fopen(bundle_and_script, "w");
    if (file == NULL)
        return FALSE;

    fprintf(file, "#!/bin/sh\n");
    fprintf(file, "#Helper script for %s\n\n", linkname);

    /* Just like xdg-menus we DO NOT support running a wine binary other
     * than one that is already present in the path
     */
    fprintf(file, "%s \n\n", path );

    fprintf(file, "#EOF");
    
    fclose(file);    
    chmod(bundle_and_script,0755);
    
    return TRUE;
}

/* Add icon to bundle - now fully implemented with PNG/SVG/ICNS support */
BOOL add_icns_for_bundle(const char *icon_src, const char *path_to_bundle_resources)
{
    IconFormat format;
    char *output_icns;
    BOOL ret = FALSE;

    if (!icon_src || !path_to_bundle_resources) {
        DEBUG_PRINT("Invalid parameters to add_icns_for_bundle\n");
        return FALSE;
    }

    /* Check if source file exists and is readable */
    if (access(icon_src, R_OK) != 0) {
        DEBUG_PRINT("Icon source file not accessible: %s\n", icon_src);
        return FALSE;
    }

    /* Detect the icon format */
    format = detect_icon_format(icon_src);
    if (format == ICON_FORMAT_UNKNOWN) {
        DEBUG_PRINT("Unknown icon format: %s (supported: .png, .svg, .icns)\n", icon_src);
        return FALSE;
    }

    /* Determine output path */
    output_icns = heap_printf("%s/icon.icns", path_to_bundle_resources);
    if (!output_icns) {
        DEBUG_PRINT("Failed to allocate memory for output path\n");
        return FALSE;
    }

    /* Convert or copy based on format */
    switch(format) {
        case ICON_FORMAT_ICNS:
            DEBUG_PRINT("Icon is already ICNS, copying directly\n");
            ret = copy_file(icon_src, output_icns);
            break;

        case ICON_FORMAT_PNG:
            DEBUG_PRINT("Converting PNG icon to ICNS\n");
            ret = convert_png_to_icns(icon_src, output_icns);
            break;

        case ICON_FORMAT_SVG:
            DEBUG_PRINT("Converting SVG icon to ICNS\n");
            ret = convert_svg_to_icns(icon_src, output_icns);
            break;

        default:
            DEBUG_PRINT("Unsupported icon format\n");
            ret = FALSE;
    }

    free(output_icns);

    if (ret) {
        DEBUG_PRINT("Successfully added icon to bundle\n");
    } else {
        DEBUG_PRINT("Failed to add icon to bundle\n");
    }

    return ret;
}

/* build out the directory structure for the bundle and then populate */
BOOL build_app_bundle(const AppBundleOptions *options)
{
    BOOL ret = FALSE;
    char *bundle, *path_to_bundle, *path_to_bundle_contents, *path_to_bundle_macos;
    char *path_to_bundle_resources, *path_to_bundle_resources_lang;
    static const char extension[] = "app";
    static const char contents[] = "Contents";
    static const char macos[] = "MacOS";
    static const char resources[] = "Resources";
    static const char resources_lang[] = "English.lproj"; /* FIXME */

    if (!options) {
        DEBUG_PRINT("Invalid options passed to build_app_bundle\n");
        return FALSE;
    }

    DEBUG_PRINT("bundle file name %s\n", options->bundle_name);

    bundle = heap_printf("%s.%s", options->bundle_name, extension);
    path_to_bundle = heap_printf("%s/%s", options->bundle_dest, bundle);
    path_to_bundle_contents = heap_printf("%s/%s", path_to_bundle, contents);
    path_to_bundle_macos =  heap_printf("%s/%s", path_to_bundle_contents, macos);
    path_to_bundle_resources = heap_printf("%s/%s", path_to_bundle_contents, resources);
    path_to_bundle_resources_lang = heap_printf("%s/%s", path_to_bundle_resources, resources_lang);

    create_directories(path_to_bundle);
    create_directories(path_to_bundle_contents);
    create_directories(path_to_bundle_macos);
    create_directories(path_to_bundle_resources);
    create_directories(path_to_bundle_resources_lang);

    DEBUG_PRINT("created bundle %s\n", path_to_bundle);

    ret = generate_bundle_script(path_to_bundle_macos, options->executable_path, NULL, options->bundle_name);
    if(ret==FALSE)
       return ret;

    ret = generate_pkginfo_file(path_to_bundle_contents);
    if(ret==FALSE)
       return ret;

    ret = generate_plist(path_to_bundle_contents, options);
    if(ret==FALSE)
       return ret;

    /* Add icon if provided */
    if (options->icon_path) {
        ret = add_icns_for_bundle(options->icon_path, path_to_bundle_resources);
        if(ret==FALSE)
           DEBUG_PRINT("Failed to add icon to Application Bundle\n");
    }

    /* Cleanup allocated paths */
    free(bundle);
    free(path_to_bundle);
    free(path_to_bundle_contents);
    free(path_to_bundle_macos);
    free(path_to_bundle_resources);
    free(path_to_bundle_resources_lang);

    return TRUE;
}

/* Code sign a bundle with specified options */
BOOL codesign_bundle(const char *bundle_path, const CodeSignOptions *options)
{
    char cmd[4096];
    int offset = 0;
    int result;

    if (!bundle_path) {
        DEBUG_PRINT("Invalid bundle path for code signing\n");
        return FALSE;
    }

    if (!options || !options->identity) {
        DEBUG_PRINT("Code signing skipped: no identity provided\n");
        return TRUE;  /* Not an error, just skip signing */
    }

    DEBUG_PRINT("Code signing bundle: %s\n", bundle_path);
    DEBUG_PRINT("  Identity: %s\n", options->identity);

    /* Build codesign command */
    offset = snprintf(cmd, sizeof(cmd), "codesign -s '%s'", options->identity);

    /* Add hardened runtime flag */
    if (options->enable_hardened_runtime) {
        DEBUG_PRINT("  Hardened runtime: enabled\n");
        offset += snprintf(cmd + offset, sizeof(cmd) - offset, " -o runtime");
    }

    /* Add force flag to replace existing signature */
    if (options->force) {
        DEBUG_PRINT("  Force: replacing existing signature\n");
        offset += snprintf(cmd + offset, sizeof(cmd) - offset, " --force");
    }

    /* Add timestamp for distribution (recommended) */
    if (options->timestamp) {
        DEBUG_PRINT("  Timestamp: enabled\n");
        offset += snprintf(cmd + offset, sizeof(cmd) - offset, " --timestamp");
    }

    /* Add entitlements if provided */
    if (options->entitlements_path) {
        DEBUG_PRINT("  Entitlements: %s\n", options->entitlements_path);
        offset += snprintf(cmd + offset, sizeof(cmd) - offset,
                         " --entitlements '%s'", options->entitlements_path);
    }

    /* Add verbose output for debugging */
    offset += snprintf(cmd + offset, sizeof(cmd) - offset, " --verbose");

    /* Add bundle path */
    offset += snprintf(cmd + offset, sizeof(cmd) - offset, " '%s'", bundle_path);

    /* Redirect stderr to stdout for better error capture */
    offset += snprintf(cmd + offset, sizeof(cmd) - offset, " 2>&1");

    DEBUG_PRINT("Executing: %s\n", cmd);

    /* Execute codesign command */
    result = system(cmd);

    if (result != 0) {
        DEBUG_PRINT("Code signing failed with exit code %d\n", result);
        return FALSE;
    }

    DEBUG_PRINT("Code signing successful\n");
    return TRUE;
}

/* Verify the code signature of a bundle */
BOOL verify_codesign(const char *bundle_path)
{
    char cmd[2048];
    int result;

    if (!bundle_path) {
        DEBUG_PRINT("Invalid bundle path for verification\n");
        return FALSE;
    }

    DEBUG_PRINT("Verifying code signature: %s\n", bundle_path);

    /* Build verification command */
    snprintf(cmd, sizeof(cmd),
            "codesign --verify --verbose=2 '%s' 2>&1",
            bundle_path);

    /* Execute verification */
    result = system(cmd);

    if (result != 0) {
        DEBUG_PRINT("Code signature verification failed (exit code: %d)\n", result);
        return FALSE;
    }

    DEBUG_PRINT("Code signature verification successful\n");
    return TRUE;
}

