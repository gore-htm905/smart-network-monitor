#ifndef SORTING_H
#define SORTING_H

#include "Packet.h"
#include <vector>
#include <algorithm>

class Sorting {
public:
    enum class SortField { ID, PRIORITY, DELAY, SIZE, TIMESTAMP, STATUS, SOURCE, DESTINATION };
    enum class SortOrder { ASCENDING, DESCENDING };

    static void sortPackets(std::vector<Packet>& packets, SortField field, SortOrder order) {
        auto naturalIdCompare = [](const std::string& a, const std::string& b) {
            auto extractNum = [](const std::string& s) {
                int n = 0;
                for (char c : s) {
                    if (std::isdigit(c)) n = n * 10 + (c - '0');
                }
                return n;
            };
            int numA = extractNum(a);
            int numB = extractNum(b);
            if (numA != numB) return numA < numB;
            return a < b;
        };

        auto comparator = [field, order, naturalIdCompare](const Packet& a, const Packet& b) {
            bool asc = (order == SortOrder::ASCENDING);
            switch (field) {
                case SortField::ID:
                    return asc ? naturalIdCompare(a.packetId, b.packetId) : naturalIdCompare(b.packetId, a.packetId);
                case SortField::PRIORITY:
                    if (a.priority != b.priority) {
                        return asc ? (static_cast<int>(a.priority) < static_cast<int>(b.priority))
                                   : (static_cast<int>(a.priority) > static_cast<int>(b.priority));
                    }
                    return naturalIdCompare(a.packetId, b.packetId);
                case SortField::DELAY:
                    if (a.delay != b.delay) {
                        return asc ? (a.delay < b.delay) : (a.delay > b.delay);
                    }
                    return naturalIdCompare(a.packetId, b.packetId);
                case SortField::SIZE:
                    if (a.size != b.size) {
                        return asc ? (a.size < b.size) : (a.size > b.size);
                    }
                    return naturalIdCompare(a.packetId, b.packetId);
                case SortField::TIMESTAMP:
                    if (a.generationTime != b.generationTime) {
                        return asc ? (a.generationTime < b.generationTime) : (a.generationTime > b.generationTime);
                    }
                    return naturalIdCompare(a.packetId, b.packetId);
                case SortField::STATUS:
                    if (a.status != b.status) {
                        return asc ? (static_cast<int>(a.status) < static_cast<int>(b.status))
                                   : (static_cast<int>(a.status) > static_cast<int>(b.status));
                    }
                    return naturalIdCompare(a.packetId, b.packetId);
                case SortField::SOURCE:
                    if (a.source != b.source) {
                        return asc ? (a.source < b.source) : (a.source > b.source);
                    }
                    return naturalIdCompare(a.packetId, b.packetId);
                case SortField::DESTINATION:
                    if (a.destination != b.destination) {
                        return asc ? (a.destination < b.destination) : (a.destination > b.destination);
                    }
                    return naturalIdCompare(a.packetId, b.packetId);
                default:
                    return naturalIdCompare(a.packetId, b.packetId);
            }
        };
        std::stable_sort(packets.begin(), packets.end(), comparator);
    }
};

#endif // SORTING_H