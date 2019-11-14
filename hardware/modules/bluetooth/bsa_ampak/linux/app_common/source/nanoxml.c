/*****************************************************************************
 **
 **  Name:           nanoxml.c
 **
 **  Description:    Bluetooth Services functions
 **
 **  Copyright (c) 2004-2011, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/

#include "nanoxml.h"
#include <stdlib.h>
#include <string.h>

//#include "cms_util.h"
#ifdef DMALLOC
#include "dmalloc.h"
#endif

#define ALLOC_ZEROIZE 0

void *cmsMem_alloc(int size, int flag)
{
    return malloc(size);
}

void cmsMem_free(void *ptr)
{
    free(ptr);
}

int xmlOpen(nxml_t *handle, const nxml_settings *settings)
{
    if ((*handle = (nxml_t)cmsMem_alloc(sizeof(**handle), ALLOC_ZEROIZE)) != NULL) {
        (*handle)->settings = *settings;
        (*handle)->namecachesize = 0;
        (*handle)->treelevel = 0;
        (*handle)->skipwhitespace = 1;
        (*handle)->state = state_start;
        return 1;
    }
    return 0;
}

void xmlClose(nxml_t handle)
{
    cmsMem_free(handle);
}

#define WHITESPACE " \t\r\n"
#define strskip(DATA,CHARS) ((const char *)(strspn(DATA,CHARS) + (DATA)))

/* Append data to the namecache. This is used for tag and attribute names.
If the nocopy parameters are specified, then the caller regards the name
as complete, and if there's nothing already in the cache, then there's no
need for a copy. */
static void nxml_add_namecache(nxml_t handle, char *data, int len,
                               char **data_nocopy, unsigned *len_nocopy)
{
    /* if the nocopy parameters are supplied, and there's nothing in the cache,
    then don't copy. just pass them back. */
    if (data_nocopy && !handle->namecachesize) {
        *data_nocopy = data;
        *len_nocopy = len;
    } else {
        if (len > NXML_MAX_NAME_SIZE - handle->namecachesize)
            len = NXML_MAX_NAME_SIZE - handle->namecachesize;
        if (len) {
            strncpy(&handle->namecache[handle->namecachesize], data, len);
            handle->namecachesize += len;
        }
        if (data_nocopy) {
            *data_nocopy = handle->namecache;
            *len_nocopy = handle->namecachesize;
        }
    }
}
/*
* Returns: *endp to end of parsed data.
*   1 : OK - parse complete
 *  0 : Out of data - parse end tag not found
 */

