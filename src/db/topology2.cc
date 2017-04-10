#include <czmq.h>
#include <tntdb.h>
#include <algorithm>
#include <exception>

#include "../../src/include/asset_types.h"

/**
 *  topologyv2.cc
 *
 *  PoC of a new location_from function, will print DB content to stdout
 *
 *  TODO:
 *  2. more values to be returned (type, subtype, dbid, asset name)
 *  3. recursive = true|false
 *  4. feed_by
 *  5. API between function and REST API
 *
 *  g++ -std=c++11 src/db/topology2.cc -lcxxtools -ltntdb && ./a.out ;
 *
 */

std::string url = std::string("mysql:db=box_utf8;user=") +
                  ((getenv("DB_USER")   == NULL) ? "root" : getenv("DB_USER")) +
                  ((getenv("DB_PASSWD") == NULL) ? ""     :
                      std::string(";password=") + getenv("DB_PASSWD"));

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

//
//  from    - iname of asset where topology starts
//  filter  - (datacenter,row,rack,room,device) - show only selected types
//  recursive - true|false
//  feed_by - additional filtering - only devices feed by given iname
//
//  return tntdb::Result
//

static tntdb::Result
s_topologyv2 (
    tntdb::Connection& conn,
    const std::string& from,
    bool recursive,
    const std::string& feed_by)
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

static void
s_print_topology2 (
    std::ostream& out,
    tntdb::Result& res,
    const std::string& _filter
    ) {

    int id_type = -1;
    if (!_filter.empty()) {
        std::string filter = _filter;
        std::transform (filter.begin(), filter.end(), filter.begin(), ::tolower);
        if (filter == "rooms") {
            id_type = persist::asset_type::ROOM;
        } else if (filter == "rows") {
            id_type = persist::asset_type::ROW;
        } else if (filter == "racks") {
            id_type = persist::asset_type::RACK;
        } else if (filter == "devices") {
            id_type = persist::asset_type::DEVICE;
        } else if (filter == "groups") {
            id_type = persist::asset_type::GROUP;
        } else {
            throw std::invalid_argument ("unknown filter value");
        }
    }

    std::cerr << "D: id_type=" << id_type << std::endl;

    for (const auto& row: res) {
        for (int i = 1; i != 7; i++) {

            std::string TYPEID = "TYPEID";
            TYPEID.append (std::to_string (i));

            if (id_type != -1 && s_geti (row, TYPEID) != id_type)
                continue;

            std::string ID = "ID";
            ID.append (std::to_string (i));

            std::string ORDER = "ORDER";
            ORDER.append (std::to_string (i));

            out << ID << ": " << s_get (row, ID) << ", " << TYPEID << ": " << s_geti (row, TYPEID) << ", " << ORDER << ": " << s_get (row, ORDER) << ", ";
        }

        out << std::endl;
    }
}

static void
print_non_recursively (std::ostream& out,
                       tntdb::Result& res,
                       std::string from)
{
    int start;
    int from_type;
    std::string tmp;
    for (int i = 1; i != 6; i++)
    {
        for (const auto& row : res)
        {
            std::string ID = "ID";
            std::string TYPEID = "TYPEID";

            ID.append(std::to_string(i));
            TYPEID.append(std::to_string(i));

            if (s_get (row, ID) != from)
                continue;
            else
            {
                start = i;
                from_type = s_geti (row, TYPEID);

                out << ID << ": " << s_get (row, ID) << " " << TYPEID << " : " << s_geti (row, TYPEID) <<  "\n";
                break;
            }
        }
    }

    for (int t = from_type + 1; t != 7; ++t)
    {
        for (int ii = start; ii != 7; ii++)
        {
            for (const auto& row : res)
            {
                std::string ID = "ID";
                std::string TYPEID = "TYPEID";
                std::string ORDER = "ORDER";
                ID.append (std::to_string (ii));
                TYPEID.append (std::to_string (ii));
                ORDER.append (std::to_string (ii));

                if (s_geti (row, TYPEID) == t && s_get (row, ID) != tmp)
                {
                    out << ID << " : " << s_get (row, ID) << " " << TYPEID << " : " << s_geti (row, TYPEID) << ", " << ORDER << ": " << s_get (row, ORDER) << "\n";
                    tmp = s_get (row, ID);
                }
            }
        }
    }


}


int main () {

    tntdb::Connection conn = tntdb::connectCached (url);

    auto res = s_topologyv2 (conn, "room-2", true, "");
    s_print_topology2 (std::cout, res, "");
    std::cout << "======================================================" << std::endl;
    res = s_topologyv2 (conn, "room-2", true, "");
    s_print_topology2 (std::cout, res, "devices");
    std::cout << "======================================================" << std::endl;
    print_non_recursively (std::cout, res, "room-2");

}
