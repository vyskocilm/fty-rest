#include <zmq.h>
#include <czmq.h>
#include <tnt/http.h>
#include <algorithm>
#include <string>
#include <map>
#include <limits.h>

#include "log.h"
#include "asset_types.h"
#include "common_msg.h"
#include "dbpath.h"
#include "monitor.h"
#include "upsstatus.h"
#include "persistencelogic.h"
#include "defs.h"

#include "cleanup.h"
#include "defs.h"
#include "data.h"
#include "asset_types.h"
#include "common_msg.h"
#include "dbpath.h"
#include "monitor.h"
#include "upsstatus.h"
#include "utils.h"

typedef std::string (*MapValuesTransformation)(std::string);

byte asset_manager::type_to_byte(std::string type) {
    std::transform(type.begin(), type.end(), type.begin(), ::tolower);
    byte ret = asset_type::UNKNOWN;
    if(type == "datacenter") {
        ret = asset_type::DATACENTER;
    } else if(type == "room") {
        ret = asset_type::ROOM;
    } else if(type == "row") {
        ret = asset_type::ROW;
    } else if(type == "rack") {
        ret = asset_type::RACK;
    } else if(type == "group") {
        ret = asset_type::GROUP;
    } else if(type == "device") {
        ret = asset_type::DEVICE;
    }
    return ret;
}

zmsg_t *asset_manager::get_items(std::string type) {
    log_debug("Trying to get elements of type %s", type.c_str());
    byte real_type = asset_manager::type_to_byte(type);
    if ( real_type == (byte)asset_type::UNKNOWN ) {
        return NULL;
    }

    _scoped_zmsg_t *get_elements = asset_msg_encode_get_elements(real_type);
    zmsg_t *ret = persist::process_message(&get_elements);
    zmsg_destroy(&get_elements);

    return ret;
}

std::string asset_manager::byte_to_type(byte type) {
    switch(type) {
        case asset_type::DATACENTER:
            return "datacenter";
        case asset_type::ROOM:
            return "room";
        case asset_type::ROW:
            return "row";
        case asset_type::RACK:
            return "rack";
        case asset_type::GROUP:
            return "group";
        case asset_type::DEVICE:
            return "device";
        default:
            return "unknown";
    }
}

static std::string s_scale(const std::string& val, int8_t scale) {
// TODO: Refactor away multiple calls to val.size(),
// they seem redundant (val does not change here?)
    assert(val != "");
// The string.size() is a size_t (unsigned int or larger), and
// our scale is a signed byte; make sure they fit each other
    assert(val.size() <= SCHAR_MAX);

    std::string ret{val};

    //1. no scale, nothing to do
    if (!scale)
        return ret;

    //2. positive scale, simply append things
    if (scale > 0) {
        while (scale > 0) {
            ret.append("0");
            scale--;
        }
        return ret;
    }

    //3. scale is "bigger" than size of string,
    if (-scale >= (int8_t)val.size()) {
        //3a. prepend zeroes
        while (-scale != (int8_t)val.size()) {
            ret.insert(0, "0");
            scale++;
        }

        //3b and prepend 0.
        ret.insert(0, "0.");
        return ret;
    }

    //4. just find the right place for dot
    ret.insert(val.size() + scale, ".");
    return ret;

}

std::string measures_manager::apply_scale(const std::string &val, const std::string &scale)
{
    int scale_num = std::stoi (scale, nullptr, 10);
    return s_scale(val, scale_num);
}

std::string measures_manager::map_values(std::string name, std::string value) {
    static std::map<std::string, MapValuesTransformation > map = {
        { "status.ups", &shared::upsstatus_to_string }
    };
    
    auto it = map.find(name);
    if(it != map.end()) {
        return "\"" + it->second(value) + "\"";
    }
    return value;
}

std::string ui_props_manager::get(std::string& result) {

    //FIXME: where to put the constant?
    _scoped_common_msg_t *reply = select_ui_properties(url.c_str());
    if (!reply)
        return std::string("{\"error\" : \"Can't load ui/properties from database!\"}");

    uint32_t msg_id = common_msg_id(reply);

    if (msg_id == COMMON_MSG_FAIL) {
        auto ret = std::string{"{\"error\" : \""};
        ret.append(common_msg_errmsg(reply));
        ret.append("\"}");
        common_msg_destroy(&reply);
        return ret;
    }
    else if (msg_id != COMMON_MSG_RETURN_CINFO) {
        common_msg_destroy(&reply);
        return std::string("{\"error\" : \"Unexpected msg_id delivered, expected COMMON_MSG_RETURN_CINFO\"}");
    }

    _scoped_zmsg_t *zmsg = common_msg_get_msg(reply);
    if (!zmsg) {
        common_msg_destroy(&reply);
        return std::string("{\"error\" : \"Can't extract inner message from reply!\"}");
    }

    _scoped_common_msg_t *msg = common_msg_decode(&zmsg);
    common_msg_destroy(&reply);
    if (!msg)
        return std::string("{\"error\" : \"Can't decode inner message from reply!\"}");
    
    _scoped_zchunk_t *info = common_msg_get_info(msg);
    common_msg_destroy(&msg);
    if (!info)
        return std::string("{\"error\" : \"Can't get chunk from reply!\"}");

    _scoped_char *s = zchunk_strdup(info);
    zchunk_destroy(&info);
    if (!s)
        return std::string("{\"error\" : \"Can't get string from reply!\"}");
    
    result = s;
    FREE0 (s)

    return std::string{};
}

