/*  =========================================================================
    bios_legacy_asset_server - Server translating legacy configure messages to new protocl

    Copyright (C) 2014 - 2015, 2017 Eaton

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
    bios_legacy_asset_server - Server translating legacy configure messages to new protocl
@discuss
@end
*/

#ifndef __BIOS_LEGACY_ASSET_SERVER__
#define __BIOS_LEGACY_ASSET_SERVER__

#include <czmq.h>
#include <malamute.h>
#include <bios_agent.h>
#include <agents.h>
#include <ftyproto.h>
#include <string>
#include <algorithm>

extern "C" {

void
    legacy_asset_server (zsock_t *pipe, void *args);

void
    legacy_asset_server_test (bool verbose);
}

#endif //__BIOS_LEGACY_ASSET_SERVER__
