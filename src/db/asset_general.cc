/*
Copyright (C) 2014-2015 Eaton

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

#include "db/assets.h"

#include <tntdb/transaction.h>
#include <locale.h>

#include "shared/log.h"
#include "shared/asset_types.h"
#include "defs.h"
#include "shared/ic.h"
#include "shared/utils++.h"


namespace persist {


//=============================================================================
// transaction is used
int
    update_dc_room_row_rack_group
        (tntdb::Connection  &conn,
         a_elmnt_id_t     element_id,
         const char      *element_name,
         a_elmnt_tp_id_t  element_type_id,
         a_elmnt_id_t     parent_id,
         zhash_t         *extattributes,
         const char      *status,
         a_elmnt_pr_t     priority,
         std::set <a_elmnt_id_t> const &groups,
         const std::string &asset_tag,
         std::string     &errmsg,
         zhash_t         *extattributesRO)
{
    LOG_START;

    tntdb::Transaction trans(conn);

    int affected_rows = 0;
    int ret1 = update_asset_element
        (conn, element_id, element_name, parent_id, status, priority,
         asset_tag.c_str(), affected_rows);

    if ( ( ret1 != 0 ) && ( affected_rows != 1 ) )
    {
        trans.rollback();
        errmsg = "check  element name, location, status, priority, asset_tag";
        log_error ("end: %s", errmsg.c_str());
        return 1;
    }

    auto ret2 = delete_asset_ext_attributes_with_ro
        (conn, element_id, false);
    if ( ret2.status == 0 )
    {
        trans.rollback();
        errmsg = "cannot erase old external attributes";
        log_error ("end: %s", errmsg.c_str());
        return 2;
    }

    auto ret3 = insert_into_asset_ext_attributes
        (conn, element_id, extattributes, false, errmsg);
    if ( ret3 != 0 )
    {
        trans.rollback();
        log_error ("end: %s", errmsg.c_str());
        return 3;
    }
    
    if(extattributesRO != NULL) {
        auto ret31 = insert_into_asset_ext_attributes
        (conn, element_id, extattributes, false, errmsg);
        if ( ret31 != 0 )
        {
            trans.rollback();
            log_error ("end: %s", errmsg.c_str());
            return 31;
        }
    }

    auto ret4 = delete_asset_element_from_asset_groups
        (conn, element_id);
    if ( ret4.status == 0 )
    {
        trans.rollback();
        errmsg = "cannot remove element from old groups";
        log_error ("end: %s", errmsg.c_str());
        return 4;
    }

    auto ret5 = insert_element_into_groups
        (conn, groups, element_id);
    if ( ( ret5.status == 0 ) && ( ret5.affected_rows != groups.size() ) )
    {
        trans.rollback();
        errmsg = "cannot insert device into all specified groups";
        log_error ("end: %s", errmsg.c_str());
        return 5;
    }

    trans.commit();
    LOG_END;
    return 0;
}

//=============================================================================
// transaction is used
int
    update_device
        (tntdb::Connection  &conn,
         a_elmnt_id_t     element_id,
         const char      *element_name,
         a_elmnt_tp_id_t  element_type_id,
         a_elmnt_id_t     parent_id,
         zhash_t         *extattributes,
         const char      *status,
         a_elmnt_pr_t     priority,
         std::set <a_elmnt_id_t> const &groups,
         std::vector <link_t> &links,
         const std::string &asset_tag,
         std::string     &errmsg,
         zhash_t      *extattributesRO)
{
    LOG_START;

    tntdb::Transaction trans(conn);

    int affected_rows = 0;
    int ret1 = update_asset_element
        (conn, element_id, element_name, parent_id, status, priority,
         asset_tag.c_str(), affected_rows);

    if ( ( ret1 != 0 ) && ( affected_rows != 1 ) )
    {
        trans.rollback();
        errmsg = "check  element name, location, status, priority, asset_tag";
        log_error ("end: %s", errmsg.c_str());
        return 1;
    }

    auto ret2 = delete_asset_ext_attributes_with_ro
        (conn, element_id, false);
    if ( ret2.status == 0 )
    {
        trans.rollback();
        errmsg = "cannot erase old external attributes";
        log_error ("end: %s", errmsg.c_str());
        return 2;
    }

    auto ret3 = insert_into_asset_ext_attributes
        (conn, element_id, extattributes, false, errmsg);
    if ( ret3 != 0 )
    {
        trans.rollback();
        log_error ("end: %s", errmsg.c_str());
        return 3;
    }
    
    if(extattributesRO != NULL) {
        auto ret31 = insert_into_asset_ext_attributes
        (conn, element_id, extattributesRO, true, errmsg);
        if ( ret31 != 0 )
        {
            trans.rollback();
            log_error ("end: %s", errmsg.c_str());
            return 31;
        }
    }

    auto ret4 = delete_asset_element_from_asset_groups
        (conn, element_id);
    if ( ret4.status == 0 )
    {
        trans.rollback();
        errmsg = "cannot remove element from old groups";
        log_error ("end: %s", errmsg.c_str());
        return 4;
    }

    auto ret5 = insert_element_into_groups
        (conn, groups, element_id);
    if ( ret5.affected_rows != groups.size() )
    {
        trans.rollback();
        errmsg = "cannot insert device into all specified groups";
        log_error ("end: %s", errmsg.c_str());
        return 5;
    }

    // links don't have 'dest' defined - it was not known until now; we have to fix it
    for ( auto &one_link: links )
    {
        one_link.dest = element_id;
    }
    auto ret6 = delete_asset_links_to
        (conn, element_id);
    if ( ret6.status == 0 )
    {
        trans.rollback();
        errmsg = "cannot remove old power sources";
        log_error ("end: %s", errmsg.c_str());
        return 6;
    }

    auto ret7 = insert_into_asset_links
           (conn, links);
    if ( ret7.affected_rows != links.size() )
    {
        trans.rollback();
        errmsg = "cannot add new power sources";
        log_error ("end: %s", errmsg.c_str());
        return 7;
    }

    trans.commit();
    LOG_END;
    return 0;
}

//=============================================================================
// transaction is used
db_reply_t
    insert_dc_room_row_rack_group
    (tntdb::Connection  &conn,
     const char      *element_name,
     a_elmnt_tp_id_t  element_type_id,
     a_elmnt_id_t     parent_id,
     zhash_t         *extattributes,
     const char      *status,
     a_elmnt_pr_t     priority,
     std::set <a_elmnt_id_t> const &groups,
     const std::string &asset_tag,
     zhash_t         *extattributesRO)
{
    LOG_START;
    if (extname_to_asset_id(element_name) != -1) {
        db_reply_t ret;
        ret.status     = 0;
        ret.errtype    = DB_ERR;
        ret.errsubtype = DB_ERROR_BADINPUT;
        ret.rowid      = 8;
        ret.msg        = std::string ("Element '").append (element_name).append ("' cannot be processed because of conflict. Most likely duplicate entry.");
        return ret;
    }
    setlocale (LC_ALL, ""); // move this to main?
    std::string iname = utils::strip (persist::typeid_to_type (element_type_id));
    log_debug ("  element_name = '%s/%s'", element_name, iname.c_str ());

    tntdb::Transaction trans(conn);
    auto reply_insert1 = insert_into_asset_element
                        (conn, iname.c_str (), element_type_id, parent_id,
                         status, priority, 0, asset_tag.c_str(), false);
    if ( reply_insert1.status == 0 )
    {
        trans.rollback();
        log_error ("end: element was not inserted");
        return reply_insert1;
    }
    auto element_id = reply_insert1.rowid;
    
    std::string err = "";

    int reply_insert2 = insert_into_asset_ext_attributes
        (conn, element_id, extattributes, false, err);
    if ( reply_insert2 != 0 )
    {
        trans.rollback();
        log_error ("end: device was not inserted (fail in ext_attributes)");
        db_reply_t ret;
        ret.status     = 0;
        ret.errtype    = DB_ERR;
        ret.errsubtype = DB_ERROR_BADINPUT;
        // too complicated, to transform from one format to onother
        ret.rowid      = -reply_insert2;
        ret.msg        = err;
        return ret;
    }
    
    if(extattributesRO != NULL) {
        err = "";

        int reply_insert21 = insert_into_asset_ext_attributes
            (conn, element_id, extattributesRO, true, err);
        if ( reply_insert21 != 0 )
        {
            trans.rollback();
            log_error ("end: device was not inserted (fail in ext_attributes)");
            db_reply_t ret;
            ret.status     = 0;
            ret.errtype    = DB_ERR;
            ret.errsubtype = DB_ERROR_BADINPUT;
            // too complicated, to transform from one format to onother
            ret.rowid      = -reply_insert21;
            ret.msg        = err;
            return ret;
        }
    }

    auto reply_insert3 = insert_element_into_groups (conn, groups, element_id);
    if ( ( reply_insert3.status == 0 ) && ( reply_insert3.affected_rows != groups.size() ) )
    {
        trans.rollback();
        log_info ("end: device was not inserted (fail into groups)");
        return reply_insert3;
    }

    if ( ( element_type_id == asset_type::DATACENTER ) ||
         ( element_type_id == asset_type::RACK) )
    {
        auto reply_insert4 = insert_into_monitor_device
            (conn, 1, element_name);
        if ( reply_insert4.status == 0 )
        {
            trans.rollback();
            log_info ("end: \"device\" was not inserted (fail monitor_device)");
            return reply_insert4;
        }
        auto reply_insert5 = insert_into_monitor_asset_relation
            (conn, reply_insert4.rowid, reply_insert1.rowid);
        if ( reply_insert5.status == 0 )
        {
            trans.rollback();
            log_info ("end: monitor asset link was not inserted (fail monitor asset relation)");
            return reply_insert5;
        }
    }

    trans.commit();
    LOG_END;
    return reply_insert1;
}

// because of transactions, previous function is not used here!
db_reply_t
    insert_device
       (tntdb::Connection &conn,
        std::vector <link_t> &links,
        std::set <a_elmnt_id_t> const &groups,
        const char    *element_name,
        a_elmnt_id_t   parent_id,
        zhash_t       *extattributes,
        a_dvc_tp_id_t  asset_device_type_id,
        const char    *asset_device_type_name,
        const char    *status,
        a_elmnt_pr_t   priority,
        const std::string &asset_tag,
        zhash_t       *extattributesRO)
{
    LOG_START;
    if (extname_to_asset_id(element_name) != -1) {
        db_reply_t ret;
        ret.status     = 0;
        ret.errtype    = DB_ERR;
        ret.errsubtype = DB_ERROR_BADINPUT;
        ret.rowid      = 8;
        ret.msg        = std::string ("Element '").append (element_name).append ("' cannot be processed because of conflict. Most likely duplicate entry.");
        return ret;
    }
    setlocale (LC_ALL, ""); // move this to main?
    std::string iname = utils::strip (persist::subtypeid_to_subtype (asset_device_type_id));
    log_debug ("  element_name = '%s/%s'", element_name, iname.c_str ());
    
    tntdb::Transaction trans(conn);

    auto reply_insert1 = insert_into_asset_element
                        (conn, iname.c_str (), asset_type::DEVICE, parent_id,
                         status, priority, asset_device_type_id, asset_tag.c_str(), false);
    if ( reply_insert1.status == 0 )
    {
        trans.rollback();
        log_info ("end: device was not inserted (fail in element)");
        return reply_insert1;
    }
    auto element_id = reply_insert1.rowid;
    std::string err = "";
   
    int reply_insert2 = insert_into_asset_ext_attributes
        (conn, element_id, extattributes, false, err);
    if ( reply_insert2 != 0 )
    {
        trans.rollback();
        log_error ("end: device was not inserted (fail in ext_attributes)");
        db_reply_t ret;
        ret.status     = 0;
        ret.errtype    = DB_ERR;
        ret.errsubtype = DB_ERROR_BADINPUT;
        // too complicated, to transform from one format to onother
        ret.rowid      = -reply_insert2;
        ret.msg        = err;
        return ret;
    }
    
    if(extattributesRO != NULL) {
        err = "";
        int reply_insert21 = insert_into_asset_ext_attributes
            (conn, element_id, extattributesRO, true, err);
        if ( reply_insert21 != 0 )
        {
            trans.rollback();
            log_error ("end: device was not inserted (fail in ext_attributes)");
            db_reply_t ret;
            ret.status     = 0;
            ret.errtype    = DB_ERR;
            ret.errsubtype = DB_ERROR_BADINPUT;
            // too complicated, to transform from one format to onother
            ret.rowid      = -reply_insert21;
            ret.msg        = err;
            return ret;
        }
    }

    auto reply_insert3 = insert_element_into_groups (conn, groups, element_id);
    if ( ( reply_insert3.status == 0 ) && ( reply_insert3.affected_rows != groups.size() ) )
    {
        trans.rollback();
        log_info ("end: device was not inserted (fail into groups)");
        return reply_insert3;
    }

    // links don't have 'dest' defined - it was not known until now; we have to fix it
    for ( auto &one_link: links )
    {
        one_link.dest = element_id;
    }

    auto reply_insert5 = insert_into_asset_links
           (conn, links);
    if ( reply_insert5.affected_rows != links.size() )
    {
        trans.rollback();
        log_info ("end: not all links were inserted (fail asset_link)");
        return reply_insert5;
    }

    // BIOS-1962: we do not use this classification. So ignore it.
    auto reply_select = select_monitor_device_type_id (conn, "not_classified");
    if ( reply_select.status == 1 )
    {
        auto reply_insert6 = insert_into_monitor_device
            (conn, reply_select.item, element_name);
        if ( reply_insert6.status == 0 )
        {
            trans.rollback();
            log_info ("end: device was not inserted (fail monitor_device)");
            return reply_insert6;
        }

        auto reply_insert7 = insert_into_monitor_asset_relation
            (conn, reply_insert6.rowid, reply_insert1.rowid);
        if ( reply_insert7.status == 0 )
        {
            trans.rollback();
            log_info ("end: monitor asset link was not inserted (fail monitor asset relation)");
            return reply_insert7;
        }
    }
    else if ( reply_select.errsubtype == DB_ERROR_NOTFOUND )
    {
        log_debug ("device should not being inserted into monitor part");
    }
    else
    {
        trans.rollback();
        log_warning ("end: some error in denoting a type of device in monitor part");
        return reply_select;

    }
    trans.commit();
    LOG_END;
    return reply_insert1;
}
//=============================================================================
db_reply_t
    delete_dc_room_row_rack
        (tntdb::Connection &conn,
        a_elmnt_id_t element_id)
{
    LOG_START;
    tntdb::Transaction trans(conn);

    auto reply_delete2 = delete_asset_element_from_asset_groups
                                                        (conn, element_id);
    if ( reply_delete2.status == 0 )
    {
        trans.rollback();
        log_info ("end: error occured during removing from groups");
        return reply_delete2;
    }

    m_dvc_id_t monitor_element_id = 0;
    {
        // find monitor counterpart
        int rv = convert_asset_to_monitor(conn, element_id, monitor_element_id);
        if ( rv != 0 )
        {
            db_reply_t ret = db_reply_new();
            ret.status = 0;
            ret.errtype = rv;
            ret.errsubtype = rv;
            log_error ("error during converting asset to monitor");
            return ret;
        }
    }

    auto reply_delete3 = delete_monitor_asset_relation_by_a
                                                (conn, element_id);
    if ( reply_delete3.status == 0 )
    {
        trans.rollback();
        log_info ("end: error occured during removing ma relation");
        return reply_delete3;
    }

    auto reply_delete4 = delete_asset_element (conn, element_id);
    if ( reply_delete4.status == 0 )
    {
        trans.rollback();
        log_info ("end: error occured during removing element");
        return reply_delete4;
    }

    trans.commit();
    LOG_END;
    return reply_delete4;
}

//=============================================================================
db_reply_t
    delete_group
        (tntdb::Connection &conn,
         a_elmnt_id_t element_id)
{
    LOG_START;
    tntdb::Transaction trans(conn);

    auto reply_delete2 = delete_asset_group_links (conn, element_id);
    if ( reply_delete2.status == 0 )
    {
        trans.rollback();
        log_info ("end: error occured during removing from groups");
        return reply_delete2;
    }

    auto reply_delete3 = delete_asset_element (conn, element_id);
    if ( reply_delete3.status == 0 )
    {
        trans.rollback();
        log_info ("end: error occured during removing element");
        return reply_delete3;
    }

    trans.commit();
    LOG_END;
    return reply_delete3;
}

//=============================================================================
db_reply_t
    delete_device
        (tntdb::Connection &conn,
         a_elmnt_id_t element_id)
{
    LOG_START;
    tntdb::Transaction trans(conn);

    auto reply_delete2 = delete_asset_element_from_asset_groups (conn, element_id);
    if ( reply_delete2.status == 0 )
    {
        trans.rollback();
        log_info ("end: error occured during removing from groups");
        return reply_delete2;
    }

    auto reply_delete3 = delete_asset_links_all (conn, element_id);
    if ( reply_delete3.status == 0 )
    {
        trans.rollback();
        log_info ("end: error occured during removing links");
        return reply_delete3;
    }

    auto reply_delete5 = delete_monitor_asset_relation_by_a
                                                (conn, element_id);
    if ( reply_delete5.status == 0 )
    {
        trans.rollback();
        log_info ("end: error occured during removing ma relation");
        return reply_delete5;
    }

    auto reply_delete6 = delete_asset_element (conn, element_id);
    if ( reply_delete6.status == 0 )
    {
        trans.rollback();
        log_info ("end: error occured during removing element");
        return reply_delete6;
    }

    trans.commit();
    LOG_END;
    return reply_delete6;
}

} // end namespace
