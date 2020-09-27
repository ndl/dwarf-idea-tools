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

#include "csv_parser.h"

#include "utils.h"

#include <fstream>
#include <string>
#include <vector>

#include <glog/logging.h>

void CsvParser::parse(const char* connection_spec, ILocationAggregator& aggregator)
{
    std::ifstream ifs(connection_spec);
    std::string line;
    while (std::getline(ifs, line))
    {
        parseLine(line, aggregator);
    }
}

void CsvParser::parse(const char* data, size_t size, ILocationAggregator& aggregator)
{
    if (data != nullptr && size > 0)
    {
      buffer_.append(data, size);
      size_t prev_pos = 0;
      for (size_t cur_pos; (cur_pos = buffer_.find('\n', prev_pos)) != std::string::npos; prev_pos = cur_pos + 1)
      {
          parseLine(buffer_.substr(prev_pos, cur_pos - prev_pos), aggregator);
      }
      buffer_.erase(0, prev_pos);
    }
    else
    {
        if (!buffer_.empty())
        {
            parseLine(buffer_, aggregator);
        }
        buffer_.clear();
    }
}
