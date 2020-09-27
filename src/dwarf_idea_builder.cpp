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

#include "dwarf_idea_builder.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include <glog/logging.h>

#include <bitstream/DefaultOutputBitStream.hpp>
#include <function/ZRLT.hpp>
#include <transform/BWTS.hpp>
#include <transform/SBRT.hpp>

namespace {

const char* kFileSignature = "DwarfIdea";
const uint16_t kFileFormatVersion = 1;

template <typename T>
Bytes asVarInt(T value)
{
    Bytes result;
    T cur_value = value;

    while (true)
    {
        if (cur_value < 0x80)
	{
	    result.push_back(cur_value);
	    break;
	}
	else
	{
	    result.push_back((cur_value & 0x7F) | 0x80);
	    cur_value >>= 7;
	}
    }

    return result;
}

template <typename T>
void writeVarInt(std::ostream& os, T value) {
  Bytes value_bytes = asVarInt(value);
  os.write((const char*)value_bytes.data(), value_bytes.size());
}

template <typename T>
T clamp(T value, T min = 0, T max = 1)
{
    return std::max(min, std::min(value, max));
}

} // namespace

template <int KeySize, int ExtraDataSize>
DwarfIdeaBuilder<KeySize, ExtraDataSize>::Entry::Entry(
    const Key& key, const Point& point, const std::string& extra_data_str):
    key(key), point(point)
{
    std::copy(extra_data_str.begin(), extra_data_str.end(), &extra_data[0]);
}

template <int KeySize, int ExtraDataSize>
DwarfIdeaBuilder<KeySize, ExtraDataSize>::FseInfo::FseInfo():
    total_size(0),
    freqs(256, 0),
    freqs_normalized(256, 0)
{
}

template <int KeySize, int ExtraDataSize>
DwarfIdeaBuilder<KeySize, ExtraDataSize>::DwarfIdeaBuilder(
    float max_dist_error, uint16_t min_entries_per_block,
    uint16_t max_entries_per_block, uint8_t bounding_box_bits):
    min_entries_per_block_(min_entries_per_block),
    max_entries_per_block_(max_entries_per_block),
    bounding_box_bits_(bounding_box_bits),
    max_dist_error_(max_dist_error)
{
    CHECK_LT(bounding_box_bits_, 32) << "Too many bounding box bits requested!";
    bounding_box_max_index_ = (1 << (int32_t)bounding_box_bits_) - 1;
    bounding_box_lat_step_ = (kMaxLat - kMinLat) / bounding_box_max_index_;
    bounding_box_lon_step_ = (kMaxLon - kMinLon) / bounding_box_max_index_;
    double max_central_angle = max_dist_error / kEarthRadius;
    double sin_ca2 = std::sin(max_central_angle / 2.0);
    sin2_ca2_2_ = sin_ca2 * sin_ca2 / 2.0;
    dlat_ = 180.0 * 2.0 / M_PI * std::asin(std::sqrt(sin2_ca2_2_));
    dlon_coef_ = 180.0 * 2.0 / M_PI;
}

template <int KeySize, int ExtraDataSize>
void DwarfIdeaBuilder<KeySize, ExtraDataSize>::addLocation(
    const std::string& key_str, float lat, float lon,
    const std::string& extra_data)
{
    Key key;
    CHECK_EQ(key_str.size(), KeySize) << "Unexpected key size " <<
       key_str.size() << " for key " << key_str;
    std::copy(key_str.begin(), key_str.end(), key.begin());
    entries_.emplace_back(Entry(key, Point(lat, lon), extra_data));
}

template <int KeySize, int ExtraDataSize>
void DwarfIdeaBuilder<KeySize, ExtraDataSize>::buildIndex()
{
    index_dist_.resize(entries_.size() - 1, 0.0f);
    for (size_t i = 0; i < entries_.size() - 1; ++i)
    {
        index_dist_[i] = getDist(entries_[i].point, entries_[i + 1].point);
    }
    index_.push_back(0);
    findIndexSplit(0, entries_.size() - 1);
    std::sort(index_.begin(), index_.end());
}

