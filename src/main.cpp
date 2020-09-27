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

#include <archive.h>
#include <fstream>

#include <archive.h>
#include <archive_entry.h>
#include <glog/logging.h>
#include <gflags/gflags.h>

#include "cells_csv_parser.h"
#include "cells_sqlite_parser.h"
#include "bssids_csv_parser.h"
#include "bssids_sqlite_parser.h"
#include "cells_dwarf_idea_builder.h"
#include "simple_dwarf_idea_builder.h"
#include "location_aggregator.h"

DEFINE_string(cells_files, "", "Comma-separated list of files used to extract cells IDs. Archived files are supported.");
DEFINE_string(bssids_files, "", "Comma-separated list of files used to extract BSSIDs. Archived files are supported.");
DEFINE_string(blacklisted_standards, "CDMA,1xRTT,eHRPD,EVDO_0,EVDO_A,EVDO_B", "Comma-separated list of mobile standards to skip.");
DEFINE_double(max_dist_error, 50.0f, "Maximum distance error, in meters.");
DEFINE_int32(min_entries_per_block, 64, "Min number of entries per block.");
DEFINE_int32(max_entries_per_block, 256, "Max number of entries per block.");
DEFINE_int32(bounding_box_bits, 16, "Number of bits per coordinate in bounding box.");
DEFINE_string(cells_output_path, "", "If set, generate cells DB and output to the given path.");
DEFINE_string(bssids_output_path, "", "If set, generate BSSIDs DB and output to the given path.");
DEFINE_string(debug_cells_output_path, "", "If set, generate cells CSV output file.");
DEFINE_string(debug_bssids_output_path, "", "If set, generate BSSIDs CSV output file.");

namespace {

bool readArchive(const std::string& path, CsvParser& csv_parser, ILocationAggregator& aggregator, bool raw)
{
    struct archive *a = archive_read_new();
    archive_read_support_filter_all(a);
    if (raw)
    {
        archive_read_support_format_raw(a);
    }
    else
    {
        archive_read_support_format_all(a);
    }
    int r = archive_read_open_filename(a, path.c_str(), 8192);
    if (r == ARCHIVE_OK)
    { 
        struct archive_entry *entry;
        bool found_entry = false;
        while (archive_read_next_header(a, &entry) == ARCHIVE_OK)
        {
            found_entry = true;
            size_t size = 0;
            off_t offset = 0;
            const char* buffer = nullptr;
            while (archive_read_data_block(a, (const void**)&buffer, &size, &offset) == ARCHIVE_OK)
            {
                csv_parser.parse(buffer, size, aggregator);
            }
            csv_parser.parse(nullptr, 0, aggregator);
        }
        CHECK_EQ(archive_read_free(a), ARCHIVE_OK) <<
            "Failed to close archive file " << path;
        return found_entry;
    }
    else
    {
        return false;
    }
}

void process(
    const std::string& files_list,
    const std::string& debug_output_path,
    const std::string& output_path,
    CsvParser& csv_parser,
    SqliteParser& sqlite_parser,
    ILocationAggregator& aggregator,
    IDwarfIdeaBuilder& builder)
{
    std::vector<std::string> paths;
    split(files_list, ',', paths);
    for (const auto& path: paths)
    {
        if (path.find(".sqlite") != std::string::npos)
        {
            sqlite_parser.reset();
            sqlite_parser.parse(path.c_str(), aggregator);
        }
        else
        {
            csv_parser.reset();
            if (!readArchive(path, csv_parser, aggregator, false))
            {
                if (!readArchive(path, csv_parser, aggregator, true))
                {
                    csv_parser.parse(path.c_str(), aggregator);
                }
            }
        }
    }

    if (!debug_output_path.empty())
    {
        SimpleDwarfIdeaBuilder debug_builder;
        std::ofstream ofs(debug_output_path.c_str());
        aggregator.aggregate(debug_builder);
        debug_builder.build(ofs);
    }
    
    if (!output_path.empty())
    {
        std::ofstream ofs(output_path.c_str());
        aggregator.aggregate(builder);
        builder.build(ofs);
    }
}

void processCells()
{
    CellsCsvParser csv_parser(FLAGS_blacklisted_standards);
    CellsSqliteParser sqlite_parser(FLAGS_blacklisted_standards);
    LocationAggregator<kCellKeySize, kCellExtraDataSize> aggregator;
    CellsDwarfIdeaBuilder builder(
        FLAGS_max_dist_error,
        FLAGS_min_entries_per_block,
        FLAGS_max_entries_per_block,
        FLAGS_bounding_box_bits
    );
    process(
        FLAGS_cells_files,
        FLAGS_debug_cells_output_path,
        FLAGS_cells_output_path,
        csv_parser,
        sqlite_parser,
        aggregator,
        builder
    );
}

void processBssids()
{
    BssidsCsvParser csv_parser;
    BssidsSqliteParser sqlite_parser;
    LocationAggregator<kBssidKeySize, kBssidExtraDataSize> aggregator;
    DwarfIdeaBuilder<kBssidKeySize, kBssidExtraDataSize> builder(
        FLAGS_max_dist_error,
        FLAGS_min_entries_per_block,
        FLAGS_max_entries_per_block,
        FLAGS_bounding_box_bits
    );
    process(
        FLAGS_bssids_files,
        FLAGS_debug_bssids_output_path,
        FLAGS_bssids_output_path,
        csv_parser,
        sqlite_parser,
        aggregator,
        builder
    );
}

}

int main(int argc, char* argv[])
{
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    if (!FLAGS_debug_cells_output_path.empty() || !FLAGS_cells_output_path.empty())
    {
        processCells();
    }
    if (!FLAGS_debug_bssids_output_path.empty() || !FLAGS_bssids_output_path.empty())
    {
        processBssids();
    }
    return 0;
}
