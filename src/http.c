#include "http.h"
#include "logger.h"
#include <stdlib.h>
#include <string.h>

static char *http_get(char *targ);

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

servc_http *servc_http_respond(char *req, char **res)
{
    servc_http *http = servc_http_parse(req);
    if (!http)
        return NULL;

    switch (http->meth)
    {
    case SERVC_HTTP_GET:
    {
        *res = http_get(http->targ);
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

static char *http_get(char *targ)
{
    char *res_header = SERV_HTTP_PROTO " 200 OK\r\n"
                                       "Server: servc\r\n"
                                       "Content-Type: text/html\r\n"
                                       "Content-Length: 0\r\n" // TODO fix len
                                       "\r\n";

    // TODO add body

    return strdup(res_header);
}
