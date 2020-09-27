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

#include "bssids_sqlite_parser.h"

#include <glog/logging.h>

BssidsSqliteParser::BssidsSqliteParser():
    SqliteParser("SELECT bssid, latitude as lat, longitude as lon, measurements as samples FROM wifi_zone")
{
}

void BssidsSqliteParser::parseRow(int cols, char* row[], char* names[], ILocationAggregator& aggregator)
{
    CHECK_EQ(cols, 4) << "Internal error: unexpected results from SQLite3 query!";
    // Note: we don't use 'samples' so don't check whether it's null or not.
    for (int i = 0; i < 3; ++i)
        if (!row[i]) return;
    addBssidEntry(row[0], row[1], row[2], aggregator);
}
