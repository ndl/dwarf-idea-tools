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

#include "simple_dwarf_idea_builder.h"

#include <iomanip>

SimpleDwarfIdeaBuilder::SimpleDwarfIdeaBuilder() {
}

void SimpleDwarfIdeaBuilder::addLocation(const std::string& key, float lat, float lon, const std::string& extra_data) {
  // Note: using stringstream here is sub-optimal, but doesn't really matter for debugging purposes.
  std::stringstream os;
  os << std::hex << std::setfill('0');
  for (int i = 0; i < key.size(); ++i) {
    os << std::setw(2) << (unsigned)(unsigned char)(key[i]);
  }
  os << "\t" << std::dec << std::setw(1) << std::setprecision(8) << lat << "\t" << lon;
  if (!extra_data.empty()) {
    os << "\t" << std::hex << std::setfill('0') << std::setw(2);
    for (int i = 0; i < extra_data.size(); ++i) {
      os << std::setw(2) << (unsigned)(unsigned char)(extra_data[i]);
    }
  }
  entries_.emplace_back(os.str());
}

void SimpleDwarfIdeaBuilder::build(std::ostream& os) {
  for (const auto& str: entries_) {
    os << str << std::endl;
  }
}
