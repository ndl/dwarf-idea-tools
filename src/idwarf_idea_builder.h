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

#include <ostream>
#include <string>

// The interface for actual location DB construction.
//
// 'addLocation' will be called exactly once per each key and once
// all entries are added - 'build' method will be called.
class IDwarfIdeaBuilder
{
  public:
    virtual ~IDwarfIdeaBuilder() {}

    // Adds new key + its data.
    virtual void addLocation(const std::string& key, float lat, float lon, const std::string& extra_data) = 0;

    // Constructs the actual DB.
    virtual void build(std::ostream& os) = 0;
};
