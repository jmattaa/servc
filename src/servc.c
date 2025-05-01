#include "servc.h"
#include "http.h"
#include "logger.h"
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

static volatile uint8_t running = 1;
static void servc_stop(int _)
{
    running = 0;
    servc_logger_info("Stopping server...\n");
}
static void servc_handle_conn(int cfd);

void servc_run(servc_opts *opts)
{
    signal(SIGINT, servc_stop); // override ctrl+c so we don mem leak

    // AF_INET = IPv4, SOCK_STREAM = TCP, 0 = http protocol
    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd == -1)
    {
        servc_logger_error("unable to create socket: %s\n", strerror(errno));
        return;
    }

    // so that we can reuse the port
    int opt = 1;
    setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(opts->port),
        .sin_addr = {.s_addr = INADDR_ANY},
    };

    if (bind(sfd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    {
        servc_logger_error("unable to bind socket: %s\n", strerror(errno));
        goto cleanup;
    }

    if (listen(sfd, 10) == -1)
    {
        servc_logger_error("unable to listen on socket: %s\n", strerror(errno));
        goto cleanup;
    }

    servc_logger_info("Listening on http://127.0.0.1:%d\n", opts->port);

    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(sfd, &fds);
    int maxfd = sfd;

    while (running)
    {
        fd_set copy = fds;
        if (select(maxfd + 1, &copy, NULL, NULL, NULL) == -1)
        {
            if (errno != EINTR) // if not intr then there is an error
                servc_logger_error("unable to select: %s\n", strerror(errno));
            goto cleanup;
        }

        if (FD_ISSET(sfd, &copy))
        {
            struct sockaddr_in caddr;
            socklen_t caddr_len = sizeof(caddr);
            int cfd = accept(sfd, (struct sockaddr *)&caddr, &caddr_len);
            if (cfd == -1)
            {
                servc_logger_error("unable to accept connection: %s\n",
                                   strerror(errno));
                continue;
            }

            servc_handle_conn(cfd);
            close(cfd);
        }
    }

cleanup:
    close(sfd);
}

static void servc_handle_conn(int cfd)
{
    char b[1024] = {0};

    ssize_t n = recv(cfd, b, sizeof(b) - 1, 0);
    if (n == -1)
    {
        servc_logger_error("Unable to receive data: %s\n", strerror(errno));
        return;
    }

    if (n == 0)
    {
        servc_logger_info("Connection closed by client\n");
        return;
    }

    char *res;
    servc_http *http = servc_http_respond(b, &res);
    servc_logger_info("%s\n", http->startline);
    if (!res)
    {
        servc_logger_error("unable to respond to client\n");
        return;
    }
    if (send(cfd, res, strlen(res), 0) == -1)
    {
        servc_logger_error("Unable to send data: %s\n", strerror(errno));
        return;
    }
}
