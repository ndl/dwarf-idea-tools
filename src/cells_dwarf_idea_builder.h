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

#include "dwarf_idea_builder.h"
#include "utils.h"

#include <unordered_map>

// Specialization of DwarfIdeaBuilder that performs cells-specific keys encoding.
//
// In particular, MCCs and MNCs take 2 bytes each, but the number of their real-world
// combinations is actually quite small (< 1000), so by replacing (MCC, MNC) with
// a single 2 byte index in the constructed mapping we can save 2 bytes per key
// at the expense of one extra lookup + one small extra mapping stored.
class CellsDwarfIdeaBuilder: public DwarfIdeaBuilder<kCellKeySize, kCellExtraDataSize>
{
  public:
    using DwarfIdeaBuilder<kCellKeySize, kCellExtraDataSize>::DwarfIdeaBuilder;

    void build(std::ostream& os) override;

  protected:
    Bytes mapKey(const Key& key) const override;

    int mappedKeySize() const override;

    void writeHeaderExtra(std::ostream& os) const override;

  private:
    std::unordered_map<uint32_t, uint16_t> mccs_mncs_map_;
    std::vector<uint32_t> mccs_mncs_values_;
};
