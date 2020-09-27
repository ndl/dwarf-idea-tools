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

#include "bssids_parser.h"

#include "ilocation_aggregator.h"
#include "utils.h"

#include <glog/logging.h>

void BssidsParser::addBssidEntry(
    const std::string& bssid_str,
    const std::string& lat_str,
    const std::string& lon_str,
    ILocationAggregator& aggregator)
{
    Bytes bssid;
    bssid.reserve(kBssidKeySize);
    for (int i = 0; i < bssid_str.size(); i += 2)
    {
        int val = 0;
        try
        {
            val = std::stoi(bssid_str.substr(i, 2), nullptr, 16);
        }
        catch (std::invalid_argument)
        {
            VLOG(1) << "Failed to parse BSSID: '" << bssid_str << "'";
            return;
        }
	bssid.push_back(uint8_t(val));
    }

    if (bssid.size() != kBssidKeySize)
    {
	VLOG(1) << "Wrong BSSID format: '" << bssid_str << "'";
        return;
    }

    float lat, lon;
    try
    {
        lat = std::stof(lat_str);
        lon = std::stof(lon_str);
    }
    catch (std::out_of_range)
    {
        VLOG(1) << "Failed to parse coords for BSSID '" << bssid_str << "'";
        return;
    }

    if (lat == 0.0f && lon == 0.0f)
    {
        VLOG(1) << "Coordinates are zero, skipping BSSID '" << bssid_str << "'";
        return;
    }

    aggregator.addLocation(bssid, lat, lon, 0, 1);
}
