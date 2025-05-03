#ifndef SERVC_FTYPE_H
#define SERVC_FTYPE_H

typedef struct
{
    const char *ext;
    const char *mime;
} mime_map_t;

const char *servc_mime(const char *fpath);

#endif
