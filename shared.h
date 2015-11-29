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

#if 0
#ifdef DEBUG
# define DEBUG_PRINT(x) printf x
#else
# define DEBUG_PRINT(x) do {} while (0)
#endif
Use it like:

DEBUG_PRINT(("var1: %d; var2: %d; str: %s\n", var1, var2, str));
#endif

#ifdef DEBUG
#define DEBUG_PRINT(...) do{ fprintf( stderr, __VA_ARGS__ ); } while( false )
#else
#define DEBUG_PRINT(...) do{ } while ( false )
#endif

/* build out the directory structure for the bundle and then populate */
BOOL build_app_bundle(const char *path, char *bundle_dst, const char *args, const char *linkname);

#endif
