/*
 * Icon Utilities for AppBundleGenerator
 * Handles PNG, SVG, and ICNS format conversion for macOS app bundles
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/stat.h>

#include "shared.h"

/* External heap_printf function from main.c */
extern char* heap_printf(const char *format, ...);
extern BOOL create_directories(char *directory);

/* Detect icon format based on file extension */
IconFormat detect_icon_format(const char *path)
{
    const char *ext;

    if (!path) return ICON_FORMAT_UNKNOWN;

    /* Find the extension */
    ext = strrchr(path, '.');
    if (!ext) return ICON_FORMAT_UNKNOWN;

    /* Case-insensitive comparison */
    if (strcasecmp(ext, ".png") == 0) return ICON_FORMAT_PNG;
    if (strcasecmp(ext, ".svg") == 0) return ICON_FORMAT_SVG;
    if (strcasecmp(ext, ".icns") == 0) return ICON_FORMAT_ICNS;

    return ICON_FORMAT_UNKNOWN;
}

/* Simple file copy utility */
BOOL copy_file(const char *src, const char *dst)
{
    FILE *source, *dest;
    char buffer[8192];
    size_t bytes;
    BOOL ret = TRUE;

    if (!src || !dst) return FALSE;

    source = fopen(src, "rb");
    if (!source) {
        DEBUG_PRINT("Failed to open source file: %s\n", src);
        return FALSE;
    }

    dest = fopen(dst, "wb");
    if (!dest) {
        DEBUG_PRINT("Failed to open destination file: %s\n", dst);
        fclose(source);
        return FALSE;
    }

    /* Copy data in chunks */
    while ((bytes = fread(buffer, 1, sizeof(buffer), source)) > 0) {
        if (fwrite(buffer, 1, bytes, dest) != bytes) {
            DEBUG_PRINT("Failed to write to destination file\n");
            ret = FALSE;
            break;
        }
    }

    fclose(source);
    fclose(dest);

    if (ret) {
        DEBUG_PRINT("Successfully copied icon from %s to %s\n", src, dst);
    }

    return ret;
}

/* Generate all required icon sizes from a high-resolution PNG */
BOOL generate_iconset_from_png(const char *source_png, const char *iconset_dir)
{
    /* Array of required icon sizes for modern macOS */
    struct {
        int size;
        const char *name;
    } icons[] = {
        {16, "icon_16x16.png"},
        {32, "icon_16x16@2x.png"},
        {32, "icon_32x32.png"},
        {64, "icon_32x32@2x.png"},
        {128, "icon_128x128.png"},
        {256, "icon_128x128@2x.png"},
        {256, "icon_256x256.png"},
        {512, "icon_256x256@2x.png"},
        {512, "icon_512x512.png"},
        {1024, "icon_512x512@2x.png"},
        {0, NULL}
    };

    char cmd[2048];
    char *output;
    int i;
    int result;

    DEBUG_PRINT("Generating iconset from PNG: %s\n", source_png);

    /* Generate each required icon size using sips */
    for (i = 0; icons[i].name != NULL; i++) {
        output = heap_printf("%s/%s", iconset_dir, icons[i].name);

        /* Use sips to resize the image */
        snprintf(cmd, sizeof(cmd),
            "sips -z %d %d '%s' --out '%s' 2>/dev/null",
            icons[i].size, icons[i].size, source_png, output
        );

        DEBUG_PRINT("Creating icon: %s (%dx%d)\n", icons[i].name, icons[i].size, icons[i].size);

        result = system(cmd);
        if (result != 0) {
            DEBUG_PRINT("sips failed for size %d (exit code: %d)\n", icons[i].size, result);
            free(output);
            return FALSE;
        }

        free(output);
    }

    DEBUG_PRINT("Successfully generated all icon sizes\n");
    return TRUE;
}