int xmlWrite(nxml_t handle, char *data, unsigned len, char **endp)
{
    int treeEnd=0;
    const char *enddata = data + len;

//    printf(" ---- nxml_write(): len %d",len);
    if (handle->state == state_start) {
        char *p;
//        printf(" ---- skip <?xml header ");
        p =  (char*) strskip(data, WHITESPACE);
        if ( !strncmp(p,"<?",2) ) {
            p = strstr( p, "?>");
            if (p)
                data = p+2;
        }
        handle->state = state_begin_tag;
    }

    while ((data < enddata) && (!treeEnd)) {
        char *s; /* temp value for capturing search results */

        /* skip whitespace */
        if (handle->skipwhitespace) {
            s = (char*) strskip(data, WHITESPACE);
            if (s != data) {
                data = s;
                continue;
            }
        }

//        cmsLog_debug(" ---- state %d, %c, %d", handle->state, *data, handle->namecachesize);

        switch (handle->state) {
        case state_begin_tag:
            s = strchr(data, '<');
            if (!s) {
                /* it's all data */
                if ( handle->settings.data )
                    (*handle->settings.data)(handle, data, enddata-data, 1/*maybe more data*/);
                *endp = data;
                return 0;
            } else if ( (data != s) && handle->settings.data) {
                /* we have some data, then a tag */
                (*handle->settings.data)(handle, data, s-data, 0/*end of data*/);
            }
            /* skip over the tag begin and process the tag name */
            data = s+1;
            handle->state = state_tag_name;
            handle->namecachesize = 0;
            ++(handle->treelevel);
            break;

        case state_finish_tag:
            /* we don't care about anything but the end of a tag */
            s = strchr(data, '>');
            if (!s) {
                *endp = data;
                return 0;
            }

            /* we found it, so start looking for the next tag */
            data = s+1;
            handle->state = state_begin_tag;
            if ( --(handle->treelevel) <= 0 )
                treeEnd = 1;
            break;

        case state_end_tag_name:
            s = strpbrk(data, WHITESPACE ">");
            if (!s) {
                /* it's all name, and we're not done */
                nxml_add_namecache(handle, data, enddata-data, NULL, NULL);
                handle->skipwhitespace = 0;
                *endp = data;
                return 0;
            } else {
                char *name;
                unsigned namelen;
                nxml_add_namecache(handle, data, s-data, &name, &namelen);

                (*handle->settings.tag_end)(handle, name, namelen);
                handle->state = state_finish_tag;
                data = s;
                if (--(handle->treelevel) <=0)
                    treeEnd = 1;
            }
            break;

        case state_tag_name:
        case state_attr_name:
            if (*data == '/') {
                /* this tag is done */
                if (handle->state == state_tag_name && !handle->namecachesize) {
                    /* we can still parse the end tag name so that the uppperlevel app
                    can validate if it cares */
                    handle->state = state_end_tag_name;
                    data++;
                    break;
                } else if (handle->state == state_attr_name) {
                    /* we had an attribute, so this tag is just done */
                    (*handle->settings.tag_end)(handle, handle->namecache, handle->namecachesize);
                    handle->state = state_finish_tag;
                    data++;
                    break;
                }
            } else if (*data == '>') {
                handle->state = state_begin_tag;
                data++;
                break;
            }

            /* TODO: is = a valid in a tag? I don't think so. */
            s = (char *)strpbrk(data, WHITESPACE "=/>");
            if (!s) {
                /* it's all name, and we're not done */
                nxml_add_namecache(handle, data, enddata-data, NULL, NULL);
                handle->skipwhitespace = 0;
                *endp = data;
                return 0;
            } else {
                /* we have the entire name */
                char *name;
                unsigned namelen;
                nxml_add_namecache(handle, data, s-data, &name, &namelen);

                if (handle->state == state_tag_name) {
                    (*handle->settings.tag_begin)(handle, name, namelen);
                    handle->state = state_attr_name;
                } else {
                    (*handle->settings.attribute_begin)(handle, name, namelen);
                    handle->state = state_attr_value_equals;
                }
                handle->namecachesize = 0;
                data = s;
            }
            break;

        case state_attr_value:
            s = (char *)strchr(data, '"');
            if (!s) {
                /* it's all attribute_value, and we're not done */
                (*handle->settings.attribute_value)(handle, data, enddata-data, 1);
                handle->skipwhitespace = 0;
                *endp = data;
                return 0;
            } else {
                /* we have some value data, then a tag */
                (*handle->settings.attribute_value)(handle, data, s-data, 0);
            }
            /* skip over the quote and look for more attributes */
            data = s+1;
            handle->state = state_attr_name;
            handle->namecachesize = 0;
            break;


        case state_attr_value_equals:
            if (*data == '>') {
                handle->state = state_begin_tag;
                data++;
            } else if (*data == '=') {
                handle->state = state_attr_value_quote;
                data++;
            } else
                handle->state = state_attr_name;
            break;

        case state_attr_value_quote:
            if (*data == '"')
                data++;
            handle->state = state_attr_value;
            break;
        default:
            break;
        }
        handle->skipwhitespace = 1;
    }  /* while () ... */
    *endp = data;
    return treeEnd? 1: 0;
}


