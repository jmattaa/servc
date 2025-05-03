#include "cli.h"
#include "logger.h"
#include <errno.h>
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define S_ARGS_ITER(_X)                                                        \
    _X("help", 0, 0, 'h', "Show help message")                                 \
    _X("port", 0, 0, 'p',                                                      \
       "Port number to use defaults to (6969). (Usage: -p6969 or "             \
       "--port=6969)")                                                         \
    _X("verbose", 0, 0, 'v',                                                   \
       "Show the whole request instead of just the start line")

#define _X(name, a, b, short, ...) {name, a, b, short},
static const struct option long_args[] = {S_ARGS_ITER(_X){0, 0, 0, 0}};
#undef _X
#define _X(name, a, b, short, description) description,
static const char *descriptions[] = {S_ARGS_ITER(_X)};
#undef _X

servc_opts *servc_cli_parse(int argc, char **argv)
{
    servc_opts *opts = malloc(sizeof(servc_opts));

    // dir is set below defaults to "."
    opts->port = 6969;
    opts->show_help = 0;
    opts->verbose = 0;

    const char *dir = ".";

    int c;
    while ((c = getopt_long(argc, argv, "hvp::", long_args, NULL)) != -1)
    {
        switch (c)
        {
        case 'p':
            errno = 0;
            if (optarg == NULL)
                break;

            opts->port = strtol(optarg, NULL, 10);
            if (errno != 0 || (opts->port < 0 || opts->port > 65535))
                servc_logger_fatal(1, "Invalid port number: %s\n", optarg);
            break;
        case 'h':
            opts->show_help = 1;
            break;
        case 'v':
            opts->verbose = 1;
            break;
        default:
            break;
        }
    }

    if (optind < argc)
        dir = argv[optind];
    opts->dir = dir;

    return opts;
}

void servc_cli_print_help(void)
{
    printf("Usage: servc [OPTIONS] [DIRECTORY]\n");
    printf("Options:\n");
    for (int i = 0; long_args[i].name != NULL; i++)
    {
        if (long_args[i].val != 1)
            printf("  -%c, --%-28s %s\n", long_args[i].val, long_args[i].name,
                   descriptions[i]);
        else
            printf("      --%-28s %s\n", long_args[i].name, descriptions[i]);
    }
}

void servc_cli_destroy_opts(servc_opts *opts) { free(opts); }
