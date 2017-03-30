/*  =========================================================================
    fty_asset_uptime_configurator - Configuration for kpi-uptime

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

/*
@header
    fty_asset_uptime_configurator - Configuration for kpi-uptime
@discuss
@end
*/
#include "dbpath.h"
#include "dbhelpers.h"
#include "defs.h"
#include <tntdb/connect.h>
#include <tntdb/result.h>
#include <tntdb/row.h>
#include <tntdb/error.h>
#include <exception>
#include <functional>
#include <czmq.h>

// returnns map with desired assets and their ids
db_reply <std::map <uint32_t, std::string> >
    select_short_elements
        (tntdb::Connection &conn,
         uint32_t type_id,
         uint32_t subtype_id)
{
    zsys_debug ("  type_id = %" PRIi16, type_id);
    zsys_debug ("  subtype_id = %" PRIi16, subtype_id);
    std::map <uint32_t, std::string> item{};
    db_reply <std::map <uint32_t, std::string> > ret = db_reply_new(item);

    std::string query;
    if ( subtype_id == 0 )
    {
        query = " SELECT "
                "   v.name, v.id "
                " FROM "
                "   v_bios_asset_element v "
                " WHERE "
                "   v.id_type = :typeid ";
    }
    else
    {
        query = " SELECT "
                "   v.name, v.id "
                " FROM "
                "   v_bios_asset_element v "
                " WHERE "
                "   v.id_type = :typeid AND "
                "   v.id_subtype = :subtypeid ";
    }
    try {
        // Can return more than one row.
        tntdb::Statement st = conn.prepareCached(query);

        tntdb::Result result;
        if ( subtype_id == 0 )
        {
            result = st.set("typeid", type_id).
                    select();
        } else {
            result = st.set("typeid", type_id).
                    set("subtypeid", subtype_id).
                    select();
        }

        // Go through the selected elements
        for (auto const& row: result) {
            std::string name;
            row[0].get(name);
            uint32_t id = 0;
            row[1].get(id);
            ret.item.insert(std::pair<uint32_t, std::string>(id, name));
        }
        ret.status = 1;

        return ret;
    }
    catch (const std::exception &e) {
        ret.status        = 0;
        ret.errtype       = DB_ERR;
        ret.errsubtype    = DB_ERROR_INTERNAL;
        ret.msg           = e.what();
        ret.item.clear();
        zsys_error ("Exception caught %s", e.what());
        return ret;
    }
}

//
int
    select_assets_by_container_
        (tntdb::Connection &conn,
         uint32_t element_id,
         std::string asset_name,
         std::vector<uint32_t> types,
         std::vector<uint32_t> subtypes,
         std::function<void(const tntdb::Row&)> cb
         )
{
    zsys_debug ("container element_id = %" PRIu32, element_id);

    try {
        std::string select =
            " SELECT "
            "   v.name, "
            "   v.id_asset_element as asset_id, "
            "   v.id_asset_device_type as subtype_id, "
            "   v.type_name as subtype_name, "
            "   v.id_type as type_id "
            " FROM "
            "   v_bios_asset_element_super_parent v "
            " WHERE "
            "   :containerid in (v.id_parent1, v.id_parent2, v.id_parent3, v.id_parent4, v.id_parent5)";
        if (!subtypes.empty()) {
            std::string list;
            for( auto &id: subtypes) list += std::to_string(id) + ",";
            select += " and v.id_asset_device_type in (" + list.substr(0,list.size()-1) + ")";
        }
        if (!types.empty()) {
            std::string list;
            for( auto &id: types) list += std::to_string(id) + ",";
            select += " and v.id_type in (" + list.substr(0,list.size()-1) + ")";
        }
        // Can return more than one row.
        tntdb::Statement st = conn.prepareCached (select);

        tntdb::Result result = st.set("containerid", element_id).
                                  select();
        zsys_debug("[v_bios_asset_element_super_parent]: were selected %" PRIu32 " rows",
                                                            result.size());
        for ( auto &row: result ) {
            std::string _asset_name;
            row["name"].get (_asset_name);
            if (streq (_asset_name.c_str(), asset_name.c_str ()))
            {
                for ( auto &row: result ) {
                    cb(row);
                }
                return 0;
            }
        }

        return 1;
    }
    catch (const std::exception& e) {
        zsys_error ("Exception caught: select_assets_by_container %s", e.what());
        return -1;
    }
}

int
    select_assets_by_container_
        (tntdb::Connection &conn,
         uint32_t element_id,
         std::string &asset_name,
         std::function<void(const tntdb::Row&)> cb
         )
{
    return select_assets_by_container_(conn, element_id, asset_name, {}, {}, cb);
}

bool
    get_dc_upses
(std::map <std::string, std::vector <std::string>>& dc_upses, std::string &asset_name)
{
    try {
        tntdb::Connection conn = tntdb::connectCached (url);

        std::vector <std::string> container_upses{};
        std::function<void(const tntdb::Row&)> func = \
            [&container_upses](const tntdb::Row& row)
            {
                uint32_t type_id = 0;
                row["type_id"].get(type_id);

                std::string device_type_name = "";
                row["subtype_name"].get(device_type_name);

                if ( ( type_id == 6 ) && ( device_type_name == "ups" ) )
                {
                    std::string device_name = "";
                    row["name"].get(device_name);

                    container_upses.push_back(device_name);
                }
            };

        // select dcs and thei IDs
        db_reply <std::map <uint32_t, std::string> > reply =
            select_short_elements (conn, 2, 11);
        if (reply.status == 0) {
            zsys_error ("Cannot select datacenters");
            return false;
        }
        // for each DC save its devices (/upses...done by fun)
        for (const auto& dc : reply.item )
        {
            int rv = select_assets_by_container_ (conn, dc.first, asset_name, func);

            if (rv == 0) {
                dc_upses.emplace (dc.second, container_upses);
                break;
            }
        }
        container_upses.clear();
        conn.close ();
    }
    catch (const tntdb::Error& e) {
        zsys_error ("Database error: %s", e.what());
        return false;
    }
    catch (const std::exception& e) {
        zsys_error ("Error: %s", e.what());
        return false;
    }
    return true;
}

bool
    insert_upses_to_aux (zhash_t *aux, std::string asset_name)
{
    assert (aux);

    std::map<std::string, std::vector <std::string>> dc_upses;
    get_dc_upses(dc_upses, asset_name);

    for (auto& item : dc_upses)
    {
        int i = 0;
        char key[10];
        for (auto& ups_it : item.second)
        {
            sprintf (key,"ups%d", i);
            zhash_insert (aux, key, (void*) ups_it.c_str());
            i++;
        }
    }
    return true;
}

// helper for testing
bool
    list_zhash_content (zhash_t *zhash)
{
    for (void *it = zhash_first (zhash);
         it != NULL;
	     it = zhash_next (zhash))
    {
        zsys_debug ("--->list: %s\n", (char*) it);
    }
    return true;
}
