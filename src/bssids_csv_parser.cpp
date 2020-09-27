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

#include "bssids_csv_parser.h"

#include "utils.h"

#include <vector>

#include <glog/logging.h>

namespace {

const char* kFormat[] =
{
    "id,bssid,lat,lon,updated,data",
    "bssid\tlat\tlon"
};

const int kFormatIndices[][3] =
{
    {1, 2, 3},
    {0, 1, 2},
};

const int kFormatColumns[] = { 6, 3 };

const char kFormatSeparator[] = { ',', '\t' };
}

void BssidsCsvParser::reset()
{
    CsvParser::reset();
    format_index_ = -1;
}

void BssidsCsvParser::parseLine(const std::string& line, ILocationAggregator& aggregator)
{
    std::vector<std::string> tokens;
    if (format_index_ == -1)
    {
        for (int i = 0; i < sizeof(kFormat) / sizeof(kFormat[0]); ++i)
        {
            if (line.compare(0, strlen(kFormat[i]), kFormat[i]) == 0)
            {
                format_index_ = i;
                break;
            }
        }
        CHECK_NE(format_index_, -1) << "Couldn't find suitable format for CSV header " << line;
        return;
    }

    split(line, kFormatSeparator[format_index_], tokens);
    if (tokens.size() != kFormatColumns[format_index_])
    {
        LOG(ERROR) << "Failed to parse CSV line " << line << ": found " << tokens.size() << " tokens";
    }
    else if (line.compare(0, strlen(kFormat[format_index_]), kFormat[format_index_]) == 0)
    {
        // Some files contain the header multiple times.
        // Do nothing, just skip the header line.
    }
    else
    {
        addBssidEntry(
            tokens[kFormatIndices[format_index_][0]],
            tokens[kFormatIndices[format_index_][1]],
            tokens[kFormatIndices[format_index_][2]],
            aggregator);
    }
}
