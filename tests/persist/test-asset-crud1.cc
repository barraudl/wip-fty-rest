/*
 *
 * Copyright (C) 2015 Eaton
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
 * \file test-asset-crud1.cc
 * \author Alena Chernikava <AlenaChernikava@Eaton.com>
 * \brief Not yet documented file
 */
#include <catch.hpp>

#include "dbpath.h"
#include "log.h"

#include "assetcrud.h"
#include "db/assets.h"

TEST_CASE("UPDATE asset element #1","[db][CRUD][update][asset_element][crud_test.sql][999]")
{
    log_open ();
    log_info ("=============== UPDATE asset element #1 ==================");

    tntdb::Connection conn;
    REQUIRE_NOTHROW ( conn = tntdb::connectCached(url) );

    // assumption
    // (id_asset_element, name, id_type, id_parent, status, priority) VALUES (6, "asset_crud_DEVICE_update",6, 1, "nonactive", 3);
    a_elmnt_id_t  asset_element_id = 6;
    const char   *element_name = "asset_crud_DEVICE_update";
    a_elmnt_id_t  parent_id = 1;
    const char   *status = "nonactive";
    a_elmnt_pr_t  priority = 3;
    const char   *asset_tag ="1234567890";

    int32_t n = 0;
    int r = persist::update_asset_element
        (conn, asset_element_id, element_name, parent_id, status, priority, asset_tag, n);
    REQUIRE ( r == 0 );
    REQUIRE ( n == 1 );

    auto reply_select = persist::select_asset_element_web_byId(conn, asset_element_id);
    REQUIRE ( reply_select.status == 1 );
    auto item = reply_select.item;
    REQUIRE ( item.id == asset_element_id );
    REQUIRE ( item.name == element_name );
    REQUIRE ( item.status == status );
    REQUIRE ( item.priority == priority );
    REQUIRE ( item.parent_id == parent_id );
    REQUIRE ( item.asset_tag == asset_tag );

    const char   *element_name_new = "asset_crud_DEVICE_updateN";
    a_elmnt_id_t  parent_id_new = 2;
    const char   *status_new = "retired";
    a_elmnt_pr_t  priority_new = 1;

    r = persist::update_asset_element
        (conn, asset_element_id, element_name_new, parent_id_new, status_new,
         priority_new, asset_tag, n);
    REQUIRE ( r == 0 );
    REQUIRE ( n == 1 );

    reply_select = persist::select_asset_element_web_byId(conn, asset_element_id);
    REQUIRE ( reply_select.status == 1 );
    item = reply_select.item;
    REQUIRE ( item.id == asset_element_id );
    // UNTIL BIOS-1028 is implemented name could not be updated
    REQUIRE ( item.name == element_name );
    REQUIRE ( item.status == status_new );
    REQUIRE ( item.priority == priority_new );
    REQUIRE ( item.parent_id == parent_id_new );

    // return to the starting point
    r = persist::update_asset_element
        (conn, asset_element_id, element_name, parent_id, status, priority, asset_tag, n);
    REQUIRE ( r == 0 );
}
