// TODO: many 404 messages should be 500!!!!!! but yo no orka
#include "http.h"
#include "ftype.h"
#include "logger.h"
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define HEADER_SIZE 512

static char *http_get(const char *targ, size_t *res_len);
static char *http_getfile(struct stat st, const char *targ, size_t *res_len);
static char *http_getdir(struct stat st, const char *targ, size_t *res_len);

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
                     "Content-Type: text/plain\r\n"
                     "Content-Length:" SERVC_HTTP_404_MSGLEN "\r\n"
                     "\r\n" SERVC_HTTP_404_MSG;

static char *http_get(const char *targ, size_t *res_len)
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

    struct stat st;
    if (stat(targ, &st) == -1)
    {
        servc_logger_error("Failed to stat file: %s\n", targ);
        *res_len = strlen(notfound);
        return strdup(notfound);
    }
    if (S_ISDIR(st.st_mode))
        return http_getdir(st, targ, res_len);
    return http_getfile(st, targ, res_len);
}

static char *http_getfile(struct stat st, const char *targ, size_t *res_len)
{
    int fd = open(targ, O_RDONLY);
    if (fd == -1)
    {
        servc_logger_error("Failed to open '%s': %s\n", targ, strerror(errno));
        *res_len = strlen(notfound);
        return strdup(notfound);
    }

    // todo we repeat this header string again in getdir FIX IT!!!!!
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

static char *http_getdir(struct stat st, const char *targ, size_t *res_len)
{
    // kinda like https://github.com/jmattaa/laser
    // the function called laser_process_entries in src/laser.c

    DIR *dir = opendir(targ);
    if (dir == NULL)
    {
        servc_logger_error("Failed to open directory: %s\n", targ);
        *res_len = strlen(notfound);
        return strdup(notfound);
    }

    struct dirent *ent = malloc(sizeof(struct dirent));
    if (!ent)
    {
        servc_logger_error("Memory allocation failed\n");
        closedir(dir);
        *res_len = strlen(notfound);
        return strdup(notfound);
    }

    char *res = malloc(1);
    if (!res)
    {
        servc_logger_error("Memory allocation failed\n");
        free(ent);
        closedir(dir);
        *res_len = strlen(notfound);
        return strdup(notfound);
    }
    char *enttemp = "<li><a href=\"%s\">%s</a></li>\r\n";
    while ((ent = readdir(dir)) != NULL)
    {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
            continue;

        res = realloc(res, strlen(res) + strlen(enttemp) +
                               (strlen(ent->d_name) * 2) + 1);
        if (!res)
        {
            servc_logger_error("Memory allocation failed\n");
            free(ent);
            closedir(dir);
            *res_len = strlen(notfound);
            return strdup(notfound);
        }

        snprintf(res + strlen(res), strlen(enttemp) + strlen(ent->d_name) + 1,
                 enttemp, ent->d_name, ent->d_name);
    }

    free(ent);
    closedir(dir);

    if (res == NULL)
    {
        servc_logger_error("Failed to read directory: %s\n", targ);
        *res_len = strlen(notfound);
        return strdup(notfound);
    }

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
        free(res);
        return NULL;
    }

    size_t total_len = header_len + strlen(res);
    res = realloc(res, total_len);
    if (!res)
    {
        servc_logger_error("Memory allocation failed\n");
        return NULL;
    }
    memcpy(res, header, header_len);
    *res_len = total_len;
    return res;
}
