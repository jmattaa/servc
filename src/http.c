#include "http.h"
#include "ftype.h"
#include "logger.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static char *http_get(char *targ, size_t *res_len);

servc_http *servc_http_parse(char *req)
{
    servc_http *http = malloc(sizeof(servc_http));
    if (!http)
        servc_logger_fatal(1, "unable to allocate memory for http request\n");

    // we just care for the first line for now
    // and it looks like this:
    //  <method> <target> <protocol>\r\n
    char *startline = strtok(req, "\r\n");
    if (startline == NULL)
    {
        servc_logger_error("unable to parse http request: %s\n", req);
        free(http);
        return NULL;
    }
    http->startline = strdup(startline);

    const char *method = strtok(startline, " ");
    if (method == NULL)
    {
        servc_logger_error("unable to get method from http request: %s\n", req);
        free(http);
        return NULL;
    }

    if (strcmp(method, "GET") == 0)
        http->meth = SERVC_HTTP_GET;
    else
    {
        servc_logger_error("unsupported http method: %s\n", method);
        free(http);
        return NULL;
    }

    const char *targ = strtok(NULL, " ");
    if (targ == NULL)
    {
        servc_logger_error("unable to get target from http request: %s\n", req);
        free(http);
        return NULL;
    }
    http->targ = strdup(targ);
    if (!http->targ)
    {
        servc_logger_error("failed to allocate memory for target\n");
        free(http);
        return NULL;
    }

    return http;
}

servc_http *servc_http_respond(char *req, char **res, size_t *res_len)
{
    servc_http *http = servc_http_parse(req);
    if (!http)
        return NULL;

    switch (http->meth)
    {
    case SERVC_HTTP_GET:
    {
        *res = http_get(http->targ, res_len);
        return http;
    }
    default:
        servc_logger_error("unsupported http method: %s\n", http->targ);
        break;
    }

    servc_http_destroy(http);
    return NULL;
}

void servc_http_destroy(servc_http *http)
{
    if (http->startline)
        free(http->startline);
    if (http->targ)
        free(http->targ);
    if (http)
        free(http);
}

static const char *notfound =
    SERVC_HTTP_PROTO "404 Not Found\r\n"
                     "Server: servc\r\n"
                     "Content-Type: text/html\r\n"
                     "Content-Length:" SERVC_HTTP_404_MSGLEN "\r\n"
                     "\r\n" SERVC_HTTP_404_MSG;

static char *http_get(char *targ, size_t *res_len)
{
    if (targ == NULL)
    {
        servc_logger_error("Target is NULL\n");
        *res_len = strlen(notfound);
        return strdup(notfound);
    }

    if (strcmp(targ, "/") == 0)
        targ = "index.html";

    if (targ[0] == '/')
        targ++; // skip '/'

    int fd = open(targ, O_RDONLY);
    if (fd == -1)
    {
        servc_logger_error("Failed to open '%s': %s\n", targ, strerror(errno));
        *res_len = strlen(notfound);
        return strdup(notfound);
    }

    struct stat st;
    if (fstat(fd, &st) == -1)
    {
        servc_logger_error("Failed to stat file: %s\n", targ);
        close(fd);
        return NULL;
    }

#define HEADER_SIZE 512
    char header[HEADER_SIZE];
    const char *mime = servc_mime(targ);
    int header_len = snprintf(header, sizeof(header),
                              SERVC_HTTP_PROTO "200 OK\r\n"
                                               "Server: servc\r\n"
                                               "Content-Type: %s\r\n"
                                               "Content-Length: %zu\r\n"
                                               "\r\n",
                              mime, (size_t)st.st_size);

    if (header_len < 0 || header_len >= HEADER_SIZE)
    {
        servc_logger_error("Header generation failed or truncated\n");
        close(fd);
        return NULL;
    }

    size_t total_len = header_len + st.st_size;
    char *res = malloc(total_len);
    if (!res)
    {
        servc_logger_error("Memory allocation failed\n");
        close(fd);
        return NULL;
    }
    memcpy(res, header, header_len);

    ssize_t read_bytes = read(fd, res + header_len, st.st_size);
    if (read_bytes != st.st_size)
    {
        servc_logger_error("File read incomplete: %s\n", targ);
        free(res);
        close(fd);
        return NULL;
    }

    close(fd);
    *res_len = total_len;
    return res;
}