template <int KeySize, int ExtraDataSize>
void DwarfIdeaBuilder<KeySize, ExtraDataSize>::findIndexSplit(
   size_t min_index, size_t max_index)
{
    if (max_index - min_index + 1 <= max_entries_per_block_)
        return;

    size_t split_index = min_index + min_entries_per_block_;
    float max_index_dist = index_dist_[split_index - 1];
    for (size_t i = split_index + 1; i <= max_index - min_entries_per_block_; ++i)
    {
        if (index_dist_[i - 1] > max_index_dist)
	{
	    max_index_dist = index_dist_[i - 1];
	    split_index = i;
	}
    }

    index_.push_back(split_index);

    findIndexSplit(min_index, split_index - 1);
    findIndexSplit(split_index, max_index);
}

template <int KeySize, int ExtraDataSize>
void DwarfIdeaBuilder<KeySize, ExtraDataSize>::writeFseHeader(std::ostream& os, FseInfo& fse_info)
{
    unsigned table_log = FSE_optimalTableLog(0, fse_info.total_size, 255);
    FSE_normalizeCount(
        &fse_info.freqs_normalized[0], table_log, &fse_info.freqs[0], fse_info.total_size, 255);
    fse_info.ctable.reset(FSE_createCTable(255, table_log));
    FSE_buildCTable(fse_info.ctable.get(), &fse_info.freqs_normalized[0], 255, table_log);
    Bytes buffer(FSE_NCountWriteBound(255, table_log), 0);
    size_t header_size = FSE_writeNCount(
            (void*)buffer.data(), buffer.size(), &fse_info.freqs_normalized[0],
	    255, table_log);
    putValue(uint32_t(header_size), os);
    os.write((const char*)buffer.data(), header_size);
}

template <int KeySize, int ExtraDataSize>
void DwarfIdeaBuilder<KeySize, ExtraDataSize>::encodePass(std::ostream& os, size_t iteration)
{
    if (iteration > 0)
    {
        writeFseHeader(os, keys_fse_info_);
        writeFseHeader(os, coords_fse_info_);

        if (ExtraDataSize)
        {
            writeFseHeader(os, extra_data_fse_info_);
        }

        index_offset_ = os.tellp();
        for (size_t i = 0; i < index_.size(); ++i)
        {
            const Entry& entry = entries_[index_[i]];
	    Bytes mapped_key = mapKey(entry.key);
            os.write((const char*)mapped_key.data(), mapped_key.size());
	    putValue(uint32_t(0), os);
        }
    }

    // This is likely not the most efficient approach:
    // 1) We need extra memory (although not too much compared to overall usage).
    // 2) The writing happens after all the calculation is done, interleaving it
    //    with calculations would be more efficient.
    // However, all the alternatives I can think of seem to be pretty complex
    // and not worth the hassle.
    std::vector<Bytes> result(iteration > 0 ? index_.size() : 0);

#pragma omp parallel for schedule(static)
    for (size_t i = 0; i < index_.size(); ++i)
    {
        size_t num_cur_entries =
	    ((i == index_.size() - 1) ? entries_.size() : index_[i + 1]) - index_[i];
	if (num_cur_entries > 0)
	{
            BlockInfo block_info = computeBlockInfo(i, num_cur_entries);
            flushBlock(iteration > 0 ? &result[i] : nullptr, iteration, block_info, i, num_cur_entries);
	}
    }

    if (iteration > 0)
    {
        for (size_t i = 0; i < index_.size(); ++i)
        {
            writeIndexPos(os, i);
            os.write((const char*)result[i].data(), result[i].size());
        }
    }
}

