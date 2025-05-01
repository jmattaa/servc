#include "cli.h"
#include "servc.h"
#include <stdio.h>

servc_opts *sopts;

int main(int argc, char **argv)
{
    sopts = servc_cli_parse(argc, argv);
    if (sopts->show_help)
    {
        servc_cli_print_help();
        goto clean;
    }
    if (sopts->show_version)
    {
        printf("servc %s\n", SERVC_VERSION);
        goto clean;
    }

    servc_run();

clean:
    servc_cli_destroy_opts(sopts);
    return 0;
}
