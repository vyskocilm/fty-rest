/*  =========================================================================
    legacy_asset_server - Server translating legacy configure messages to new protocol

    Copyright (C) 2017 Eaton

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

/*
@header
    bios_legacy_asset_server - Server translating legacy configure messages to new protocol
@discuss
@end
*/
#include "legacy_asset_server.h"
#include "str_defs.h"

int main () {

    bool verbose = false;
    if (streq (::getenv ("BIOS_LOG_LEVEL"), "LOG_DEBUG"))
        verbose = true;

    zactor_t *server = zactor_new (legacy_asset_server, (void*) "legacy-asset-server");
    if (verbose)
        zstr_send (server, "VERBOSE");
    zstr_sendx (server, "CONNECT", MLM_ENDPOINT, NULL);
    zstr_sendx (server, "PRODUCER", bios_get_stream_main (), NULL);
    zstr_sendx (server, "CONSUMER", "ASSETS", ".*", NULL);

    while (true) {
        char *str = zstr_recv (server);
        if (str) {
            puts (str);
            zstr_free (&str);
        }
        else {
            zsys_info ("Interrupted");
            break;
        }
    }

    zactor_destroy (&server);

}
