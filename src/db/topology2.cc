/*
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
*/

#include <czmq.h>
#include <tntdb.h>
#include <algorithm>
#include <exception>
#include <cxxtools/split.h>
#include <tntdb/error.h>
#include <tntdb.h>
#include <sstream>
#include <cxxtools/serializationinfo.h>
#include <cxxtools/jsondeserializer.h>
#include <cxxtools/jsonserializer.h>

#include "asset_types.h"

/**
 *  topologyv2.cc
 *
 *  next generation location_from DB layer
 *
 *  TODO:
 *  1. support for unlocated elements
 *  2. proper REST API reply
 *  3. ... pass the tests ...
 *
 */

namespace persist {

static std::string
s_mkspc (int level);

class NodeMap {

    public:

        NodeMap ():
            _map {}
        {}

        void add (const std::string &name, const std::string &child) {
            if (_map.count (name) == 0) {
                std::set <std::string> s {};
                _map.emplace (std::make_pair (name, s));
            }
            _map [name].insert (child);
        }

        void print (const std::string &name, int level) {
            std::cout << s_mkspc (level) << name << ":" << std::endl;
            for (const auto &child: _map [name]) {
                print (child, level+1);
            }
        }

        void _feed_by (const std::string& name, std::set <std::string> &ret) {
            if (_map.count (name) == 0)
                return;

            ret.insert (name);
            for (const auto& kid: _map [name])
                _feed_by (kid, ret);
        }

        // return a subtree - recursively
        std::set <std::string> feed_by (const std::string& name) {
            std::set <std::string> ret {};
            _feed_by (name, ret);
            return ret;
        }

        // return just kids
        std::set <std::string> at (const std::string& name) const {
            return _map.at (name);
        }

        bool has (const std::string& name) const {
            return _map.count (name) != 0;
        }

