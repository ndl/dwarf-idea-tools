// DwarfIdea - offline network-based location format, tooling and libraries,
// see https://endl.ch/projects/dwarf-idea
//
// Copyright (C) 2019 - 2020 Alexander Tsvyashchenko <android@endl.ch>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "sqlite_parser.h"

#include <sqlite3.h>
#include <glog/logging.h>

SqliteParser::SqliteParser(const char* query):
    query_(query)
{
}

int SqliteParser::sqliteCallback(void* data, int cols, char** row, char** names)
{
    SqliteParser* parser = reinterpret_cast<SqliteParser*>(data);
    parser->parseRow(cols, row, names, *parser->aggregator_);
    return 0;
}

void SqliteParser::parse(const char* connection_spec, ILocationAggregator& aggregator)
{
    sqlite3* db;
    int rc = sqlite3_open(connection_spec, &db);
    if (rc) {
        LOG(ERROR) << "Can't open database: " << sqlite3_errmsg(db);
        sqlite3_close(db);
        return;
    }

    aggregator_ = &aggregator;

    char *zErrMsg = 0;
    rc = sqlite3_exec(
        db, query_.c_str(), &SqliteParser::sqliteCallback,
        reinterpret_cast<void*>(this), &zErrMsg);
    if (rc != SQLITE_OK) {
        LOG(ERROR) << "Sqlite error: " << zErrMsg;
        sqlite3_free(zErrMsg);
    }

    aggregator_ = nullptr;

    sqlite3_close(db);
}
