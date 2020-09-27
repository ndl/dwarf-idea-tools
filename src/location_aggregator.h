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

#include "ilocation_aggregator.h"
#include "utils.h"

#include <array>
#include <map>

template <int KeySize, int ExtraDataSize>
class LocationAggregator: public ILocationAggregator
{
  public:
    void addLocation(const Bytes& key, float lat, float lon, int radius, int samples) override;

    void aggregate(IDwarfIdeaBuilder& builder) override;

  private:
    struct __attribute__((__packed__)) EntryDetails
    {
        Point point;
        int radius, samples;

        EntryDetails(): radius(0), samples(0) {}
        EntryDetails(const Point& pnt, int radius, int samples):
            point(pnt), radius(radius), samples(samples) {}
    };
    typedef std::array<uint8_t, KeySize> Key;
    typedef std::multimap<Key, EntryDetails> Entries;
    Entries entries_;

    EntryDetails averageData(const typename Entries::key_type& key) const;
};