template <int KeySize, int ExtraDataSize>
BlockInfo DwarfIdeaBuilder<KeySize, ExtraDataSize>::computeBlockInfo(size_t index, size_t num_entries)
{
    size_t entry_index = index_[index];
    Point min_corner(kMaxLat, kMaxLon);
    Point max_corner(kMinLat, kMinLon);
    for (size_t i = 0; i < num_entries; ++i)
    {
        const Point& pnt = entries_[entry_index + i].point;
        min_corner.lat = std::min(min_corner.lat, pnt.lat);
        min_corner.lon = std::min(min_corner.lon, pnt.lon);
        max_corner.lat = std::max(max_corner.lat, pnt.lat);
        max_corner.lon = std::max(max_corner.lon, pnt.lon);
    }

    BlockInfo block_info;
    block_info.lat_min_index = clamp(
	int32_t(std::floor((min_corner.lat - kMinLat) / bounding_box_lat_step_)),
	0, bounding_box_max_index_);
    block_info.lon_min_index = clamp(
	int32_t(std::floor((min_corner.lon - kMinLon) / bounding_box_lon_step_)),
	0, bounding_box_max_index_);
    block_info.lat_max_index = clamp(
	int32_t(std::ceil((max_corner.lat - kMinLat) / bounding_box_lat_step_)),
	0, bounding_box_max_index_);
    block_info.lon_max_index = clamp(
	int32_t(std::ceil((max_corner.lon - kMinLon) / bounding_box_lon_step_)),
	0, bounding_box_max_index_);
    block_info.min_corner = Point(
        block_info.lat_min_index * bounding_box_lat_step_ + kMinLat,
	block_info.lon_min_index * bounding_box_lon_step_ + kMinLon);
    block_info.max_corner = Point(
        block_info.lat_max_index * bounding_box_lat_step_ + kMinLat,
	block_info.lon_max_index * bounding_box_lon_step_ + kMinLon);
    block_info.max_lat_diff = block_info.max_corner.lat - block_info.min_corner.lat;
    block_info.max_lon_diff = block_info.max_corner.lon - block_info.min_corner.lon;
    block_info.lat_bits = (int8_t)std::ceil(
        std::log(std::ceil(block_info.max_lat_diff / dlat_)) / log(2.0));
    block_info.lon_bits = 1;
    for (size_t i = 0; i < num_entries; ++i)
    {
        const Point& pnt = entries_[entry_index + i].point;
	// The error in coords representation comes from the rounding which is controlled by the number of bits
	// used to quantize the indices. Transforming the https://en.wikipedia.org/wiki/Haversine_formula we get
	// the following equality: sin^2(dCA / 2) = sin^2(dLAT / 2) + cos^2(LAT) * sin^2(dLON / 2).
	// Splitting the error between these two components equally, we get:
	// dLAT = 2 * asin(sqrt(sin^2(dCA / 2) / 2)),
	// dLON = 2 * asin(sqrt(sin^2(dCA / 2) / 2 / cos^2(LAT - dLAT))) where dCA is central angle.
	double cos_lat = std::cos(M_PI * (pnt.lat - dlat_) / 180.0);
	double dlon = dlon_coef_ * std::asin(std::sqrt(sin2_ca2_2_ / (cos_lat * cos_lat)));
	block_info.lon_bits = std::max(
	    block_info.lon_bits,
	    (int8_t)std::ceil(std::log(std::ceil(block_info.max_lon_diff / dlon)) / log(2.0)));
    }

    CHECK_LT(block_info.lat_bits, 32) << "Too many lat bits!";
    CHECK_LT(block_info.lon_bits, 32) << "Too many lon bits!";

    return block_info;
}

