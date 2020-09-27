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

#include "cells_sqlite_parser.h"

#include <glog/logging.h>

CellsSqliteParser::CellsSqliteParser(const std::string& blacklisted_standards):
    CellsParser(blacklisted_standards),
    SqliteParser("SELECT technology, mcc, mnc, area as lac, cid as cell, latitude as lat, longitude as lon, measurements as samples FROM cell_zone")
{
}

void CellsSqliteParser::parseRow(int cols, char* row[], char* names[], ILocationAggregator& aggregator)
{
    CHECK_EQ(cols, 8) << "Internal error: unexpected results from SQLite3 query!";
    for (int i = 0; i < 8; ++i)
        if (!row[i]) return;
    addCellEntry(
        row[0], row[1], row[2], row[3], row[4],
        row[5], row[6], "500", row[7], aggregator);
}
