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
#include <sstream>
#include <vector>

#include <glog/logging.h>

constexpr int kBssidKeySize = 6;
constexpr int kCellKeySize = 10;
constexpr int kCellExtraDataSize = 1;
constexpr int kBssidExtraDataSize = 0;
constexpr float kMinLat = -90.0;
constexpr float kMaxLat = 90.0;
constexpr float kMinLon = -180.0;
constexpr float kMaxLon = 180.0;
constexpr double kEarthRadius = 6371000.0;

typedef std::vector<uint8_t> Bytes;

template <typename T>
T sqr(T v) { return v * v; }

template <typename Result>
void split(const std::string& input, char delim, Result& result)
{
    for (size_t prev_pos = 0, cur_pos = 0; cur_pos != std::string::npos; prev_pos = cur_pos + 1)
    {
        cur_pos = input.find(delim, prev_pos);
        result.push_back(input.substr(prev_pos, cur_pos - prev_pos));
    }
}

template <typename T>
T asInt(Bytes bytes, bool big_endian = false)
{
    CHECK_LE(bytes.size(), sizeof(T)) << "Cannot fit " << bytes.size() << " bytes into type " << typeid(T).name();
    T int_val = 0;
    for (int i = 0; i < bytes.size(); ++i)
    {
        int_val |= (T)bytes[big_endian ? bytes.size() - i - 1 : i] << (8 * i);
    }
    return int_val;
}

template <typename T>
Bytes asBytes(T value, bool big_endian=false)
{
    Bytes output(sizeof(T));
    for (int i = 0; i < sizeof(value); ++i)
    {
        output[big_endian ? sizeof(value) - i - 1 : i] = uint8_t((value >> (8 * i)) & 0xFF);
    }
    return output;
}

template <typename T>
void putValue(T value, std::ostream& os, bool big_endian=false)
{
    Bytes bytes = asBytes(value, big_endian);
    os.write((const char*)bytes.data(), bytes.size());
}

template <typename T>
void appendBytes(const T& src_bytes, Bytes& dst_bytes)
{
    dst_bytes.insert(dst_bytes.end(), src_bytes.begin(), src_bytes.end());
}

struct __attribute__((__packed__)) Point
{
    float lat, lon;

    Point(): lat(0.0), lon(0.0) {}
    Point(float lat, float lon): lat(lat), lon(lon) {}
};

double getDist(const Point& pnt0, const Point& pnt1);
