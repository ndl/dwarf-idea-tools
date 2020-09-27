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

#include "cells_dwarf_idea_builder.h"

#include <glog/logging.h>

#include <algorithm>
#include <limits>

namespace {

uint16_t char2short(char c)
{
    return uint8_t(c);
}

}

void CellsDwarfIdeaBuilder::build(std::ostream& os)
{
    // Accumulate MCC and MNC values that are actually
    // present in the data so that we can remap them later.
    for (const auto& entry: getEntries())
    {
        uint16_t mcc = (char2short(entry.key[0]) << 8) | char2short(entry.key[1]);
        uint16_t mnc = (char2short(entry.key[2]) << 8) | char2short(entry.key[3]);
        uint32_t mcc_mnc = (uint32_t(mcc) << 16) | mnc;
        if (!mccs_mncs_map_.count(mcc_mnc))
        {
            CHECK_LT(mccs_mncs_map_.size(), std::numeric_limits<uint16_t>::max()) << "Too many MCC, MNC pairs found!";
	    uint16_t mcc_mnc_index = mccs_mncs_map_.size();
            mccs_mncs_map_.insert(std::make_pair(mcc_mnc, mcc_mnc_index));
            mccs_mncs_values_.push_back(mcc_mnc);
        }
    }

    DwarfIdeaBuilder<kCellKeySize, kCellExtraDataSize>::build(os);
}

void CellsDwarfIdeaBuilder::writeHeaderExtra(std::ostream& os) const
{
    // Serialize MCCs / MNCs mapping.
    putValue(static_cast<uint16_t>(mccs_mncs_values_.size()), os);
    for (const auto& mcc_mnc: mccs_mncs_values_)
    {
        putValue(mcc_mnc, os);
    }
}

Bytes CellsDwarfIdeaBuilder::mapKey(const Key& key) const
{
    std::string key_str;
    uint16_t mcc = (char2short(key[0]) << 8) | char2short(key[1]);
    uint16_t mnc = (char2short(key[2]) << 8) | char2short(key[3]);
    uint32_t mcc_mnc = (uint32_t(mcc) << 16) | mnc;
    auto mcc_mnc_it = mccs_mncs_map_.find(mcc_mnc);
    CHECK(mcc_mnc_it != mccs_mncs_map_.end()) << "Cannot find MCC, MNC " << mcc << ", " << mnc;
    key_str.append(1, static_cast<char>((mcc_mnc_it->second >> 8) & 0xFF));
    key_str.append(1, static_cast<char>(mcc_mnc_it->second & 0xFF));
    std::copy(&key[4], &key[key.size()], std::back_inserter(key_str));
    return Bytes(key_str.begin(), key_str.end());
}

int CellsDwarfIdeaBuilder::mappedKeySize() const
{
    return 8;
}
