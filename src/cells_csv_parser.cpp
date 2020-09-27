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

#include "cells_csv_parser.h"

#include "utils.h"

#include <glog/logging.h>

namespace {

// OpenCell and Mozilla
const char* OpenCellFormat = "radio,mcc,net,area,cell,unit,lon,lat,range,samples,changeable,created,updated,averageSignal";

// mylnikov.org
const char* MylnikovFormat = "id,data_source,radio_type,mcc,mnc,lac,cellid,lat,lon,range,created,updated";

}

void CellsCsvParser::parseLine(const std::string& line, ILocationAggregator& aggregator)
{
    if (line == OpenCellFormat || line == MylnikovFormat)
    {
        // Do nothing, just skip the header line.
	return;
    }

    std::vector<std::string> tokens;
    split(line, ',', tokens);
    if (tokens.size() == 14)
    {
        // Note that lat, lon are reversed in OpenCellID CSV format.
        addCellEntry(
            tokens[0], tokens[1], tokens[2], tokens[3], tokens[4],
            tokens[7], tokens[6], tokens[8], tokens[9], aggregator);
    }
    else if (tokens.size() == 12)
    {
        addCellEntry(
            tokens[2], tokens[3], tokens[4], tokens[5], tokens[6],
            tokens[7], tokens[8], tokens[9], "1", aggregator);
    }
    else
    {
        LOG(ERROR) << "Failed to parse CSV line " << line << ": found " << tokens.size() << " tokens";
    }
}