    private:
        std::map <std::string, std::set <std::string>> _map;
};

// helper print function to be deleted
static const std::string
s_get (const tntdb::Row& row, const std::string& key) {
    try {
        return row.getString (key);
    }
    catch (const tntdb::NullValue &n) {
        return "(null)";
    }
}

static int
s_geti (const tntdb::Row& row, const std::string& key) {
    try {
        return row.getInt (key);
    }
    catch (const tntdb::NullValue &n) {
        return -1;
    }
}

static std::string
s_mkspc (int level) {
    std::string ret;
    if (level <=0)
        return ret;
    for (int i =0; i != level; i++) {
        ret.append ("  ");
    }
    return ret;
}


//  return a set of devices feeded by feed_by
//
//  feed_by - return devices feed by given iname
//
//  return tntdb::Result
//
std::set <std::string>
topology2_feed_by (
    tntdb::Connection& conn,
    const std::string& feed_by)
{
    std::string query = "SELECT src_name, dest_name FROM v_bios_asset_link_topology WHERE id_asset_link_type = 1";
    tntdb::Statement st = conn.prepareCached (query);

    NodeMap nm{};

    for (const auto& row: st.select ()) {

        std::string name = s_get (row, "src_name");
        std::string kid = s_get (row, "dest_name");

        nm.add (name, kid);
    }

    return nm.feed_by (feed_by);
}

//  return a topology
//
//  from    - iname of asset where topology starts
//  filter  - (datacenter,row,rack,room,device) - show only selected types
//  recursive - true|false
//  feed_by - additional filtering - only devices feed by given iname
//
//  return tntdb::Result
//

tntdb::Result
topology2_from (
    tntdb::Connection& conn,
    const std::string& from
    )
{

    // TODO: db error handling
    std::string query = \
        " SELECT "
        "    t1.id AS DBID1, "
        "    t2.id AS DBID2, "
        "    t3.id AS DBID3, "
        "    t4.id AS DBID4, "
        "    t5.id AS DBID5, "
        "    t6.id AS DBID6, "
        "    t1.name AS ID1, "
        "    t2.name AS ID2, "
        "    t3.name AS ID3, "
        "    t4.name AS ID4, "
        "    t5.name AS ID5, "
        "    t6.name AS ID6, "
        "    t1.id_type AS TYPEID1, "
        "    t2.id_type AS TYPEID2, "
        "    t3.id_type AS TYPEID3, "
        "    t4.id_type AS TYPEID4, "
        "    t5.id_type AS TYPEID5, "
        "    t6.id_type AS TYPEID6, "
        "    t1.id_subtype AS SUBTYPEID1, "
        "    t2.id_subtype AS SUBTYPEID2, "
        "    t3.id_subtype AS SUBTYPEID3, "
        "    t4.id_subtype AS SUBTYPEID4, "
        "    t5.id_subtype AS SUBTYPEID5, "
        "    t6.id_subtype AS SUBTYPEID6, "
        "    t1ext.value AS ORDER1, "
        "    t2ext.value AS ORDER2, "
        "    t3ext.value AS ORDER3, "
        "    t4ext.value AS ORDER4, "
        "    t5ext.value AS ORDER5, "
        "    t6ext.value AS ORDER6, "
        "    t1name.value AS NAME1, "
        "    t2name.value AS NAME2, "
        "    t3name.value AS NAME3, "
        "    t4name.value AS NAME4, "
        "    t5name.value AS NAME5, "
        "    t6name.value AS NAME6  "
        "  FROM v_bios_asset_element AS t1 "
        "    LEFT JOIN v_bios_asset_element AS t2 ON t2.id_parent = t1.id "
        "    LEFT JOIN v_bios_asset_element AS t3 ON t3.id_parent = t2.id "
        "    LEFT JOIN v_bios_asset_element AS t4 ON t4.id_parent = t3.id "
        "    LEFT JOIN v_bios_asset_element AS t5 ON t5.id_parent = t4.id "
        "    LEFT JOIN v_bios_asset_element AS t6 ON t6.id_parent = t5.id "
        "    INNER JOIN t_bios_asset_device_type v6 "
        "    ON (v6.id_asset_device_type = t1.id_subtype) "
        "    LEFT JOIN t_bios_asset_ext_attributes AS t1ext ON (t1.id = t1ext.id_asset_element AND t1ext.keytag=\"order\") "
        "    LEFT JOIN t_bios_asset_ext_attributes AS t2ext ON (t2.id = t2ext.id_asset_element AND t2ext.keytag=\"order\") "
        "    LEFT JOIN t_bios_asset_ext_attributes AS t3ext ON (t3.id = t3ext.id_asset_element AND t3ext.keytag=\"order\") "
        "    LEFT JOIN t_bios_asset_ext_attributes AS t4ext ON (t4.id = t4ext.id_asset_element AND t4ext.keytag=\"order\") "
        "    LEFT JOIN t_bios_asset_ext_attributes AS t5ext ON (t5.id = t5ext.id_asset_element AND t5ext.keytag=\"order\") "
        "    LEFT JOIN t_bios_asset_ext_attributes AS t6ext ON (t6.id = t6ext.id_asset_element AND t6ext.keytag=\"order\") "
        "    LEFT JOIN t_bios_asset_ext_attributes AS t1name ON (t1.id = t1name.id_asset_element AND t1name.keytag=\"name\") "
        "    LEFT JOIN t_bios_asset_ext_attributes AS t2name ON (t2.id = t2name.id_asset_element AND t2name.keytag=\"name\") "
        "    LEFT JOIN t_bios_asset_ext_attributes AS t3name ON (t3.id = t3name.id_asset_element AND t3name.keytag=\"name\") "
        "    LEFT JOIN t_bios_asset_ext_attributes AS t4name ON (t4.id = t4name.id_asset_element AND t4name.keytag=\"name\") "
        "    LEFT JOIN t_bios_asset_ext_attributes AS t5name ON (t5.id = t5name.id_asset_element AND t5name.keytag=\"name\") "
        "    LEFT JOIN t_bios_asset_ext_attributes AS t6name ON (t6.id = t6name.id_asset_element AND t6name.keytag=\"name\") "
        "  WHERE t1.name=:from "
        "  ORDER BY "
        "   ORDER1 ASC, ORDER2 ASC, ORDER3 ASC, ORDER4 ASC, ORDER5 ASC, ORDER6 ASC ";

    tntdb::Statement st = conn.prepareCached (query);

    st.set ("from", from);
    return st.select ();
}


// gcc -lstdc++ -std=c++11 -lcxxtools -lczmq -ltntdb -lmlm json.cc -o json && ./json

struct Item
{

struct Topology
{
    std::vector <Item> rooms;
    std::vector <Item> rows;
    std::vector <Item> racks;
    std::vector <Item> devices;

    Topology () {}

    size_t empty () const {
        return \
        rooms.empty () && \
        rows.empty () && \
        racks.empty () && \
        devices.empty ();
    }
};

