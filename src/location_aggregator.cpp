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

#include "location_aggregator.h"

#include "idwarf_idea_builder.h"

#include <glog/logging.h>

namespace {

const int kMinRadius = 500;
const int kRadiusStep = 100;
const int kMaxRadius = 500 + kRadiusStep * 15;
const int kMaxSamples = 15;
const float kDistanceThreshold = 500.0f;

}

template <int KeySize, int ExtraDataSize>
void LocationAggregator<KeySize, ExtraDataSize>::addLocation(const Bytes& key_bytes, float lat, float lon, int radius, int samples)
{
    CHECK_EQ(key_bytes.size(), KeySize) << "Expected key size " << KeySize << " but got " << key_bytes.size();

    // Do some basic sanitisation, we'll also repeat it later on the result.
    radius = std::min(kMaxRadius, std::max(kMinRadius, radius));
    samples = std::max(0, std::min(kMaxSamples, samples));

    // TODO: is there any more elegant way of initializing this?
    typename Entries::key_type key = { 0 };
    std::copy(key_bytes.begin(), key_bytes.end(), key.data());
    entries_.insert(
        std::make_pair(key, EntryDetails(Point(lat, lon), radius, samples)));
}

template <int KeySize, int ExtraDataSize>
typename LocationAggregator<KeySize, ExtraDataSize>::EntryDetails
LocationAggregator<KeySize, ExtraDataSize>::averageData(const typename Entries::key_type& key) const
{
    auto range = entries_.equal_range(key);
    const auto len = std::distance(range.first, range.second);
    if (len < 2)
    {
        return range.first->second;
    }
    else if (len == 2)
    {
        auto next = range.first;
        std::advance(next, 1);

        const EntryDetails& entry0 = range.first->second;
        const EntryDetails& entry1 = next->second;
        const Point pnt0 = entry0.point;
        const Point pnt1 = entry1.point;
        if (getDist(pnt0, pnt1) < kDistanceThreshold)
        {
            // The points are "close enough" - aggregate them.
            return EntryDetails(
                Point((pnt0.lat + pnt1.lat) / 2, (pnt0.lon + pnt1.lon) / 2),
                (entry0.radius + entry1.radius) / 2,
                entry0.samples + entry1.samples);
        }
        else if (entry0.samples != entry1.samples)
        {
            // Return the one that claims the higher number of samples.
            return entry0.samples > entry1.samples ? entry0 : entry1;
        }
        else if (sqr(pnt0.lat) + sqr(pnt0.lon) < sqr(pnt1.lat) + sqr(pnt1.lon))
        {
            // We have no way of knowing which one is better so just take
            // the smallest one.
            return entry0;
        }
        else
        {
            return entry1;
        }
    }

    float best_dist = 2.0 * sqr(360.0);
    Point median = range.first->second.point;
    // Find the "median" defined as "minimal distance sum to every point".
    // Note: there are not that many locations with the same key,
    // so O(N^2) complexity is not an issue.
    for (auto entry0_it = range.first; entry0_it != range.second; ++entry0_it)
    {
        Point pnt0 = entry0_it->second.point;
        float dist = 0.0;
        for (auto entry1_it = range.first; entry1_it != range.second; ++entry1_it)
        {
            Point pnt1 = entry1_it->second.point;
            // Note: we're using an (incorrect) approximation here, but
            // we don't care too much about the absolute accuracy of it.
            dist += sqr(pnt0.lat - pnt1.lat) + sqr(pnt0.lon - pnt1.lon);
        }
        if (dist < best_dist)
        {
            median = pnt0;
            best_dist = dist;
        }
    }

    float sum_lat = 0.0;
    float sum_lon = 0.0;
    float sum_radius = 0.0;
    float sum_samples = 0.0;
    int count = 0;
    // Leave only the points within the kDistanceThreshold to median and
    // aggregate them. In the worst case, this is just the median itself.
    for (auto entry_it = range.first; entry_it != range.second; ++entry_it)
    {
        const EntryDetails& entry = entry_it->second;
        Point pnt = entry.point;
        float dist = getDist(pnt, median);
        if (dist < kDistanceThreshold)
        {
            sum_lat += pnt.lat;
            sum_lon += pnt.lon;
            sum_radius += entry.radius;
            sum_samples += entry.samples;
            ++count;
        }
    }
    return EntryDetails(
        Point(sum_lat / count, sum_lon / count),
        sum_radius / count,
        sum_samples);
}

template <int KeySize, int ExtraDataSize>
void LocationAggregator<KeySize, ExtraDataSize>::aggregate(IDwarfIdeaBuilder& builder)
{
    // We assume either 0 or 1 byte extra data size below - check that's the case indeed.
    static_assert(ExtraDataSize == 0 || ExtraDataSize == 1, "Unsupported extra data size requested!");

    for (auto it = entries_.begin(); it != entries_.end(); it = entries_.upper_bound(it->first))
    {
        EntryDetails entry = averageData(it->first);
        std::string extra_data;
        if (ExtraDataSize)
        {
            entry.radius = std::min(kMaxRadius, std::max(kMinRadius, entry.radius));
            entry.samples = std::max(0, std::min(kMaxSamples, entry.samples));
            uint8_t value = (entry.samples << 4) | ((entry.radius - kMinRadius) / kRadiusStep);
            extra_data.append(1, static_cast<char>(value));
        }
        std::string key(&it->first.data()[0], &it->first.data()[it->first.size()]);
        builder.addLocation(key, entry.point.lat, entry.point.lon, extra_data);
    }
}

template class LocationAggregator<kCellKeySize, kCellExtraDataSize>;
template class LocationAggregator<kBssidKeySize, kBssidExtraDataSize>;
