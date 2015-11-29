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
 *  (2.1) This agreement in no forces the license of any other patented technology
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
char *wine_applications_dir;
char* heap_printf(const char *format, ...);
BOOL create_directories(char *directory);

CFPropertyListRef CreateMyPropertyListFromFile(CFURLRef fileURL);
void WriteMyPropertyListToFile(CFPropertyListRef propertyList, CFURLRef fileURL );
 
 
CFDictionaryRef CreateMyDictionary(const char *linkname)
{
   CFMutableDictionaryRef dict;
   CFStringRef linkstr;

   linkstr = CFStringCreateWithCString(NULL, linkname, CFStringGetSystemEncoding());
 
 
   /* Create a dictionary that will hold the data. */
   dict = CFDictionaryCreateMutable( kCFAllocatorDefault,
            0,
            &kCFTypeDictionaryKeyCallBacks,
            &kCFTypeDictionaryValueCallBacks );
 
   /* Put the various items into the dictionary. */ 
   /* FIXME - Some values assumed the ought not to be */
   CFDictionarySetValue( dict, CFSTR("CFBundleDevelopmentRegion"), CFSTR("English") ); 
   CFDictionarySetValue( dict, CFSTR("CFBundleExecutable"), linkstr );
   CFDictionarySetValue( dict, CFSTR("CFBundleIdentifier"), CFSTR("org.winehq.wine") ); 
   CFDictionarySetValue( dict, CFSTR("CFBundleInfoDictionaryVersion"), CFSTR("6.0") ); 
   CFDictionarySetValue( dict, CFSTR("CFBundleName"), linkstr );
   CFDictionarySetValue( dict, CFSTR("CFBundlePackageType"), CFSTR("APPL") ); 
   CFDictionarySetValue( dict, CFSTR("CFBundleVersion"), CFSTR("1.0") ); 
   CFDictionarySetValue( dict, CFSTR("CFBundleSignature"), CFSTR("???") ); 
   CFDictionarySetValue( dict, CFSTR("CFBundleVersion"), CFSTR("1.0") ); 
   CFDictionarySetValue( dict, CFSTR("CFBundleIconFile"), CFSTR("icon.icns") ); 
   
   return dict;
}
 
void WriteMyPropertyListToFile( CFPropertyListRef propertyList, CFURLRef fileURL ) 
{
   CFDataRef xmlData;
   Boolean status;
   SInt32 errorCode;
 
   /* Convert the property list into XML data */
   xmlData = CFPropertyListCreateXMLData( kCFAllocatorDefault, propertyList );
 
   /* Write the XML data to the file */
   status = CFURLWriteDataAndPropertiesToResource (
               fileURL,
               xmlData,
               NULL,
               &errorCode);
 
   CFRelease(xmlData);
}

static BOOL generate_plist(const char *path_to_bundle_contents, const char *linkname)
{
    char *plist_path;
    static const char info_dot_plist_file[] = "Info.plist";
    CFPropertyListRef propertyList;
    CFStringRef pathstr;
    CFURLRef fileURL;

    /* Append all of the filename and path stuff and shove it in to CFStringRef */
    plist_path = heap_printf("%s/%s", path_to_bundle_contents, info_dot_plist_file); 
    pathstr = CFStringCreateWithCString(NULL, plist_path, CFStringGetSystemEncoding());
 
    /* Construct a complex dictionary object */
    propertyList = CreateMyDictionary(linkname);
 
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
                                   const char *args, const char *linkname)
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

static BOOL generate_icns_for_bundle(const char *path, const char *path_to_bundle_resources)
{
    /* Stub for now */
    DEBUG_PRINT("Unable to extract png file from target executable for App Bundle\n");
    return FALSE;
}

/* build out the directory structure for the bundle and then populate */
BOOL build_app_bundle(const char *path, char *bundle_dst, const char *args, const char *linkname)
{
    BOOL ret = FALSE;
    char *bundle, *path_to_bundle, *path_to_bundle_contents, *path_to_bundle_macos;
    char *path_to_bundle_resources, *path_to_bundle_resources_lang;
    static const char extentsion[] = "app";
    static const char contents[] = "Contents";
    static const char macos[] = "MacOS";
    static const char resources[] = "Resources";
    static const char resources_lang[] = "English.lproj"; /* FIXME */
    
    DEBUG_PRINT("bundle file name %s\n", linkname);

    bundle = heap_printf("%s.%s", linkname, extentsion);
    path_to_bundle = heap_printf("%s/%s", bundle_dst, bundle);
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
    
    ret = generate_icns_for_bundle(path, path_to_bundle_resources);
    if(ret==FALSE)
       DEBUG_PRINT("Failed to generate icon for Application Bundle\n");

    ret = generate_bundle_script(path_to_bundle_macos, path, args, linkname);
    if(ret==FALSE)
       return ret;

    ret = generate_pkginfo_file(path_to_bundle_contents); 
    if(ret==FALSE)
       return ret;

    ret = generate_plist(path_to_bundle_contents, linkname);
    if(ret==FALSE)
       return ret;
    /* we really shouldn't get here */
    return ret;
}



