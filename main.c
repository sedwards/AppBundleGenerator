#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <errno.h>
#include <getopt.h>
#include <unistd.h>

#include "shared.h"

char* heap_printf(const char *format, ...)
{
    va_list args;
    int size = 4096;
    char *buffer, *ret;
    int n;

    while (1)
    {
        buffer = malloc(size);
        if (buffer == NULL)
            break;
        va_start(args, format);
        n = vsnprintf(buffer, size, format, args);
        va_end(args);
        if (n == -1)
            size *= 2;
        else if (n >= size)
            size = n + 1;
        else
            break;
        free(buffer);
    }

    if (!buffer) return NULL;
    ret = realloc( buffer, strlen(buffer) + 1 );
    if (!ret) ret = buffer;
    return ret;
}

BOOL create_directories(char *directory)
{
    BOOL ret = TRUE;
    int i;

    for (i = 0; directory[i]; i++)
    {
        if (i > 0 && directory[i] == '/')
        {
            directory[i] = 0;
            mkdir(directory, 0777);
            directory[i] = '/';
        }
    }
    if (mkdir(directory, 0777) && errno != EEXIST)
       ret = FALSE;

    return ret;
}

/* Modern usage function with comprehensive help */
int usage(char *progname)
{
   printf("AppBundleGenerator - Modern macOS Application Bundle Creator\n");
   printf("Version 2.0 - Target: macOS 12+ (Monterey and later)\n\n");

   printf("Usage:\n");
   printf("  %s [options] BundleName DestinationDir ExecutableOrCommand\n\n", progname);

   printf("Required Arguments:\n");
   printf("  BundleName           Display name for the application (e.g., 'My App')\n");
   printf("  DestinationDir       Directory where .app bundle will be created\n");
   printf("  ExecutableOrCommand  Command or path to execute when launched\n\n");

   printf("Icon Options:\n");
   printf("  --icon PATH          Icon file (PNG, SVG, or ICNS format)\n");
   printf("                       Automatically converts PNG/SVG to .icns\n\n");

   printf("Code Signing Options:\n");
   printf("  --sign IDENTITY      Code signing identity\n");
   printf("                       Use '-' for ad-hoc signing (development only)\n");
   printf("                       Example: 'Developer ID Application: Your Name'\n");
   printf("  --hardened-runtime   Enable hardened runtime (recommended for distribution)\n");
   printf("  --entitlements PATH  Custom entitlements plist file\n");
   printf("  --force-sign         Replace existing signature\n\n");

   printf("Info.plist Options:\n");
   printf("  --identifier ID      Custom bundle identifier\n");
   printf("                       Default: auto-generated from bundle name\n");
   printf("  --min-os VERSION     Minimum macOS version (default: 12.0)\n");
   printf("                       Examples: 12.0, 13.0, 14.0\n");
   printf("  --category TYPE      App category for Gatekeeper\n");
   printf("                       Default: public.app-category.utilities\n");
   printf("                       Other: developer-tools, productivity, graphics-design\n");
   printf("  --version VER        Bundle version (default: 1.0.0)\n\n");

   printf("Entitlement Exceptions (for hardened runtime):\n");
   printf("  --allow-jit          Allow JIT compilation\n");
   printf("  --allow-unsigned     Allow unsigned executable memory\n");
   printf("  --allow-dyld-vars    Allow DYLD environment variables\n\n");

   printf("Other Options:\n");
   printf("  --help, -h           Show this help message\n\n");

   printf("Examples:\n\n");

   printf("  1. Basic bundle (no icon, no signing):\n");
   printf("     %s 'My App' /Applications '/usr/local/bin/myapp'\n\n", progname);

   printf("  2. With PNG icon:\n");
   printf("     %s --icon icon.png 'My App' /Applications '/usr/local/bin/myapp'\n\n", progname);

   printf("  3. With SVG icon and ad-hoc signing:\n");
   printf("     %s --icon icon.svg --sign - --hardened-runtime \\\n", progname);
   printf("       'My App' /Applications '/usr/local/bin/myapp'\n\n");

   printf("  4. Full production build:\n");
   printf("     %s --icon app.svg \\\n", progname);
   printf("       --sign 'Developer ID Application: Your Name' \\\n");
   printf("       --hardened-runtime --allow-dyld-vars \\\n");
   printf("       --identifier com.example.myapp \\\n");
   printf("       --min-os 12.0 \\\n");
   printf("       --category public.app-category.developer-tools \\\n");
   printf("       'My Development Tool' /Applications '/usr/local/bin/devtool'\n\n");

   printf("  5. Terminal launcher example:\n");
   printf("     %s 'Midnight Commander' /Applications \\\n", progname);
   printf("       'open -b com.apple.terminal /usr/local/bin/mc' Terminal.png\n\n");

   printf("Notes:\n");
   printf("  - May require sudo/root depending on destination directory\n");
   printf("  - Icon generation requires macOS utilities: sips, iconutil, qlmanage\n");
   printf("  - Code signing requires valid signing identity in Keychain\n");
   printf("  - Generated bundles are compatible with macOS 12+ (Monterey and later)\n\n");

   printf("Author: Steven Edwards (winehacker@gmail.com)\n");
   printf("License: See source code for licensing terms\n\n");

   return 1;
}

