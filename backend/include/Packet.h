#ifndef PACKET_H
#define PACKET_H

#include <string>
#include <vector>

enum class Priority {
    LOW,
    MEDIUM,
    HIGH
};

enum class PacketStatus {
    CREATED,
    QUEUED,
    IN_TRANSIT,
    DELIVERED,
    DROPPED,
    REROUTED
};

struct Packet {
    std::string packetId;
    std::string source;
    std::string destination;
    int size;
    Priority priority;
    long long generationTime;
    long long transmissionTime;
    long long deliveryTime;
    long long delay;
    PacketStatus status;

    // The route as a single string ("N1 -> R1 -> R2 -> N8")
    std::string route;
    // Ordered list of hops in the route (including source and destination)
    std::vector<std::string> routeVec;
    // Index of the node the packet is currently at within routeVec
    int hopIndex;
    // Current location / last location of the packet
    std::string currentNode;
    // Reason a packet was dropped
    std::string dropReason;
    // How many times the packet has been rerouted
    int rerouteCount;
    // Hops remaining (used with a Stack for route tracking)
    int hopsRemaining;

    // Complete route history preservation
    std::string initialRoute;
    std::vector<std::string> initialRouteVec;
    std::vector<std::string> traversedNodes;
    std::string lastValidNode;
    std::string failedNextHop;
    std::vector<std::string> rerouteHistory;

    Packet() : size(0), priority(Priority::LOW), generationTime(0), transmissionTime(0),
               deliveryTime(0), delay(0), status(PacketStatus::CREATED),
               hopIndex(0), rerouteCount(0), hopsRemaining(0) {}

    Packet(std::string id, std::string src, std::string dest, Priority prio, int sz, long long genTime)
        : packetId(id), source(src), destination(dest), size(sz), priority(prio),
          generationTime(genTime), transmissionTime(0), deliveryTime(0), delay(0),
          status(PacketStatus::CREATED), hopIndex(0), rerouteCount(0), hopsRemaining(0) {}
};

#endif // PACKET_H