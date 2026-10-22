#ifndef SEARCHING_H
#define SEARCHING_H

#include "Packet.h"
#include <vector>
#include <string>
#include <algorithm>
#include <cctype>

class Searching {
public:
    // Linear search for a Packet ID
    static Packet* searchById(std::vector<Packet>& packets, const std::string& id) {
        for (auto& p : packets) {
            if (p.packetId == id) return &p;
        }
        return nullptr;
    }

    // Search across multiple fields: Packet ID, source, destination,
    // current node, status or route. Used by the generic search endpoint.
    static std::vector<Packet> searchByQuery(const std::vector<Packet>& packets, const std::string& query) {
        std::vector<Packet> result;
        std::string q = toLower(query);
        if (q.empty()) return result;
        for (const auto& p : packets) {
            if (matches(p, q)) result.push_back(p);
        }
        return result;
    }

    static std::vector<Packet> searchBySource(const std::vector<Packet>& packets, const std::string& src) {
        std::vector<Packet> result;
        for (const auto& p : packets) {
            if (p.source == src) result.push_back(p);
        }
        return result;
    }

    static std::vector<Packet> searchByStatus(const std::vector<Packet>& packets, const std::string& status) {
        std::vector<Packet> result;
        std::string s = toLower(status);
        for (const auto& p : packets) {
            if (toLower(statusToString(p.status)) == s) result.push_back(p);
        }
        return result;
    }

private:
    static bool matches(const Packet& p, const std::string& q) {
        if (toLower(p.packetId).find(q) != std::string::npos) return true;
        if (toLower(p.source).find(q) != std::string::npos) return true;
        if (toLower(p.destination).find(q) != std::string::npos) return true;
        if (toLower(p.currentNode).find(q) != std::string::npos) return true;
        if (toLower(p.route).find(q) != std::string::npos) return true;
        if (toLower(statusToString(p.status)).find(q) != std::string::npos) return true;
        if (toLower(p.dropReason).find(q) != std::string::npos) return true;
        return false;
    }

    static std::string toLower(const std::string& s) {
        std::string out = s;
        std::transform(out.begin(), out.end(), out.begin(), ::tolower);
        return out;
    }

    static std::string statusToString(PacketStatus s) {
        switch (s) {
            case PacketStatus::CREATED: return "Created";
            case PacketStatus::QUEUED: return "Queued";
            case PacketStatus::IN_TRANSIT: return "In Transit";
            case PacketStatus::DELIVERED: return "Delivered";
            case PacketStatus::DROPPED: return "Dropped";
            case PacketStatus::REROUTED: return "Rerouted";
        }
        return "Unknown";
    }
};

#endif // SEARCHING_H