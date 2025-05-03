#ifndef SERVC_HTTP_H
#define SERVC_HTTP_H

#include <stddef.h>
typedef enum 
{
    SERVC_HTTP_GET,
    // SERVC_HTTP_POST,
    // there is a ton of other methods but idk if we need to implement them
} servc_http_method;

typedef struct 
{
    char *startline;
    servc_http_method meth; 
    char *targ;
    // protocol; but we be just accepting HTTP/1.1
    // headers and body, but idk if we gotta have them, cuz we just want to 
    // statically serve files
} servc_http;

#define SERVC_HTTP_PROTO "HTTP/1.1"

#define SERVC_HTTP_404_MSG "404 Not Found\n"
#define SERVC_HTTP_404_MSGLEN "15"

servc_http *servc_http_parse(char *req);
servc_http *servc_http_respond(char *req, char **res, size_t *res_len);
void servc_http_destroy(servc_http *http);

#endif
