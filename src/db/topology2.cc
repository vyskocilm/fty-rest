#include <czmq.h>
#include <tntdb.h>

/**
 *  topologyv2.cc
 *
 *  PoC of a new location_from function, will print DB content to stdout
 *
 *  TODO:
 *  1. ordering by ext attribute
 *  2. more values to be returned (type, subtype, dbid, asset name)
 *  2. filter
 *  3. recursive = true|false
 *  4. feed_by
 *  5. API between function and REST API
 *
 *  g++ -std=c++11 topology2.cc -lcxxtools -ltntdb && ./a.out ;
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
    const std::string& filter,
    bool recursive,
    const std::string& feed_by)
{

    // TODO: db error handling
    tntdb::Statement st = conn.prepareCached (
        " SELECT "
        "    t1.name AS LEV1, "
        "    t2.name AS LEV2, "
        "    t3.name AS LEV3, "
        "    t4.name AS LEV4, "
        "    t5.name AS LEV5, "
        "    t6.name AS LEV6  "
        "  FROM v_bios_asset_element AS t1 "
        "    LEFT JOIN v_bios_asset_element AS t2 ON t2.id_parent = t1.id "
        "    LEFT JOIN v_bios_asset_element AS t3 ON t3.id_parent = t2.id "
        "    LEFT JOIN v_bios_asset_element AS t4 ON t4.id_parent = t3.id "
        "    LEFT JOIN v_bios_asset_element AS t5 ON t5.id_parent = t4.id "
        "    LEFT JOIN v_bios_asset_element AS t6 ON t6.id_parent = t5.id "
        "    INNER JOIN t_bios_asset_device_type v6 "
        "    ON (v6.id_asset_device_type = t1.id_subtype) "
        "  WHERE t1.name=:from ");

    return st.set ("from", from).select ();
}

int main () {

    tntdb::Connection conn = tntdb::connectCached (url);
    auto res = s_topologyv2 (conn, "DC1", "", true, "");
    for (const auto& row: res) {
        std::cout \
            << "LEV1: " << s_get (row, "LEV1") << \
            ", LEV2: " << s_get (row, "LEV2") << \
            ", LEV3: " << s_get (row, "LEV3") << \
            ", LEV4: " << s_get (row, "LEV4") << \
            ", LEV5: " << s_get (row, "LEV5") << \
            ", LEV6: " << s_get (row, "LEV6") << \
        std::endl;
    }

}