template <int KeySize, int ExtraDataSize>
Bytes DwarfIdeaBuilder<KeySize, ExtraDataSize>::compressBytes(const Bytes& input, size_t index)
{
    // Step 1: BWTS
    kanzi::BWTS bwts;
    Bytes bwts_output(input.size(), 0);
    auto bwts_sliced_input = kanzi::SliceArray<byte>((byte*)input.data(), input.size(), 0);
    auto bwts_sliced_output = kanzi::SliceArray<byte>((byte*)bwts_output.data(), bwts_output.size(), 0);
    CHECK(bwts.forward(bwts_sliced_input, bwts_sliced_output, input.size())) << "BWTS failed @ index " << index;

    // Step 2: SBRT
    kanzi::SBRT sbrt(kanzi::SBRT::MODE_RANK);
    Bytes sbrt_output(bwts_output.size(), 0);
    auto sbrt_sliced_input = kanzi::SliceArray<byte>((byte*)bwts_output.data(), bwts_output.size(), 0);
    auto sbrt_sliced_output = kanzi::SliceArray<byte>((byte*)sbrt_output.data(), sbrt_output.size(), 0);
    CHECK(sbrt.forward(sbrt_sliced_input, sbrt_sliced_output, bwts_output.size())) << "SBRT failed @ index " << index;

    // Step 3: ZRLT
    kanzi::ZRLT zrlt;
    Bytes zrlt_output(sbrt_output.size(), 0);
    auto zrlt_sliced_input = kanzi::SliceArray<byte>((byte*)sbrt_output.data(), sbrt_output.size(), 0);
    auto zrlt_sliced_output = kanzi::SliceArray<byte>((byte*)zrlt_output.data(), zrlt_output.size(), 0);
    if (zrlt.forward(zrlt_sliced_input, zrlt_sliced_output, sbrt_output.size()))
    {
        zrlt_output.resize(zrlt_sliced_output._index + 1);
        zrlt_output.back() = 0;
        return zrlt_output;
    }
    else
    {
        sbrt_output.push_back(1);
        return sbrt_output;
    }
}

template <int KeySize, int ExtraDataSize>
void DwarfIdeaBuilder<KeySize, ExtraDataSize>::updateFreqs(const Bytes& data, std::vector<unsigned>& freqs)
{
    for (int8_t c: data)
    {
#pragma omp atomic
        ++freqs[uint8_t(c)];
    }
}

template <int KeySize, int ExtraDataSize>
Bytes DwarfIdeaBuilder<KeySize, ExtraDataSize>::entropyCompress(const Bytes& data, FSE_CTable* ctable)
{
    Bytes output(FSE_compressBound(data.size() - 1), 0);
    size_t dst_size = FSE_compress_usingCTable(
        (void*)output.data(), output.size(), data.data(), data.size() - 1, ctable);
    uint8_t flags = data.back();
    bool ignore_fse = false;
    if (!dst_size || FSE_isError(dst_size))
    {
        LOG(ERROR) << "FSE compression failed with code " << dst_size << ", ignoring";
	ignore_fse = true;
    }
    else
    {
	ignore_fse = (dst_size > data.size());
    }

    if (ignore_fse)
    {
        output = data;
        output.pop_back();
        dst_size = output.size();
        flags |= 0x02;
    }
    else
    {
        output.resize(dst_size);
    }

    dst_size <<= 2;
    dst_size |= flags;

    Bytes enc_dst_size_bytes = asVarInt(dst_size);
    output.insert(output.begin(), enc_dst_size_bytes.begin(), enc_dst_size_bytes.end());
    return output;
}

template <int KeySize, int ExtraDataSize>
void DwarfIdeaBuilder<KeySize, ExtraDataSize>::writeIndexPos(std::ostream& os, size_t index)
{
    long cur_pos = os.tellp();
    os.seekp(index_offset_ + index * (mappedKeySize() + sizeof(uint32)) + mappedKeySize());
    putValue(uint32_t(cur_pos), os);
    os.seekp(cur_pos);
}

