/*
 *
 * Copyright (C) 2017 Eaton
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

/*!
 * \file  topology2.h
 * \author Michal Vyskocil <MichalVyskocil@Eaton.com>
 * \author Barbora Stepankova <BarboraStepankova@Eaton.com>
 * \brief Next generation topology calls
 */
#ifndef SRC_INCLUDE_TOPOLOGY2
#define SRC_INCLUDE_TOPOLOGY2

namespace persist {

struct Item
{

struct Topology
{
    typedef bool (compare_fn) (const Item&, const Item&);

    std::vector <Item> rooms;
    std::vector <Item> rows;
    std::vector <Item> racks;
    std::vector <Item> devices;
    std::vector <Item> groups;

    Topology () {}

    size_t empty () const {
        return \
        rooms.empty () && \
        rows.empty () && \
        racks.empty () && \
        devices.empty () && \
        groups.empty ();
    }

    void push_back (const Item &it) {
        int type = persist::type_to_typeid (it.type);
        switch (type) {
            case persist::asset_type::ROOM:
                rooms.push_back (it);
                break;
            case persist::asset_type::ROW:
                rows.push_back (it);
                break;
            case persist::asset_type::RACK:
                racks.push_back (it);
                break;
            case persist::asset_type::DEVICE:
                devices.push_back (it);
                break;
            case persist::asset_type::GROUP:
                groups.push_back (it);
        }
    }

    void sort (compare_fn compare) {
        std::sort (rooms.begin (), rooms.end (), compare);
        std::sort (rows.begin (), rows.end (), compare);
        std::sort (racks.begin (), racks.end (), compare);
        std::sort (devices.begin (), devices.end (), compare);
        std::sort (groups.begin (), groups.end (), compare);
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
        contains {},
        asset_order {}
        {}

    std::string id;
    std::string name;
    std::string subtype;
    std::string type;
    Topology contains;
    std::string asset_order;
    friend void operator<<= (cxxtools::SerializationInfo &si, const Item &asset);
};
//
//  maps node to it's kids, ideal structure for feed_by queries
//
//  eg for topology v_bios_asset_link_topology
//
//  feed, ups
//  ups, epdu1
//  epdu1, srv1.2
//  epdu1, srv2.2
//  epdu2, srv2.2
//  epdu2, srv2.1
//
//  will construct
//
//  feed -> ups -> epdu1 -> srv1.2
//                       -> srv2.2
//              -> epdu2 -> srv2.1
//                       -> srv2.2
//
//  and will return a subtree of a power chain
//
//  Example:
//  feed_by ("epdu2") -> {"epdu2", "srv2.1", "srv2.2"};
//

//  return all groups for given id
//
//  if recursive, return all devices in this group
//
std::vector <Item>
topology2_groups (
    tntdb::Connection& conn,
    const std::string& id,
    bool recursive
    );

//  return a set of devices feeded by feed_by
//
//  feed_by - return devices feed by given iname
//
//  return tntdb::Result
//
std::set <std::string>
topology2_feed_by (
    tntdb::Connection& conn,
    const std::string& feed_by);

//  return a topology frovm
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
    const std::string& from);

//  serialize topology returned by topology2_from to ostream
//
//  out - output stream
//  ret - tntdb::Result from topology2_from
//  filter - show only given devices
//  feeded_by - if not empty - show only devices from this set
//  groups - list of groups device belongs to
//

void
topology2_from_json (
    std::ostream &out,
    tntdb::Result &res,
    const std::string &from,
    const std::string &filter,
    const std::set <std::string> &feeded_by,
    const std::vector <Item> &groups
    );

//  serialize topology returned by topology2_from to ostream
//  recursive variant
//
//  out - output stream
//  ret - tntdb::Result from topology2_from
//  filter - show only given devices
//  feeded_by - if not empty - show only devices from this set
//  groups - list of groups device belongs to
//

void
topology2_from_json_recursive (
    std::ostream &out,
    tntdb::Connection &conn,
    tntdb::Result &res,
    const std::string &from,
    const std::string &_filter,
    const std::set <std::string> &feeded_by,
    const std::vector <Item> &groups
    );

// returns TRUE if asset_name is power device
// returns FALSE if otherwise
bool
    is_power_device (tntdb::Connection &conn, std::string &asset_name);

};  // namespace persist

#endif //SRC_INCLUDE_TOPOLOGY2
