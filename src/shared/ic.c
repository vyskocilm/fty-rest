/*  =========================================================================
    iconv simplified interface

    Copyright (C) 2014 - 2017 Eaton                                        
                                                                           
    This program is free software; you can redistribute it and/or modify   
    it under the terms of the GNU General Public License as published by   
    the Free Software Foundation; either version 2 of the License, or      
    (at your option) any later version.                                    
                                                                           
    This program is distributed in the hope that it will be useful,        
    but WITHOUT ANY WARRANTY; without even the implied warranty of         
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          
    GNU General Public License for more details.                           
                                                                           
    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.            
    =========================================================================
*/

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <iconv.h>
#include <ctype.h>
#include "ic.h"

static
int s_convert(iconv_t id, char *inbuf, size_t *bytes, char *outbuf, size_t *capacity)
{
    size_t converted;

    while (*bytes) {
        converted = iconv (id, &inbuf, bytes, &outbuf, capacity);
        if (converted == (size_t) -1) {
            return errno;
        }
    }
    return 0;
}

char *ic_convert(char *buf, size_t bytes, const char *from, const char *to, size_t *outsize)
{
    if (outsize) *outsize = 0;
    if (!buf || !bytes || !from) return NULL;
    
    iconv_t iconv_cd;

    if ((iconv_cd = iconv_open(to, from)) == (iconv_t) -1) {
        return NULL;
    }
    
    size_t allocated = bytes;
    
    char *out = (char *) malloc (allocated);
    if (! out) {
        iconv_close (iconv_cd);
        return NULL;
    }
    size_t res = 0;
    size_t outlen;

    while (1) {
        char *inbuf = buf;
        char *outbuf = out;
        size_t inlen = bytes;
        outlen = allocated;
        res = s_convert (iconv_cd, inbuf, &inlen, outbuf, &outlen);
        if (res == 0) {
            break;
        }
        if (errno == E2BIG) {
            // output buffer is short
            allocated += bytes;
            char *p = (char *)realloc (out, allocated);
            if (!p) {
                free (out);
                out = NULL;
                break;
            }
            out = p;
        } else {
            // conversion error
            free (out);
            out = NULL;
            break;
        }
    }
    iconv_close(iconv_cd);
    if (outsize) *outsize = allocated - outlen;
    return out;
}

char *ic_utf8_to_ascii (char *string)
{
    return ic_convert (string, strlen (string)+1, "UTF-8", "ASCII//TRANSLIT", NULL);
}

char *ic_utf8_to_name (char *string, const char *prefix)
{
    char *ascii = ic_utf8_to_ascii (string);
    if (!ascii || strlen (ascii) == 0) {
        zstr_free (&ascii);
        return zsys_sprintf ("%s-%d", prefix, time (NULL));
    }

    char *name = (char *) malloc (strlen (ascii) + 1);
    if (!name) {
        free (ascii);
        return NULL;
    }
    
    int i = 0;
    int j = 0;
    // name max size 40 -- keep space for -id
    while (j < 40) {
        if (ascii [i] == '\0') {
            // end of input string
            break;
        }
        if (
            (ascii [i] >= 'a' && ascii [i] <= 'z') ||
            (ascii [i] >= 'A' && ascii [i] <= 'Z') ||
            (ascii [i] >= '0' && ascii [i] <= '9')
        ){
            // allowed characters (letters, numbers)
            name [j] = tolower(ascii [i]);
            ++j;
        }
        else {
            // replaced characters
            if (strchr ("_- /\\.", ascii [i])) {
                name [j++] = '.';
            }
        }
        ++i;
    }
    name [j] = '\0';
    free (ascii);
    if (!name || strlen (name) == 0) {
        zstr_free (&name);
        return zsys_sprintf ("%s-%d", prefix, time (NULL));
    }
    return name;
}


