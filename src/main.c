#include "cli.h"
#include "logger.h"
#include "servc.h"
#include <stdio.h>

servc_opts *sopts;

int main(int argc, char **argv)
{
    sopts = servc_cli_parse(argc, argv);
    servc_logger_info("servc v%s\n", SERVC_VERSION);

    if (sopts->show_help)
    {
        servc_cli_print_help();
        goto clean;
    }

    if (sopts->verbose)
        servc_logger_info("Starting verbose mode...\n");
    servc_run();

clean:
    servc_cli_destroy_opts(sopts);
    return 0;
}
