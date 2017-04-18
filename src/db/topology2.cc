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
 *  PoC of a new location_from function, will print DB content to stdout
 *
 *  TODO:
 *  1. support for unlocated elements
 *  2. more values to be returned (type, subtype, dbid, asset name)
 *  3. recursive = true|false
 *  5. API between function and REST API
 *  6. JSON output
 *
 *  g++ -std=c++11 src/db/topology2.cc -lcxxtools -ltntdb && ./a.out ;
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
        "    t6ext.value AS ORDER6  "
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
        "  WHERE t1.name=:from "
        "  ORDER BY "
        "   ORDER1 ASC, ORDER2 ASC, ORDER3 ASC, ORDER4 ASC, ORDER5 ASC, ORDER6 ASC ";

    tntdb::Statement st = conn.prepareCached (query);

    st.set ("from", from);
    return st.select ();
}

//so far nonrecursive way

// gcc -lstdc++ -std=c++11 -lcxxtools -lczmq -ltntdb -lmlm json.cc -o json && ./json

struct Item
{

struct Topology
{
    std::vector <Item> datacenters;
    std::vector <Item> rooms;
    std::vector <Item> rows;
    std::vector <Item> racks;
    std::vector <Item> devices;

    Topology () {}

    size_t empty () const {
        return \
        datacenters.empty () && \
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
    //std::string order;
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
    if (!topo.datacenters.empty ())
        si.addMember("datacenters") <<= topo.datacenters;
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
    //si.addMember("order") <<= asset.order;
    if (!asset.contains.empty ())
       si.addMember("contains") <<= asset.contains;
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

    for (const auto& row: res) {

        for (int i = 1; i != 7; i++) {


            std::string idx = std::to_string (i);
            std::string ID {"ID"}; ID.append (idx);
            // TODO:!!!! NAME!!!! - need more joins?
            std::string TYPE {"TYPEID"}; TYPE.append (idx);
            std::string SUBTYPE {"SUBTYPEID"}; SUBTYPE.append (idx);

            // TODO: filter!!

            int type = s_geti (row, TYPE);
            std::string id = s_get (row, ID);
            if (processed.count (id) != 0 || id == "(null)")
                continue;

            Item it {
                id,
                "(name)",
                persist::subtypeid_to_subtype (s_geti (row, SUBTYPE)),
                persist::typeid_to_type (s_geti (row, TYPE))};

            switch (type) {
                case persist::asset_type::DATACENTER:
                    topo.datacenters.push_back (it);
                    break;
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
    std::map <std::string, Item> &im)
{
    if (from.empty ())
        return;

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
            s_topo_recursive (it.contains, kids, nm, im);
        }

        switch (type) {
            case persist::asset_type::DATACENTER:
                topo.datacenters.push_back (it);
                break;
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
    }
}

void
topology2_from_json_recursive (
    std::ostream &out,
    tntdb::Result &res,
    const std::string &from,
    const std::string &_filter,
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

    int ifilter = -1;
    if (! _filter.empty ()) {
        std::string filter = _filter;
        filter.pop_back ();
        ifilter = persist::type_to_typeid (filter);
    }

    // create a map id -> Item
    std::set <std::string> processed {};
    std::map <std::string, Item> im {};
    for (const auto& row: res) {

        for (int i = 1; i != 7; i++) {

            std::string idx = std::to_string (i);
            std::string ID {"ID"}; ID.append (idx);
            // TODO:!!!! NAME!!!! - need more joins?
            std::string TYPE {"TYPEID"}; TYPE.append (idx);
            std::string SUBTYPE {"SUBTYPEID"}; SUBTYPE.append (idx);

            // TODO: filter!!

            int type = s_geti (row, TYPE);
            if (ifilter != -1 && ifilter != type)
                continue;
            std::string id = s_get (row, ID);
            if (processed.count (id) != 0 || id == "(null)")
                continue;

            Item it {
                id,
                    "(name)",
                    persist::subtypeid_to_subtype (s_geti (row, SUBTYPE)),
                    persist::typeid_to_type (s_geti (row, TYPE))};

            im.insert (std::make_pair (id, it));
            processed.emplace (id);
        }
    }

    cxxtools::JsonSerializer serializer (out);
    serializer.beautify (true);

    Item::Topology topo {};
    s_topo_recursive (
        topo,
        std::set <std::string> {from},
        nm,
        im);

    serializer.serialize(topo).finish();
}

}// namespace persist

/*
int main () {

    tntdb::Connection conn = tntdb::connectCached (url);

    // 1. params
    std::string from {"DC1"};
    std::string feed_by {"ups-1"};

    // 2. queries
    auto res = s_topologyv2 (conn, "DC1");
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

    topo.datacenters.push_back (asset);

    printf("\n");
    return 0;

    // 3. feed_by
    //std::set <std::string> feeded_by;
    if (! feed_by.empty())
        feeded_by = s_feed_by (conn, feed_by);

    // 4. output TO BE DONE

}
*/
