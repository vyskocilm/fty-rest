#include <malamute.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <cxxtools/split.h>
#include <tntdb/error.h>
#include <tntdb.h>
#include <sstream>
#include <cxxtools/serializationinfo.h>
#include <cxxtools/jsondeserializer.h>
#include <cxxtools/jsonserializer.h>

//so far nonrecursive way
struct Item
{
    std::string id;
    std::string name;
    std::string subtype;
    std::string type;
    std::string order;
    std::vector <Item> contains;
};

struct Topology
{
    std::vector <Item> datacenters;
    std::vector <Item> rooms;
    std::vector <Item> rows;
    std::vector <Item> racks;
    std::vector <Item> devices;
};

inline void operator<<= (cxxtools::SerializationInfo &si, const Item &asset)
{
    si.addMember("name") <<= asset.name;
    si.addMember("id") <<= asset.id;
    si.addMember("sub_type") <<= asset.subtype;
    si.addMember("type") <<= asset.type;
    si.addMember("order") <<= asset.order;
    if (!asset.contains.empty ())
       si.addMember("contains") <<= asset.contains;
}

inline void operator<<= (cxxtools::SerializationInfo &si, const Topology &topo)
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


int
main ()
{/*
    std::string url = std::string("mysql:db=box_utf8;user=") +
                  ((getenv("DB_USER")   == NULL) ? "root" : getenv("DB_USER")) +
                  ((getenv("DB_PASSWD") == NULL) ? ""     :
                      std::string(";password=") + getenv("DB_PASSWD"));
 */
    //    cxxtools::SerializationInfo si;
    Item asset;
    Topology topo;
    std::vector <Item> buff;

    std::string name  = "datacente-1";
    std::string subtype = "NA";
    std::string id = "2";
    std::string type = "datacenter";
    std::string order = "1";

    asset.name  = name;
    asset.type  = type;
    asset.order  = order;
    asset.subtype = subtype;
    asset.id = id;
    asset.contains.push_back (asset);

    topo.datacenters.push_back (asset);

    cxxtools::JsonSerializer serializer (std::cout);
    serializer.beautify (true);
    serializer.serialize(asset).finish();
    printf("\n");
    return 0;
}