/* Parse command-line arguments using getopt_long */
static struct option long_options[] = {
    {"icon",            required_argument, 0, 'i'},
    {"sign",            required_argument, 0, 's'},
    {"hardened-runtime", no_argument,      0, 'H'},
    {"entitlements",    required_argument, 0, 'e'},
    {"force-sign",      no_argument,       0, 'F'},
    {"identifier",      required_argument, 0, 'I'},
    {"min-os",          required_argument, 0, 'm'},
    {"category",        required_argument, 0, 'c'},
    {"version",         required_argument, 0, 'V'},
    {"allow-jit",       no_argument,       0, 'j'},
    {"allow-unsigned",  no_argument,       0, 'u'},
    {"allow-dyld-vars", no_argument,       0, 'd'},
    {"help",            no_argument,       0, 'h'},
    {0, 0, 0, 0}
};

int parse_arguments(int argc, char *argv[], AppBundleOptions *options)
{
    int c;
    int option_index = 0;

    /* Initialize with defaults */
    memset(options, 0, sizeof(AppBundleOptions));
    options->min_os_version = "12.0";
    options->app_category = "public.app-category.utilities";
    options->version = "1.0.0";

    /* Parse options */
    while ((c = getopt_long(argc, argv, "i:s:e:I:m:c:V:hHFjud",
                           long_options, &option_index)) != -1) {
        switch (c) {
            case 'i': options->icon_path = optarg; break;
            case 's': options->signing_identity = optarg; break;
            case 'H': options->enable_hardened_runtime = TRUE; break;
            case 'e': options->entitlements_file = optarg; break;
            case 'F': options->force_sign = TRUE; break;
            case 'I': options->bundle_identifier = optarg; break;
            case 'm': options->min_os_version = optarg; break;
            case 'c': options->app_category = optarg; break;
            case 'V': options->version = optarg; break;
            case 'j': options->allow_jit = TRUE; break;
            case 'u': options->allow_unsigned_memory = TRUE; break;
            case 'd': options->allow_dyld_vars = TRUE; break;
            case 'h': return usage(argv[0]);
            case '?': /* Unknown option or missing argument */
                fprintf(stderr, "\nTry '%s --help' for more information.\n", argv[0]);
                return 1;
            default:
                return usage(argv[0]);
        }
    }

    /* Parse positional arguments */
    if (argc - optind < 3) {
        fprintf(stderr, "Error: Missing required arguments\n\n");
        return usage(argv[0]);
    }

    options->bundle_name = argv[optind];
    options->bundle_dest = argv[optind + 1];
    options->executable_path = argv[optind + 2];

    /* If 4th positional argument is provided, treat it as icon (backward compatibility) */
    if (argc - optind >= 4 && !options->icon_path) {
        options->icon_path = argv[optind + 3];
    }

    return 0;
}

/* Error handling functions */
void print_error(ErrorCode code, const char *details)
{
    fprintf(stderr, "ERROR: %s", error_code_to_string(code));
    if (details) {
        fprintf(stderr, " - %s", details);
    }
    fprintf(stderr, "\n");
}

