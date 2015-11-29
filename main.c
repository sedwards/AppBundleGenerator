#include <stdio.h>

int debug_print_args(char *argv[])
{
   printf( "%s %s %s %s\n\n", argv[1], argv[2], argv[3], argv[4]);
   return 1;
}

int usage(char *argv[0])
{
   printf( "Usage:\n" );
   printf( "  %s NameOfBundle DirectoryWhereItGoes ExecutableToEmbed PrettyIcon\n", argv[0] );
   printf( "  All options are mandantory except for the Icon\n\n" );
   printf( "  Example:\n" );
   printf( "  %s 'Midnight Commander' '/Applications' /usr/local/bin/mc Terminal.png \n\n", argv[0] );
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

#if 0
    int len = 0;
    for (char *c = argv[1]; *c; c++, len++) {
        printf("%d ", (int)(*c));
    }

    printf("\nLength: %d\n", len);
#endif

    return 0;
}


