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

#pragma once

#include "utils.h"

class IDwarfIdeaBuilder;

// Interface for the aggregation of the location data.
//
// If there are multiple entries for the given single key, the implementation
// of this interface should figure out how to reconcile these entries to produce
// the single entry per key at the output.
class ILocationAggregator
{
  public:
    virtual ~ILocationAggregator() {}

    // Adds new entry for the given key.
    //
    // The key is assumed to be binary-encoded already, e.g. in the case of CellIDs / BSSIDs it's already
    // converted from hex representation to the corresponding bytes.
    virtual void addLocation(const Bytes& key, float lat, float lon, int radius, int samples) = 0;

    // Perform the aggregation for all accumulated entries.
    //
    // For each key the implementation will perform the aggregation and then issue single
    // 'addLocation' call to 'builder' with the aggregated data per key.
    virtual void aggregate(IDwarfIdeaBuilder& builder) = 0;
};
