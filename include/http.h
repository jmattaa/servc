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

#define SERVC_HTTP_PROTO "HTTP/1.1 "

#define SERVC_HTTP_404_MSG                                                     \
    "<!DOCTYPE html><html lang=\"en\"><head><meta charset=\"UTF-8\"><meta "    \
    "name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"     \
    "<title>404 Not "                                                          \
    "Found</"                                                                  \
    "title><style>body{margin:0;font-family:sans-serif;background:#f8f9fa;"    \
    "display:flex;justify-content:center;align-items:center;height:100vh;"     \
    "color:#333}"                                                              \
    ".container{text-align:center;padding:20px}.title{font-size:72px;font-"    \
    "weight:bold}.desc{color:#666;font-size:18px}</style></head>"              \
    "<body><div class=\"container\"><div class=\"title\">404</div><div "       \
    "class=\"desc\">man you prolly a developer how didn you manage? jk man "   \
    "you good we all do mistakes</div></div></body></html>"

#define SERVC_HTTP_404_MSGLEN "610"

servc_http *servc_http_parse(char *req);
servc_http *servc_http_respond(char *req, char **res, size_t *res_len);
void servc_http_destroy(servc_http *http);

#endif
