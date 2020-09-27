#pragma once

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

#include "ilocation_parser.h"

#include <string>

class CsvParser: public ILocationParser
{
  public:
    void parse(const char* connection_spec, ILocationAggregator& aggregator) override;

    // Called multiple times, once for each data block of the data source.
    //
    // 'data' contains block data of size 'size, not necessarily aligned to
    // any particular point in the input sequence.
    // 'addLocation' member function of 'aggregator' will be used to add all entries
    // that were successfully parsed.
    void parse(const char* data, size_t size, ILocationAggregator& aggregator);

  protected:
    virtual void parseLine(const std::string& line, ILocationAggregator& aggregator) = 0;

  private:
    std::string buffer_;
};
