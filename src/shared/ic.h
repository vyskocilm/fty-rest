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

#ifndef _IC_H_INCLUDED
#define _IC_H_INCLUDED

#include <czmq.h>

#ifdef __cplusplus
extern "C" {
#endif

// Converts buffer buf of bytes into other encoding
//
// You have to free returned string yourself
// returns pointer to converted text
// if outlen != NULL bytes in returned string is set.
// This is usefull for multibyte encoding.
char *ic_convert(char *buf, size_t bytes, const char *from, const char *to, size_t *outlen);

// transliterate from utf-8 into ascii
// you have to call setlocale() function before
// first call of iconv_utf8_to_ascii
// good example is setlocale (LC_ALL, "");
// to reflect system locales settings
//
// You have to free returned string yourself
char *ic_utf8_to_ascii (char *string);

// convert user defined asset name to something
// readable but simple and asscii. Output is limited
// to first 40 characters. Asset type is appended to
// the name ("mydevice-ups" is produced for example).
//
// if transliteration to ascii returns empty string,
// assettype is returned
//
// See note about setlocale before.
//
// You have to free returned string yourself
char *ic_utf8_to_name (char *string, const char *assettype);

#ifdef __cplusplus
}
#endif

#endif