template <int KeySize, int ExtraDataSize>
void DwarfIdeaBuilder<KeySize, ExtraDataSize>::flushBlock(
    Bytes* output, size_t iteration, const BlockInfo& block_info,
    size_t index, size_t num_entries)
{
    Bytes encoded_keys = encodeKeys(index, num_entries);
    Bytes encoded_coords = encodeCoords(block_info, index, num_entries);
    Bytes encoded_extra_data =
        ExtraDataSize ? encodeExtraData(index, num_entries) : Bytes();
    if (iteration == 0)
    {
        Bytes compressed_keys = compressBytes(encoded_keys, index);
        updateFreqs(compressed_keys, keys_fse_info_.freqs);
#pragma omp atomic
        keys_fse_info_.total_size += compressed_keys.size();

        Bytes compressed_coords = compressBytes(encoded_coords, index);
        updateFreqs(compressed_coords, coords_fse_info_.freqs);
#pragma omp atomic
        coords_fse_info_.total_size += compressed_coords.size();

        if (ExtraDataSize)
        {
            Bytes compressed_extra_data = compressBytes(encoded_extra_data, index);
            updateFreqs(compressed_extra_data, extra_data_fse_info_.freqs);
#pragma omp atomic
            extra_data_fse_info_.total_size += compressed_extra_data.size();
        }
    }
    else
    {
	Bytes compressed_keys, compressed_coords, compressed_extra_data;

        compressed_keys = compressBytes(encoded_keys, index);
        compressed_keys = entropyCompress(compressed_keys, keys_fse_info_.ctable.get());

        compressed_coords = compressBytes(encoded_coords, index);
        compressed_coords = entropyCompress(compressed_coords, coords_fse_info_.ctable.get());

        if (ExtraDataSize)
        {
            compressed_extra_data = compressBytes(encoded_extra_data, index);
            compressed_extra_data = entropyCompress(compressed_extra_data, extra_data_fse_info_.ctable.get());
        }

        output->reserve(compressed_keys.size() + compressed_coords.size() + compressed_extra_data.size());
        std::copy(compressed_keys.begin(), compressed_keys.end(), std::back_inserter(*output));
        std::copy(compressed_coords.begin(), compressed_coords.end(), std::back_inserter(*output));

        if (ExtraDataSize)
        {
            std::copy(compressed_extra_data.begin(), compressed_extra_data.end(), std::back_inserter(*output));
	}
    }
}

template <int KeySize, int ExtraDataSize>
Bytes DwarfIdeaBuilder<KeySize, ExtraDataSize>::encodeKeys(size_t index, size_t num_entries)
{
    Bytes encoded_keys;
    size_t entry_index = index_[index];
    uint64_t prev_key = asInt<uint64_t>(mapKey(entries_[entry_index].key), true);
    // Skip the first key, it's written in the index anyway.
    for (size_t i = 1; i < num_entries; ++i)
    {
        uint64_t cur_key = asInt<uint64_t>(mapKey(entries_[entry_index + i].key), true);
        Bytes varint_key_diff = asVarInt(cur_key - prev_key);
        encoded_keys.insert(encoded_keys.end(), varint_key_diff.begin(), varint_key_diff.end());
        prev_key = cur_key;
    }
    return encoded_keys;
}

template <int KeySize, int ExtraDataSize>
Bytes DwarfIdeaBuilder<KeySize, ExtraDataSize>::mapKey(const Key& key) const
{
    // Default implementation just uses identity mapping.
    return Bytes(key.begin(), key.end());
}

template <int KeySize, int ExtraDataSize>
int DwarfIdeaBuilder<KeySize, ExtraDataSize>::mappedKeySize() const
{
    return KeySize;
}

