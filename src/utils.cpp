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

#include "utils.h"

#include <cmath>

double getDist(const Point& pnt0, const Point& pnt1)
{
  double sin_lat_2 = std::sin((pnt0.lat - pnt1.lat) * M_PI / 180.0 / 2.0);
  double sin_lon_2 = std::sin((pnt0.lon - pnt1.lon) * M_PI / 180.0 / 2.0);
  return kEarthRadius * 2.0 * std::asin(std::sqrt(sin_lat_2 * sin_lat_2 +
      std::cos(pnt0.lat * M_PI / 180.0) * std::cos(pnt1.lat * M_PI / 180.0) * sin_lon_2 * sin_lon_2));
}