std::string ui_props_manager::put(const std::string& ext) {

    const char* s = ext.c_str();

    _scoped_zchunk_t *chunk = zchunk_new(s, strlen(s));
    if (!chunk)
        return std::string("fail to create zchunk");

    //FIXME: where to store client_id?
    _scoped_common_msg_t *reply = update_ui_properties(url.c_str(), &chunk);
    uint32_t msg_id = common_msg_id(reply);
    common_msg_destroy(&reply);

    if (msg_id != COMMON_MSG_DB_OK)
        return std::string("msg_id != COMMON_MSG_DB_OK");

    return "";
}

/**
 * \brief get the error string, msg string and HTTP code from common_msg
 */
void common_msg_to_rest_error(common_msg_t* cm_msg, std::string& error, std::string& msg, unsigned* code) {

    assert(cm_msg);

    if (common_msg_id(cm_msg) == COMMON_MSG_FAIL) {
        switch (common_msg_errorno(cm_msg)) {
            case DB_ERROR_NOTFOUND:
                error = "not_found";
                *code = HTTP_NOT_FOUND;
                break;
            case DB_ERROR_BADINPUT:
                error = "bad_input";
                *code = HTTP_BAD_REQUEST;
                break;
            case DB_ERROR_NOTIMPLEMENTED:
                error = "not_implemented";
                *code = HTTP_NOT_IMPLEMENTED;
                break;
            default:
                error = "internal_error";
                *code = HTTP_INTERNAL_SERVER_ERROR;
                break;
        }
        const char *cmsg = common_msg_errmsg(cm_msg);
        msg = (cmsg == NULL) ? "" : cmsg;
    }
    else {
        error = "internal_error";
        msg = "unexpected message";
        *code = HTTP_INTERNAL_SERVER_ERROR;
    }
}


db_reply <db_web_element_t>
    asset_manager::get_item1
        (const std::string &id)
{
    db_reply <db_web_element_t> ret;

    // TODO add better converter
    uint32_t real_id = atoi(id.c_str());
    if ( real_id == 0 )
    {
        ret.status        = 0;
        ret.errtype       = DB_ERR;
        ret.errsubtype    = DB_ERROR_INTERNAL;
        ret.msg           = "cannot convert an id";
        log_warning (ret.msg);
        return ret;
    }
    log_debug ("id converted successfully");

    try{
        tntdb::Connection conn = tntdb::connectCached(url);
        log_debug ("connection was successful");

        auto basic_ret = persist::select_asset_element_web_byId(conn, real_id);
        log_debug ("basic select is done");

        if ( basic_ret.status == 0 )
        {
            ret.status        = basic_ret.status;
            ret.errtype       = basic_ret.errtype; 
            ret.errsubtype    = basic_ret.errsubtype;
            ret.msg           = basic_ret.msg;
            log_warning (ret.msg);
            return ret;
        }
        log_debug (" and there were no errors");
        ret.item.basic = basic_ret.item;
        
        auto ext_ret = persist::select_ext_attributes(conn, real_id);
        log_debug ("ext select is done");

        if ( ext_ret.status == 0 )
        {
            ret.status        = ext_ret.status;
            ret.errtype       = ext_ret.errtype; 
            ret.errsubtype    = ext_ret.errsubtype;
            ret.msg           = ext_ret.msg;
            log_warning (ret.msg);
            return ret;
        }
        log_debug (" and there were no errors");
        ret.item.ext = ext_ret.item;

        auto group_ret = persist::select_asset_element_groups(conn, real_id);
        log_debug ("groups select is done");

        if ( group_ret.status == 0 )
        {
            ret.status        = group_ret.status;
            ret.errtype       = group_ret.errtype; 
            ret.errsubtype    = group_ret.errsubtype;
            ret.msg           = group_ret.msg;
            log_warning (ret.msg);
            return ret;
        }
        log_debug (" and there were no errors");
        ret.item.groups = group_ret.item;

        if ( ret.item.basic.type_id == asset_type::DEVICE )
        {
            auto powers = persist::select_asset_device_links_to (conn, real_id, INPUT_POWER_CHAIN);
            log_debug ("powers select is done");

            if ( powers.status == 0 )
            {
                ret.status        = powers.status;
                ret.errtype       = powers.errtype; 
                ret.errsubtype    = powers.errsubtype;
                ret.msg           = powers.msg;
                log_warning (ret.msg);
                return ret;
            }
            log_debug (" and there were no errors");
            ret.item.powers = powers.item;
        }

        ret.status = 1;
        return ret;
    }
    catch (const std::exception &e) {
        ret.status        = 0;
        ret.errtype       = DB_ERR;
        ret.errsubtype    = DB_ERROR_INTERNAL;
        ret.msg           = e.what();
        LOG_END_ABNORMAL(e);
        return ret;
    } 
}

db_reply <std::map <uint32_t, std::string> >
   asset_manager::get_items1
        (const std::string &typeName)
{
    db_reply <std::map <uint32_t, std::string> > ret;
    
    byte type_id = asset_manager::type_to_byte(typeName);
    if ( type_id == (byte)asset_type::UNKNOWN ) {
        ret.status        = 0;
        ret.errtype       = DB_ERR;
        ret.errsubtype    = DB_ERROR_INTERNAL;
        ret.msg           = "Unsupported type of the elemtns";
        log_error (ret.msg);
        return ret;
    }

    try{
        tntdb::Connection conn = tntdb::connectCached(url);
        log_debug ("connection was successful");
        ret = persist::select_short_elements(conn, type_id);
        LOG_END;
        return ret;
    }
    catch (const std::exception &e) {
        ret.status        = 0;
        ret.errtype       = DB_ERR;
        ret.errsubtype    = DB_ERROR_INTERNAL;
        ret.msg           = e.what();
        LOG_END_ABNORMAL(e);
        return ret;
    } 
}
