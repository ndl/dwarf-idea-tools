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

#include "cells_parser.h"

#include "ilocation_aggregator.h"
#include "utils.h"

#include <algorithm>
#include <string>
#include <vector>

#include <glog/logging.h>

CellsParser::CellsParser(const std::string& blacklisted_standards)
{
    split(blacklisted_standards, ',', blacklisted_standards_);
}

void CellsParser::addCellEntry(
    const std::string& standard_str,
    const std::string& mcc_str,
    const std::string& mnc_str,
    const std::string& lac_str,
    const std::string& cell_str,
    const std::string& lat_str,
    const std::string& lon_str,
    const std::string& radius_str,
    const std::string& samples_str,
    ILocationAggregator& aggregator)
{
    if (std::find(
            blacklisted_standards_.begin(),
            blacklisted_standards_.end(),
            standard_str) ==
        blacklisted_standards_.end())
    {
        int mcc, mnc, lac, cell, radius, samples;
        float lat, lon;
        try
        {
            mcc = std::stoi(mcc_str);
            mnc = std::stoi(mnc_str);
            lac = std::stoi(lac_str);
            cell = std::stol(cell_str);
            lat = std::stof(lat_str);
            lon = std::stof(lon_str);
            radius = std::stoi(radius_str);
            samples = std::stoi(samples_str);
        }
        catch (...)
        {
            VLOG(1) << "Failed to parse cell " <<
                standard_str << ", " << mcc_str << ", " <<
                mnc_str << ", " << lac_str << ", " <<
                cell_str << ", " << lon_str << ", " <<
                lat_str << ", " << radius_str << ", " <<
                samples_str;
            return;
        }

        // According to https://en.wikipedia.org/wiki/Mobile_country_code, the valid range for MCC is [200 - 800)
        if (mcc < 200 || mcc >= 800 || mnc < 0 || mnc > 0xFFFF || lac < 0 || lac > 0xFFFF || cell < 0 || cell > 0xFFFFFFFF)
        {
             VLOG(1) << "Cell data is out of bounds: Standard = " << standard_str <<
                 ", MCC = " << mcc << ", MNC = " << mnc <<
                 ", LAC = " << lac << ", CELL = " << cell;
             return;
        }

        if (lat == 0.0f && lon == 0.0f)
        {
             VLOG(1) << "Coordinates are zero, skipping the cell: Net = " << standard_str <<
                 ", MCC = " << mcc << ", MNC = " << mnc <<
                 ", LAC = " << lac << ", CELL = " << cell;
             return;
        }

        Bytes key;
        appendBytes(asBytes(uint16_t(mcc), true), key);
        appendBytes(asBytes(uint16_t(mnc), true), key);
        appendBytes(asBytes(uint16_t(lac), true), key);
        appendBytes(asBytes(uint32_t(cell), true), key);
        aggregator.addLocation(key, lat, lon, radius, samples);
    }
}
