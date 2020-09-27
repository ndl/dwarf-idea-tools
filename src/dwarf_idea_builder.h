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

#include <array>
#include <memory>
#include <ostream>
#include <vector>

#include <fse.h>

#include "idwarf_idea_builder.h"
#include "utils.h"

struct BlockInfo
{
    int32_t lat_min_index, lon_min_index, lat_max_index, lon_max_index;
    Point min_corner, max_corner;
    double max_lat_diff, max_lon_diff;
    int8_t lat_bits, lon_bits;
};

template <int KeySize, int ExtraDataSize>
class DwarfIdeaBuilder: public IDwarfIdeaBuilder
{
  public:
    DwarfIdeaBuilder(float max_dist_error, uint16_t min_entries_per_block, uint16_t max_entries_per_block, uint8_t bounding_box_bits);

    void addLocation(const std::string& key, float lat, float lon, const std::string& extra_data) override;

    void build(std::ostream& os) override;

  protected:
    typedef std::array<uint8_t, KeySize> Key;
    typedef std::array<uint8_t, ExtraDataSize> ExtraData;

    struct __attribute__((__packed__)) Entry
    {
        Key key;
        Point point;
        ExtraData extra_data;

        Entry(const Key& key, const Point& point, const std::string& extra_data_str);
    };

    const std::vector<Entry>& getEntries() const { return entries_; }

    virtual Bytes mapKey(const Key& key) const;

    virtual int mappedKeySize() const;

    virtual void writeHeader(std::ostream& os) const;

    virtual void writeHeaderExtra(std::ostream& os) const;

  private:
    struct CTableDeleter
    {
        void operator() (FSE_CTable* table) { FSE_freeCTable(table); }
    };
    typedef std::unique_ptr<FSE_CTable, CTableDeleter> CTableUniquePtr;

    struct FseInfo
    {
        FseInfo();

        std::vector<unsigned> freqs;
        std::vector<short> freqs_normalized;
        CTableUniquePtr ctable;
	size_t total_size;
    };

    double sin2_ca2_2_, dlat_, dlon_coef_;
    float max_dist_error_;
    double bounding_box_lat_step_, bounding_box_lon_step_;
    int32_t bounding_box_max_index_;
    uint16_t min_entries_per_block_, max_entries_per_block_;
    uint8_t bounding_box_bits_;
    std::vector<Entry> entries_;
    std::vector<size_t> index_;
    std::vector<float> index_dist_;
    FseInfo keys_fse_info_, coords_fse_info_, extra_data_fse_info_;
    long index_offset_;

    void buildIndex();

    void findIndexSplit(size_t min_index, size_t max_index);

    Bytes compressBytes(const Bytes& input, size_t index);

    BlockInfo computeBlockInfo(size_t index, size_t num_entries);

    Bytes encodeKeys(size_t index, size_t num_entries);

    Bytes encodeCoords(const BlockInfo& block_info, size_t index, size_t num_entries);

    Bytes encodeExtraData(size_t index, size_t num_entries);

    void encodePass(std::ostream& os, size_t iteration);

    void writeIndexPos(std::ostream& os, size_t index);

    void flushBlock(Bytes* output, size_t iteration, const BlockInfo& block_info, size_t index, size_t num_entries);

    void updateFreqs(const Bytes& data, std::vector<unsigned>& freqs);

    void writeFseHeader(std::ostream& os, FseInfo& fse_info);

    Bytes entropyCompress(const Bytes& data, FSE_CTable* ctable);
};
