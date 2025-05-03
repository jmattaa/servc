#include "servc.h"
#include "http.h"
#include "logger.h"
#include "main.h"
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

static void *servc_handle_conn(void *arg);

void servc_run()
{
    // so we do the ctrl-c thing good
    struct sigaction sa;
    sa.sa_handler = servc_stop;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd == -1)
    {
        servc_logger_error("unable to create socket: %s\n", strerror(errno));
        return;
    }

    int opt = 1;
    setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(sopts->port),
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

    servc_logger_info("Listening on http://127.0.0.1:%d\n", sopts->port);

    while (running)
    {
        struct sockaddr_in caddr;
        socklen_t caddr_len = sizeof(caddr);
        int cfd = accept(sfd, (struct sockaddr *)&caddr, &caddr_len);
        if (cfd == -1)
        {
            if (errno == EINTR)
            {
                break; // Interrupted by SIGINT — graceful shutdown
            }
            servc_logger_error("unable to accept connection: %s\n",
                               strerror(errno));
            continue;
        }

        pthread_t tid;
        int *cfd_ptr = malloc(sizeof(int));
        if (!cfd_ptr)
        {
            servc_logger_error("malloc failed\n");
            close(cfd);
            continue;
        }
        *cfd_ptr = cfd;

        if (pthread_create(&tid, NULL, servc_handle_conn, cfd_ptr) != 0)
        {
            servc_logger_error("unable to create thread: %s\n",
                               strerror(errno));
            close(cfd);
            free(cfd_ptr);
            continue;
        }

        pthread_detach(tid);
    }

cleanup:
    close(sfd);
}

static void *servc_handle_conn(void *arg)
{
    int cfd = *((int *)arg);
    free(arg);

    char b[1024] = {0};

    ssize_t n = recv(cfd, b, sizeof(b) - 1, 0);
    if (n == -1)
    {
        servc_logger_error("Unable to receive data: %s\n", strerror(errno));
        close(cfd);
        return NULL;
    }

    if (n == 0)
    {
        close(cfd);
        return NULL;
    }

    char *btemp = strdup(b);
    char *res;

    size_t res_len;
    servc_http *http = servc_http_respond(b, &res, &res_len);

    if (sopts->verbose)
        servc_logger_info("%s\n", btemp);
    else
        servc_logger_info("%s\n", http->startline);

    free(btemp);
    servc_http_destroy(http);

    if (!res)
    {
        servc_logger_error("unable to respond to client\n");
        close(cfd);
        return NULL;
    }

    if (send(cfd, res, res_len, 0) == -1)
        servc_logger_error("Unable to send data: %s\n", strerror(errno));

    close(cfd);
    free(res);
    return NULL;
}
