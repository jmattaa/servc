#include "cli.h"
#include "servc.h"
#include <stdio.h>

int main(int argc, char **argv)
{
    servc_opts *opts = servc_cli_parse(argc, argv);
    if (opts->show_help)
    {
        servc_cli_print_help();
        goto clean;
    }
    if (opts->show_version)
    {
        printf("servc %s\n", SERVC_VERSION);
        goto clean;
    }

    servc_run(opts);

clean:
    servc_cli_destroy_opts(opts);
    return 0;
}