template <int KeySize, int ExtraDataSize>
Bytes DwarfIdeaBuilder<KeySize, ExtraDataSize>::encodeCoords(
    const BlockInfo& block_info, size_t index, size_t num_entries)
{
    // Note: DefaultOutputBitStream requires ostream interface, hence the need
    // to use stringstream. Replace with smth else if performance is too bad.
    std::stringstream ss;
    kanzi::DefaultOutputBitStream bs(ss);
    bs.writeBits(block_info.lat_min_index, bounding_box_bits_);
    bs.writeBits(block_info.lon_min_index, bounding_box_bits_);
    bs.writeBits(block_info.lat_max_index, bounding_box_bits_);
    bs.writeBits(block_info.lon_max_index, bounding_box_bits_);
    bs.writeBits(block_info.lat_bits, 5);
    bs.writeBits(block_info.lon_bits, 5);

    uint32_t lat_mask = (1 << (block_info.lat_bits)) - 1;
    uint32_t lon_mask = (1 << (block_info.lon_bits)) - 1;

    size_t entry_index = index_[index];
    for (size_t i = 0; i < num_entries; ++i)
    {
        double lat_ratio = clamp(
	    (entries_[entry_index + i].point.lat - block_info.min_corner.lat) /
	    block_info.max_lat_diff);
        double lon_ratio = clamp(
	    (entries_[entry_index + i].point.lon - block_info.min_corner.lon) /
	    block_info.max_lon_diff);
        uint32_t lat_idx = std::min(
	    (uint32_t)std::round(lat_ratio * lat_mask), lat_mask);
        uint32_t lon_idx = std::min(
	    (uint32_t)std::round(lon_ratio * lon_mask), lon_mask);
        uint64_t combined = ((uint64_t)lon_idx << block_info.lat_bits) | lat_idx;
	float rec_lat = block_info.min_corner.lat +
	    block_info.max_lat_diff * lat_idx / ((1 << block_info.lat_bits) - 1);
	float rec_lon = block_info.min_corner.lon +
	    block_info.max_lon_diff * lon_idx / ((1 << block_info.lon_bits) - 1);
	double rec_dist = getDist(
	    entries_[entry_index + i].point, Point(rec_lat, rec_lon));
	bs.writeBits(combined, block_info.lat_bits + block_info.lon_bits);
    }
    bs.close();
    const std::string& buffer = ss.str();
    const uint8_t* data = (const uint8_t*)buffer.data();
    return Bytes(&data[0], &data[buffer.size()]);
}

template <int KeySize, int ExtraDataSize>
Bytes DwarfIdeaBuilder<KeySize, ExtraDataSize>::encodeExtraData(size_t index, size_t num_entries)
{
    Bytes encoded_data;
    size_t entry_index = index_[index];
    for (size_t i = 0; i < num_entries; ++i)
    {
        encoded_data.insert(
	    encoded_data.end(),
	    entries_[entry_index + i].extra_data.begin(),
	    entries_[entry_index + i].extra_data.end());
    }
    return encoded_data;
}

template <int KeySize, int ExtraDataSize>
void DwarfIdeaBuilder<KeySize, ExtraDataSize>::writeHeader(std::ostream& os) const
{
    os.write(kFileSignature, strlen(kFileSignature));
    putValue(uint16_t(kFileFormatVersion), os);
    putValue(uint16_t(KeySize), os);
    putValue(uint16_t(ExtraDataSize), os);
    putValue(uint32_t(entries_.size()), os);
    putValue(uint32_t(index_.size()), os);
    putValue(uint16_t(min_entries_per_block_), os);
    putValue(uint16_t(max_entries_per_block_), os);
    putValue(uint16_t(bounding_box_bits_), os);
    os.write((const char*)&max_dist_error_, sizeof(max_dist_error_));
    writeHeaderExtra(os);
    Bytes mapped_key = mapKey(entries_.back().key);
    os.write((const char*)mapped_key.data(), mapped_key.size());
}

template <int KeySize, int ExtraDataSize>
void DwarfIdeaBuilder<KeySize, ExtraDataSize>::writeHeaderExtra(std::ostream& os) const
{
    // No extra data by default.
    putValue(uint16_t(0), os);
}

template <int KeySize, int ExtraDataSize>
void DwarfIdeaBuilder<KeySize, ExtraDataSize>::build(std::ostream& os)
{
    // Find split points for index.
    buildIndex();

    // Stats gathering.
    encodePass(os, 0);

    // Actual data generation.
    writeHeader(os);
    encodePass(os, 1);
}

template class DwarfIdeaBuilder<kCellKeySize, kCellExtraDataSize>;
template class DwarfIdeaBuilder<kBssidKeySize, kBssidExtraDataSize>;