const char* error_code_to_string(ErrorCode code)
{
    switch (code) {
        case ERR_SUCCESS:
            return "Success";
        case ERR_INVALID_ARGS:
            return "Invalid arguments";
        case ERR_DIR_CREATION_FAILED:
            return "Failed to create directory structure";
        case ERR_PLIST_GENERATION_FAILED:
            return "Failed to generate Info.plist";
        case ERR_SCRIPT_GENERATION_FAILED:
            return "Failed to generate launcher script";
        case ERR_ICON_CONVERSION_FAILED:
            return "Failed to convert icon";
        case ERR_CODE_SIGNING_FAILED:
            return "Code signing failed";
        case ERR_FILE_NOT_FOUND:
            return "File not found";
        case ERR_PERMISSION_DENIED:
            return "Permission denied";
        default:
            return "Unknown error";
    }
}

int main(int argc, char *argv[])
{
    AppBundleOptions options;
    char *bundle_path = NULL;
    char *temp_entitlements = NULL;
    int ret = 0;

    /* Parse command-line arguments */
    if (parse_arguments(argc, argv, &options) != 0) {
        return 1;
    }

    /* Display configuration (for debugging) */
    printf("Creating app bundle:\n");
    printf("  Name: %s\n", options.bundle_name);
    printf("  Destination: %s\n", options.bundle_dest);
    printf("  Executable: %s\n", options.executable_path);
    if (options.icon_path) {
        printf("  Icon: %s\n", options.icon_path);
    }
    if (options.signing_identity) {
        printf("  Signing: %s%s\n", options.signing_identity,
               options.enable_hardened_runtime ? " (hardened runtime)" : "");
    }
    printf("\n");

    /* Phase 1: Build bundle structure (includes icon if provided) */
    printf("Building bundle structure...\n");
    if (!build_app_bundle(&options)) {
        print_error(ERR_DIR_CREATION_FAILED, "Bundle creation failed");
        return 1;
    }

    printf("Bundle structure created successfully\n");

    /* Calculate bundle path for code signing operations */
    bundle_path = heap_printf("%s/%s.app", options.bundle_dest, options.bundle_name);

    /* Phase 2: Code signing (if requested) */
    if (options.signing_identity) {
        CodeSignOptions sign_opts = {0};

        /* Phase 2a: Generate entitlements if needed */
        if (!options.entitlements_file && options.enable_hardened_runtime) {
            /* Auto-generate entitlements file */
            temp_entitlements = heap_printf("/tmp/appbundle_%d.entitlements", getpid());
            printf("Generating entitlements...\n");

            if (!generate_entitlements_file(temp_entitlements, options.enable_hardened_runtime,
                                          options.allow_jit, options.allow_unsigned_memory,
                                          options.allow_dyld_vars)) {
                print_error(ERR_CODE_SIGNING_FAILED, "Failed to generate entitlements");
                ret = 1;
                goto cleanup;
            }

            sign_opts.entitlements_path = temp_entitlements;
        } else if (options.entitlements_file) {
            sign_opts.entitlements_path = options.entitlements_file;
        }

        /* Phase 2b: Configure signing options */
        sign_opts.identity = options.signing_identity;
        sign_opts.enable_hardened_runtime = options.enable_hardened_runtime;
        sign_opts.force = options.force_sign;
        sign_opts.timestamp = TRUE;  /* Always timestamp for distribution */

        /* Phase 2c: Perform code signing */
        printf("Code signing bundle...\n");
        if (!codesign_bundle(bundle_path, &sign_opts)) {
            print_error(ERR_CODE_SIGNING_FAILED, "Code signing failed");
            ret = 1;
            goto cleanup;
        }

        /* Phase 2d: Verify signature */
        printf("Verifying code signature...\n");
        if (!verify_codesign(bundle_path)) {
            print_error(ERR_CODE_SIGNING_FAILED, "Code signature verification failed");
            ret = 1;
            goto cleanup;
        }

        printf("Code signing completed successfully\n");
    }

    printf("\n====================================\n");
    printf("Bundle created successfully!\n");
    printf("====================================\n");
    printf("Location: %s\n", bundle_path);
    printf("\n");

    if (options.icon_path) {
        printf("Icon: Converted and added\n");
    }

    if (options.signing_identity) {
        printf("Code signing: %s\n", options.signing_identity);
        if (options.enable_hardened_runtime) {
            printf("Hardened runtime: Enabled\n");
        }
    }

    printf("\nYou can now run: open %s\n", bundle_path);

cleanup:
    /* Cleanup */
    if (temp_entitlements) {
        unlink(temp_entitlements);
        free(temp_entitlements);
    }

    if (bundle_path) {
        free(bundle_path);
    }

    return ret;
}
