#ifndef _SHARED_H
#define _SHARED_H

#define false 0
#define true 1

/* or DEFINE BOOL int */
typedef int BOOL;

#ifndef FALSE
#define FALSE               0
#endif

#ifndef TRUE
#define TRUE                1
#endif

#ifdef DEBUG
#define DEBUG_PRINT(...) do{ fprintf( stderr, __VA_ARGS__ ); } while( false )
#else
#define DEBUG_PRINT(...) do{ } while ( false )
#endif

/* Error codes for better error handling */
typedef enum {
    ERR_SUCCESS = 0,
    ERR_INVALID_ARGS,
    ERR_DIR_CREATION_FAILED,
    ERR_PLIST_GENERATION_FAILED,
    ERR_SCRIPT_GENERATION_FAILED,
    ERR_ICON_CONVERSION_FAILED,
    ERR_CODE_SIGNING_FAILED,
    ERR_FILE_NOT_FOUND,
    ERR_PERMISSION_DENIED
} ErrorCode;

/* Icon format types */
typedef enum {
    ICON_FORMAT_UNKNOWN,
    ICON_FORMAT_PNG,
    ICON_FORMAT_SVG,
    ICON_FORMAT_ICNS
} IconFormat;

/* Application bundle options structure */
typedef struct {
    /* Required arguments */
    const char *bundle_name;
    const char *bundle_dest;
    const char *executable_path;

    /* Optional - icon */
    const char *icon_path;

    /* Optional - code signing */
    const char *signing_identity;
    BOOL enable_hardened_runtime;
    const char *entitlements_file;
    BOOL force_sign;

    /* Optional - Info.plist customization */
    const char *bundle_identifier;
    const char *min_os_version;
    const char *app_category;
    const char *version;
    const char *short_version;

    /* Optional - entitlement exceptions */
    BOOL allow_jit;
    BOOL allow_unsigned_memory;
    BOOL allow_dyld_vars;
} AppBundleOptions;

/* Code signing options structure */
typedef struct {
    const char *identity;           /* Signing identity (e.g., "Developer ID Application: Name") */
    BOOL enable_hardened_runtime;   /* Add -o runtime flag */
    const char *entitlements_path;  /* Path to entitlements plist (optional) */
    BOOL force;                     /* Replace existing signature */
    BOOL timestamp;                 /* Include timestamp (required for distribution) */
} CodeSignOptions;

/* Main bundle generation function (updated signature) */
BOOL build_app_bundle(const AppBundleOptions *options);

/* Icon utility functions */
IconFormat detect_icon_format(const char *path);
BOOL copy_file(const char *src, const char *dst);
BOOL convert_png_to_icns(const char *png_path, const char *output_icns);
BOOL convert_svg_to_icns(const char *svg_path, const char *output_icns);
BOOL generate_iconset_from_png(const char *source_png, const char *iconset_dir);

/* Entitlements generation */
BOOL generate_entitlements_file(const char *output_path, BOOL hardened_runtime,
                                BOOL allow_jit, BOOL allow_unsigned_memory,
                                BOOL allow_dyld_vars);

/* Code signing functions */
BOOL codesign_bundle(const char *bundle_path, const CodeSignOptions *options);
BOOL verify_codesign(const char *bundle_path);

/* Error handling */
void print_error(ErrorCode code, const char *details);
const char* error_code_to_string(ErrorCode code);

#endif
