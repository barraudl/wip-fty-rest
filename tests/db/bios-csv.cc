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
 * \file bios-csv.cc
 * \author Michal Vyskocil <MichalVyskocil@Eaton.com>
 * \author Alena Chernikava <AlenaChernikava@Eaton.com>
 * \brief Not yet documented file; used in "diagnostic-information" in production
 */
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cxxtools/inifile.h>

#include "db/inout.h"
#include "shared/log.h"
#include "shared/csv.h"

static void
s_usage()
{
    std::cerr << "Usage: bios-csv [export|compare]" << std::endl;
    std::cerr << "       export     export csv file from current DB" << std::endl;
    std::cerr << "       compare    sourcefile_1 exportedfile_2  return exportedfile_2 corresponds with sourcefile_1" << std::endl;
    std::cerr << "Enviromental variables:" << std::endl;
    std::cerr << "      DB_USER     name of database user" << std::endl;
    std::cerr << "      DB_PASSWD   database password" << std::endl;
    std::cerr << "Config files:" << std::endl;
    std::cerr << "      ~/.my.cnf   read client/name, client/password if variables not specified" << std::endl;
}

static void
s_die_usage()
{
    s_usage();
    exit(EXIT_FAILURE);
}

static bool
s_compare(
        const char* file1,
        const char* file2)
{
    std::ifstream   sfile1{file1};
    std::ifstream   sfile2{file2};

    shared::CsvMap c1 = shared::CsvMap_from_istream(sfile1);
    shared::CsvMap c2 = shared::CsvMap_from_istream(sfile2);

    // 1. number of rows must be the same
    if (c1.rows() != c2.rows()) {
        log_error("different number of rows, %s: %zu != %s: %zu", file1, c1.rows(), file2, c2.rows());
        return false;
    }

    auto t1 = c1.getTitles();
    auto t2 = c2.getTitles();

    // 2. for each line and each title, check the fields
    for (size_t line = 1; line != c1.rows(); line++) {
        auto unused_columns = t2;
        for (const std::string& title: t1) {
            if ( t2.count(title) == 0 )
            {
                // c1 has column named AAAA, but c2 doesn't
                if ( !c1.get(line, title).empty() ) {
                    log_error("%s[%zu][%s] = %s has no equivalent is %s",
                            file1, line, title.c_str(), c1.get(line, title).c_str(),
                            file2
                            );
                    return false;
                }
                else
                    unused_columns.erase(title);
            }
            else
            {
                // c1 has column named AAAA and c2 does
                bool equals = false;
                if (title == "priority")
                {
                    auto p1 = persist::get_priority(c1.get(line, title));
                    auto p2 = persist::get_priority(c2.get(line, title));
                    equals = (p1 == p2);
                }
                else if (title == "type" || title == "sub_type" || title == "status") {
                    std::string foo1 = c1.get(line, title);
                    std::transform(foo1.cbegin(), foo1.cend(), foo1.begin(), ::tolower);

                    std::string foo2 = c2.get(line, title);
                    std::transform(foo2.cbegin(), foo2.cend(), foo2.begin(), ::tolower);

                    equals = (foo1 == foo2);
                }
                else {
                    equals = (c1.get(line, title) == c2.get(line, title));
                }
                if (!equals) {
                    log_error("%s[%zu][%s] = %s != %s[%zu][%s] = %s",
                            file1, line, title.c_str(), c1.get(line, title).c_str(),
                            file2, line, title.c_str(), c2.get(line, title).c_str()
                            );
                    return false;
                }
                else
                    unused_columns.erase(title);
            }
        }
        // c2 has more columns, than c1 -> the difference should be empty
        // but with one exception: "id"
        unused_columns.erase("id");
        for ( auto &one_col : unused_columns )
            if ( !c2.get(line, one_col).empty() )
            {
                log_error("%s[%zu][%s] = %s  has no equivalent in %s",
                        file2, line, one_col.c_str(), c2.get(line, one_col).c_str(),
                        file1
                        );
                return false;
            }
    }

    log_info("'%s' corresponds with '%s'", file2, file1);
    return true;
}

void
s_load_name_password ()
{
    if (::getenv ("DB_USER") && ::getenv ("DB_PASSWD"))
        return;

    try {
        struct passwd *pw = ::getpwuid (getuid ());
        const char* home = pw->pw_dir;

        cxxtools::IniFile inif {std::string {home} + ".my.cnf"};
        std::string user = inif.getValue ("client", "user", "");
        if (user != "")
            ::setenv ("DB_USER", user.c_str (), 1);
        std::string password = inif.getValue ("client", "password", "");
        if (password != "")
            ::setenv ("DB_PASSWD", password.c_str (), 1);

    }
    catch (const std::runtime_error &e) {
        return;
    }
}

int main(int argc, char** argv)
{
    if (argc <= 1)
        s_die_usage();

    s_load_name_password ();

    try {

        if (!strcmp(argv[1], "export"))
        {
            log_set_level(LOG_WARNING); //to suppress messages from src/db
            persist::export_asset_csv(std::cout);
        }
        else
        if (!strcmp(argv[1], "compare"))
        {
            log_set_level(LOG_INFO);
            if (argc < 4)
                s_die_usage();

            const char* file1 = argv[2];
            const char* file2 = argv[3];
            if (!s_compare(file1, file2))
                exit(EXIT_FAILURE);
        }
        else
        {
            log_error("Unknown command '%s'", argv[1]);
            exit(EXIT_FAILURE);
        }
    }
    catch (const std::exception& e) {
        log_error ("%s", e.what());
        exit (EXIT_FAILURE);
    }
    catch (...) {
        log_error ("Unknown exception");
        exit (EXIT_FAILURE);
    }
    exit(EXIT_SUCCESS);

}