/* Convert PNG to ICNS format */
BOOL convert_png_to_icns(const char *png_path, const char *output_icns)
{
    char *temp_iconset;
    char cmd[2048];
    BOOL ret = FALSE;
    int result;

    DEBUG_PRINT("Converting PNG to ICNS: %s -> %s\n", png_path, output_icns);

    /* Create temporary iconset directory */
    temp_iconset = heap_printf("/tmp/appbundle_%d.iconset", getpid());
    create_directories(temp_iconset);

    /* Generate iconset from PNG */
    if (!generate_iconset_from_png(png_path, temp_iconset)) {
        DEBUG_PRINT("Failed to generate iconset from PNG\n");
        goto cleanup;
    }

    /* Convert iconset to icns using iconutil */
    snprintf(cmd, sizeof(cmd),
        "iconutil -c icns '%s' -o '%s' 2>/dev/null",
        temp_iconset, output_icns
    );

    DEBUG_PRINT("Running iconutil to create ICNS file\n");

    result = system(cmd);
    if (result != 0) {
        DEBUG_PRINT("iconutil failed (exit code: %d)\n", result);
        goto cleanup;
    }

    ret = TRUE;
    DEBUG_PRINT("Successfully converted PNG to ICNS\n");

cleanup:
    /* Clean up temporary iconset directory */
    snprintf(cmd, sizeof(cmd), "rm -rf '%s'", temp_iconset);
    system(cmd);
    free(temp_iconset);

    return ret;
}

/* Convert SVG to ICNS format (via PNG intermediate) */
BOOL convert_svg_to_icns(const char *svg_path, const char *output_icns)
{
    char *temp_dir;
    char *iconset_dir;
    char *base_png;
    char *ql_output;
    char cmd[4096];
    int result;
    BOOL ret = FALSE;
    const char *svg_filename;

    DEBUG_PRINT("Converting SVG to ICNS: %s -> %s\n", svg_path, output_icns);

    /* Create temporary working directories */
    temp_dir = heap_printf("/tmp/appbundle_%d", getpid());
    iconset_dir = heap_printf("%s/temp.iconset", temp_dir);
    base_png = heap_printf("%s/base.png", temp_dir);

    create_directories(temp_dir);
    create_directories(iconset_dir);

    /* Step 1: Convert SVG to high-res PNG using qlmanage */
    DEBUG_PRINT("Step 1: Converting SVG to PNG using qlmanage\n");

    snprintf(cmd, sizeof(cmd),
        "qlmanage -t -s 1024 -o '%s' '%s' 2>/dev/null",
        temp_dir, svg_path
    );

    result = system(cmd);
    if (result != 0) {
        DEBUG_PRINT("qlmanage failed for SVG (exit code: %d)\n", result);
        goto cleanup;
    }

    /* qlmanage creates a file named <original>.svg.png or just <basename>.svg.png */
    /* We need to find and rename it to base.png */
    svg_filename = strrchr(svg_path, '/');
    if (svg_filename) {
        svg_filename++;  /* Skip the '/' */
    } else {
        svg_filename = svg_path;
    }

    ql_output = heap_printf("%s/%s.png", temp_dir, svg_filename);

    /* Check if the qlmanage output file exists */
    if (access(ql_output, F_OK) == 0) {
        /* Rename to base.png */
        DEBUG_PRINT("Renaming qlmanage output to base.png\n");
        rename(ql_output, base_png);
    } else {
        /* Try alternate naming convention */
        free(ql_output);
        ql_output = heap_printf("%s/%s", temp_dir, svg_filename);

        if (access(ql_output, F_OK) == 0) {
            rename(ql_output, base_png);
        } else {
            DEBUG_PRINT("Could not find qlmanage output file\n");
            free(ql_output);
            goto cleanup;
        }
    }

    free(ql_output);

    /* Verify base.png was created */
    if (access(base_png, F_OK) != 0) {
        DEBUG_PRINT("base.png was not created successfully\n");
        goto cleanup;
    }

    /* Step 2: Generate iconset from the PNG */
    DEBUG_PRINT("Step 2: Generating iconset from PNG\n");

    if (!generate_iconset_from_png(base_png, iconset_dir)) {
        DEBUG_PRINT("Failed to generate iconset\n");
        goto cleanup;
    }

    /* Step 3: Convert iconset to icns using iconutil */
    DEBUG_PRINT("Step 3: Converting iconset to ICNS\n");

    snprintf(cmd, sizeof(cmd),
        "iconutil -c icns '%s' -o '%s' 2>/dev/null",
        iconset_dir, output_icns
    );

    result = system(cmd);
    if (result != 0) {
        DEBUG_PRINT("iconutil failed (exit code: %d)\n", result);
        goto cleanup;
    }

    ret = TRUE;
    DEBUG_PRINT("Successfully converted SVG to ICNS\n");

cleanup:
    /* Clean up temporary directory */
    snprintf(cmd, sizeof(cmd), "rm -rf '%s'", temp_dir);
    system(cmd);

    free(temp_dir);
    free(iconset_dir);
    free(base_png);

    return ret;
}
