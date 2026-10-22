#ifndef NODE_H
#define NODE_H

#include <string>
#include "Queue.h"

enum class NodeStatus {
    ACTIVE,
    INACTIVE,
    HIGH_TRAFFIC,
    CONGESTED
};

enum class NodeType {
    ENDPOINT, // End device / Node generating packets
    ROUTER    // Intermediate router
};

class Node {
public:
    std::string id;
    std::string name;
    NodeType type;
    NodeStatus status;
    Queue queue; // Use our custom Queue for waiting packets
    
    int processedPackets;
    int droppedPackets;

    Node() : type(NodeType::ENDPOINT), status(NodeStatus::ACTIVE), processedPackets(0), droppedPackets(0) {}

    Node(std::string nodeId, std::string nodeName, NodeType t, int queueCapacity = 20)
        : id(nodeId), name(nodeName), type(t), status(NodeStatus::ACTIVE), queue(queueCapacity), processedPackets(0), droppedPackets(0) {}

    bool isRouter() const { return type == NodeType::ROUTER; }

    bool isCongested() const {
        return (double)queue.size() / queue.getCapacity() >= 0.85; // 85% utilization
    }

    void updateStatus() {
        if (status == NodeStatus::INACTIVE) return;
        
        double utilization = (double)queue.size() / queue.getCapacity();
        if (utilization >= 0.95) {
            status = NodeStatus::CONGESTED;
        } else if (utilization >= 0.75) {
            status = NodeStatus::HIGH_TRAFFIC;
        } else {
            status = NodeStatus::ACTIVE;
        }
    }
    
    // Attempt to enqueue a packet. Returns false if queue is full.
    bool receivePacket(const Packet& p) {
        if (status == NodeStatus::INACTIVE) return false;
        
        bool success = queue.enqueue(p);
        if (!success) {
            droppedPackets++;
        }
        updateStatus();
        return success;
    }
};

#endif // NODE_H
