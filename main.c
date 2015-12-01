#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <errno.h>

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


int debug_print_args(char *argv[])
{
   printf( "%s %s %s %s\n\n", argv[1], argv[2], argv[3], argv[4]);
   return 1;
}

int usage(char *argv[0])
{
   printf( "Usage:\n" );
   printf( "  %s NameOfBundle DirectoryWhereItGoes ExecutableOrCommandToEmbed PrettyIcon\n", argv[0] );
   printf( "  All options are mandantory except for the Icon\n\n" );
   printf( "  Example:\n" );
   printf( "  %s 'Midnight Commander' /Applications 'open -b com.apple.terminal /usr/local/bin/mc' Terminal.png \n\n", argv[0] );
   printf( "  May Require sudo/root depending on where you want to drop the bundle\n" );
   printf( "  Questions, Comments and noise can be sent to Steven Edwards (winehacker@gmail.com)\n\n" );
   return 1;
}

int main(int argc, char *argv[])
{
    int ret;
    
    if ((argc < 4) || (argc > 6)) {
        printf("invalid number of arguments\n");
        usage(argv);
        return 1;
    }

    debug_print_args(argv);

    build_app_bundle(argv[3], argv[2], NULL, argv[1]);

    if (argv[4] == NULL)
    {
        /* Missing an ICON is not fatal, just annoying */
        printf("Source icon not given. You bundle is going to look pretty lame\n\n");
    }  else {
        printf(" Sorry AppBundle Icon generation is not yet implemented in this code.\n");
        printf(" You can manually add your icon after bundle creation by running: convert_svg_to_icns.sh %s", argv[4]);
        printf(" Then copy the resulting icon.icns file to Bundle/Contents/Resources/icon.icns \n");
    }

    return 0;
}


