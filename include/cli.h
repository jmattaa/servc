#ifndef SERVC_CLI_H
#define SERVC_CLI_H

#include <stdint.h>
#define SERVC_VERSION "0.1.0"

typedef struct
{
    const char *dir;
    int port;
    uint8_t show_help : 1;
    uint8_t verbose : 1;
    uint8_t show_version : 1;
} servc_opts;

servc_opts *servc_cli_parse(int argc, char **argv);
void servc_cli_print_help(void);
void servc_cli_destroy_opts(servc_opts *opts);

#endif
