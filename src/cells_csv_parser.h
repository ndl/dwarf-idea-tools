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

#include "cells_parser.h"
#include "csv_parser.h"

class CellsCsvParser: public CellsParser, public CsvParser
{
  public:
    CellsCsvParser(const std::string& blacklisted_standards): CellsParser(blacklisted_standards) { }

  private:
    void parseLine(const std::string& line, ILocationAggregator& aggregator) override;
};
