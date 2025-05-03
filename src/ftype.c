#include "ftype.h"
#include <string.h>
#include <strings.h>

static const mime_map_t mime_types[] = {
    {"html", "text/html"},        {"htm", "text/html"},
    {"css", "text/css"},          {"js", "application/javascript"},
    {"png", "image/png"},         {"jpg", "image/jpeg"},
    {"jpeg", "image/jpeg"},       {"gif", "image/gif"},
    {"ico", "image/x-icon"},      {"svg", "image/svg+xml"},
    {"json", "application/json"}, {"pdf", "application/pdf"},
    {"txt", "text/plain"},        {"woff", "font/woff"},
    {"woff2", "font/woff2"},      {NULL, NULL}};

const char *servc_mime(const char *fpath)
{
    if (fpath == NULL)
        return "application/octet-stream";

    const char *fext = strrchr(fpath, '.');
    if (fext == NULL || *(fext + 1) == '\0')
        return "application/octet-stream";
    fext++; // skip the '.'

    for (int i = 0; mime_types[i].ext != NULL; i++)
    {
        if (strcasecmp(fext, mime_types[i].ext) == 0)
            return mime_types[i].mime;
    }

    return "application/octet-stream";
}
