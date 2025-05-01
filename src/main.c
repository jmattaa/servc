#include "cli.h"
#include <stdio.h>

int main(int argc, char **argv)
{
    servc_opts *opts = servc_cli_parse(argc, argv);
    if (opts->show_help)
    {
        servc_cli_print_help();
        goto clean;
    }

    printf("Port: %d\n", opts->port);
    printf("Dir: %s\n", opts->dir);

clean:
    servc_cli_destroy_opts(opts);
    return 0;
}