    Item () {}

    Item (
        const std::string &id,
        const std::string &name,
        const std::string &subtype,
        const std::string &type) :
        id {id},
        name {name},
        subtype {subtype},
        type {type},
        contains {}
        {}

    std::string id;
    std::string name;
    std::string subtype;
    std::string type;
    Topology contains;
    friend void operator<<= (cxxtools::SerializationInfo &si, const Item &asset);
};

/*
struct Topology
{
    std::vector <Item> datacenters;
    std::vector <Item> rooms;
    std::vector <Item> rows;
    std::vector <Item> racks;
    std::vector <Item> devices;

    size_t empty () const {
        return \
        datacenters.empty () && \
        rooms.empty () && \
        rows.empty () && \
        racks.empty () && \
        devices.empty ();
    }
};
*/
void operator<<= (cxxtools::SerializationInfo &si, const Item::Topology &topo)
{
    if (!topo.rooms.empty ())
        si.addMember("rooms") <<= topo.rooms;
    if (!topo.rows.empty ())
        si.addMember("rows") <<= topo.rows;
    if (!topo.racks.empty ())
        si.addMember("racks") <<= topo.racks;
    if (!topo.devices.empty ())
        si.addMember("devices") <<= topo.devices;
}


void operator<<= (cxxtools::SerializationInfo &si, const Item &asset)
{
    si.addMember("name") <<= asset.name;
    si.addMember("id") <<= asset.id;
    si.addMember("type") <<= asset.type;
    si.addMember("sub_type") <<= asset.subtype;
    if (!asset.contains.empty ())
       si.addMember("contains") <<= asset.contains;
}

static int
s_filter_type (const std::string &_filter) {
    if (!_filter.empty ()) {
        std::string filter = _filter;
        filter.pop_back ();
        return persist::type_to_typeid (filter);
    }
    return -1;
}

static bool
s_should_filter (int filter_type, int type) {
    return filter_type != -1 && type != filter_type;
}

void
topology2_from_json (
    std::ostream &out,
    tntdb::Result &res,
    const std::string &from,
    const std::string &filter,
    const std::set <std::string> &feeded_by)
{
    cxxtools::JsonSerializer serializer (out);
    serializer.beautify (true);

    Item::Topology topo {};

    std::set <std::string> processed {};

    int filter_type = s_filter_type (filter);

    for (const auto& row: res) {

        for (int i = 1; i != 7; i++) {

            std::string idx = std::to_string (i);
            std::string ID {"ID"}; ID.append (idx);
            std::string TYPE {"TYPEID"}; TYPE.append (idx);
            std::string SUBTYPE {"SUBTYPEID"}; SUBTYPE.append (idx);
            std::string NAME {"NAME"}; NAME.append (idx);

            // feed_by filtering
            std::string id = s_get (row, ID);
            if (!feeded_by.empty () && feeded_by.count (id) == 0)
                continue;

            // filter - type filtering
            int type = s_geti (row, TYPE);
            if (s_should_filter (filter_type, type))
                continue;

            if (processed.count (id) != 0 || id == "(null)")
                continue;

            Item it {
                id,
                s_get (row, NAME),
                persist::subtypeid_to_subtype (s_geti (row, SUBTYPE)),
                persist::typeid_to_type (s_geti (row, TYPE))};

            switch (type) {
                case persist::asset_type::ROOM:
                    topo.rooms.push_back (it);
                    break;
                case persist::asset_type::ROW:
                    topo.rows.push_back (it);
                    break;
                case persist::asset_type::RACK:
                    topo.racks.push_back (it);
                    break;
                default:
                    topo.devices.push_back (it);
            }
            processed.emplace (id);
        }
    }
    serializer.serialize(topo).finish();
}

static void
s_topo_recursive (
    Item::Topology &topo,
    std::set <std::string> from,
    const NodeMap &nm,
    std::map <std::string, Item> &im,
    int depth)
{
    if (from.empty ())
        return;
    printf("1 .hloubka, %d\n", depth);
    for (const std::string &id : from) {
        Item it;
        try {
            it = im.at (id);
        } catch (std::exception &e) {
            continue;
        }
        std::set <std::string> kids;
        int type = persist::type_to_typeid (it.type);

        // collect all kids
        if (nm.has (id) && ! nm.at (id).empty()) {
            for (const auto &kid: nm.at (id)) {
                kids.insert (kid);
            }
        }

        // add kids recursivelly to topology
        if (! kids.empty ()) {
            it.contains = Item::Topology {};
            s_topo_recursive (it.contains, kids, nm, im, depth + 1);
        }

        printf("2. hloubka, %d\n", depth);
        printf("id , %s\n", id.c_str());

        //if ( depth > 1) {
            switch (type) {
            case persist::asset_type::ROOM:
                topo.rooms.push_back (it);
                break;
            case persist::asset_type::ROW:
                topo.rows.push_back (it);
                break;
            case persist::asset_type::RACK:
                topo.racks.push_back (it);
                break;
            default:
                topo.devices.push_back (it);
            }
            //}
        printf("3. hloubka, %d\n", depth);

    }
}

void
topology2_from_json_recursive (
    std::ostream &out,
    tntdb::Result &res,
    const std::string &from,
    const std::string &filter,
    const std::set <std::string> &feeded_by)
{

    NodeMap nm {};
    // build the topology using NodeMap put data to map string->Item
    for (const auto& row: res) {
        for (int i = 1; i != 6; i++) {
            std::string name = s_get (row, "ID" + std::to_string (i));
            std::string kid = s_get (row, "ID" + std::to_string (i+1));

            if (name != "(null)" && kid != "(null)") {
                nm.add (name, kid);
            }
        }
    }
    
    int filter_type = s_filter_type (filter);

    // create a map id -> Item
    std::set <std::string> processed {};
    std::map <std::string, Item> im {};
    std::string from_type;
    std::string from_subtype;
    Item it2;
    for (const auto& row: res) {

        for (int i = 1; i != 7; i++) {

            std::string idx = std::to_string (i);
            std::string ID {"ID"}; ID.append (idx);
            // TODO:!!!! NAME!!!! - need more joins?
            std::string TYPE {"TYPEID"}; TYPE.append (idx);
            std::string SUBTYPE {"SUBTYPEID"}; SUBTYPE.append (idx);

            // feed_by filtering
            std::string id = s_get (row, ID);
            if (!feeded_by.empty () && feeded_by.count (id) == 0)
                continue;

            // filter - type filtering
            int type = s_geti (row, TYPE);
            if (s_should_filter (filter_type, type))
                continue;

            if (processed.count (id) != 0 || id == "(null)")
                continue;

            if (!id.compare (from))
            {

                from_type = persist::typeid_to_type (s_geti (row, TYPE));
                from_subtype = persist::subtypeid_to_subtype (s_geti (row, SUBTYPE));

                it2.id = from;
                it2.name = "(name)";
                it2.subtype =  from_subtype;
                it2.type =  from_type;

            }
            Item it {
                id,
                    "(name)",
                    persist::subtypeid_to_subtype (s_geti (row, SUBTYPE)),
                    persist::typeid_to_type (s_geti (row, TYPE))};

            im.insert (std::make_pair (id, it));
            processed.emplace (id);
        }
    }

    int depth = 1;
    cxxtools::JsonSerializer serializer (out);
    serializer.beautify (true);

    Item::Topology topo {};
    s_topo_recursive (
        topo,
        std::set <std::string> {from},
        nm,
        im,
        depth);

    it2.contains = topo;
    serializer.serialize(it2).finish();
}

}// namespace persist

/*
int main () {

    tntdb::Connection conn = tntdb::connectCached (url);

    // 1. params
    std::string from {"ROOM1"};
    std::string feed_by {"ups-1"};

    // 2. queries
    auto res = s_topologyv2 (conn, "ROOM1");
    std::set <std::string> feeded_by {};
    s_json (std::cout, res, "", feeded_by);

    std::cout << "===============================================================" << std::endl;
    s_json_recursive (
        std::cout,
        res,
        from,
        "",
        feeded_by);

    Item asset;
    Item::Topology topo;

    std::string name  = "datacente-1";
    std::string subtype = "NA";
    std::string id = "2";
    std::string type = "datacenter";
    std::string order = "1";

    asset.name  = name;
    asset.type  = type;
    //asset.order  = order;
    asset.subtype = subtype;
    asset.id = id;

    //topo.datacenters.push_back (asset);

    printf("\n");
    return 0;

    // 3. feed_by
    //std::set <std::string> feeded_by;
    if (! feed_by.empty())
        feeded_by = s_feed_by (conn, feed_by);

    // 4. output TO BE DONE

}
*/
