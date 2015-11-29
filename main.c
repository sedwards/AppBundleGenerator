#include <stdio.h>


int usage(char *argv[0])
{
        printf( "Usage:\n" );
        printf( "  %s NameOfBundle DirectoryWhereItGoes ExecutableToEmbed PrettyIcon\n", argv[0] );
        printf( "  %s All options are mandantory except for the Icon\n\n", argv[0] );
        printf( "  %s Example:\n", argv[0] );
        printf( "  %s 'Midnight Commander' '/Applications' /usr/local/bin/mc Terminal.png \n\n", argv[0] );
        printf( "  %s May Require sudo/root depending on where you want to drop the bundle\n\n", argv[0] );
        printf( "  %s Questions, Comments and noise can be sent to Steven Edwards (winehacker@gmail.com)\n\n", argv[0] );
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

    int len = 0;
    for (char *c = argv[1]; *c; c++, len++) {
        printf("%d ", (int)(*c));
    }

    printf("\nLength: %d\n", len);

    return 0;
}


