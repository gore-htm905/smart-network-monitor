#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include "Network.h"
#include "PriorityQueue.h"
#include "CircularLinkedList.h"
#include "DoublyLinkedList.h"
#include "Stack.h"
#include "Searching.h"
#include "Sorting.h"
#include <chrono>
#include <vector>
#include <string>
#include <random>
#include <sstream>
#include <unordered_map>
#include <algorithm>
#include <iostream>

enum class AlertSeverity { INFO, WARNING, CRITICAL };

struct Alert {
    std::string message;
    AlertSeverity severity;
    long long timestamp;
};

struct PacketTimeline {
    std::string packetId;
    std::vector<std::string> steps;
};

struct NetworkStats {
    int totalGenerated = 0;
    int totalDelivered = 0;
    int totalDropped = 0;
    int totalPending = 0;
    int totalInTransit = 0;
    int totalRerouted = 0;
    long long averageDelay = 0;
    double networkUtilization = 0.0;
    double deliveryRate = 0.0;
    double lossRate = 0.0;
    double routerQueueUtilization = 0.0;
    int activeNodes = 0;
    int failedNodes = 0;
    int endpointCount = 0;
    int routerCount = 0;
    int priorityQueueSize = 0;
    int historySize = 0;
};

struct NetworkConfig {
    int numEndpoints = 10;
    int numRouters = 4;
    int queueCapacity = 20;
    int transitCapacity = 6;
    int maxNodesPerTick = 3;
    bool randomFailures = true;
    int linkWeightMin = 1;
    int linkWeightMax = 5;
    std::string routingAlgorithm = "Dijkstra";
};

class NetworkManager {
private:
    Network network;
    // Priority Queue of packets currently in transit (ordered by priority).
    PriorityQueue priorityQueue;
    // Circular Linked List: round-robin scheduler of nodes.
    CircularLinkedList scheduler;
    // Doubly Linked List: packet history (terminal results).
    DoublyLinkedList packetHistory;
    // Hash Map: packet store indexed by Packet ID for O(1) lookups.
    std::unordered_map<std::string, Packet> packetIndex;
    // Stacks: route tracking per packet.
    std::unordered_map<std::string, Stack> routeStacks;
    // Processing timelines per packet.
    std::unordered_map<std::string, PacketTimeline> timelines;

    std::vector<Alert> alerts;

    NetworkConfig config;
    int totalPacketsGenerated = 0;

public:
    enum class SimulationState { STOPPED, RUNNING, PAUSED };

private:
    SimulationState simState = SimulationState::STOPPED;
    bool simulationRunning = false;
    bool simulationPaused = false;
    int simulationSpeed = 1;
    long long lastTickTime = 0;
    long long tickCount = 0;

    // Random failure scheduling
    int failureCooldown = 0;
    std::vector<std::pair<std::string, long long>> scheduledNodeRestores;
    std::vector<std::pair<std::string, long long>> scheduledLinkRestores;

    // Queue congestion warning flags per node (avoid alert spam)
    std::unordered_map<std::string, bool> nodeWarned;

    std::mt19937 rng;

    long long getCurrentTimeMs() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }

    void addAlert(const std::string& msg, AlertSeverity sev) {
        alerts.push_back({msg, sev, getCurrentTimeMs()});
        if (alerts.size() > 200) {
            alerts.erase(alerts.begin());
        }
    }

    void addTimeline(const std::string& id, const std::string& step) {
        timelines[id].packetId = id;
        timelines[id].steps.push_back(step);
    }

    std::string priorityToString(Priority p) {
        switch (p) {
            case Priority::HIGH: return "HIGH";
            case Priority::MEDIUM: return "MEDIUM";
            case Priority::LOW: return "LOW";
        }
        return "LOW";
    }

    std::string statusToString(PacketStatus s) {
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

    std::string nodeStatusToString(NodeStatus s) {
        switch (s) {
            case NodeStatus::ACTIVE: return "Active";
            case NodeStatus::INACTIVE: return "Inactive";
            case NodeStatus::HIGH_TRAFFIC: return "High Traffic";
            case NodeStatus::CONGESTED: return "Congested";
        }
        return "Unknown";
    }

    std::string joinRoute(const std::vector<std::string>& route) {
        std::string s;
        for (size_t i = 0; i < route.size(); ++i) {
            s += route[i];
            if (i != route.size() - 1) s += " -> ";
        }
        return s;
    }

    void indexPacket(const Packet& p) {
        packetIndex[p.packetId] = p;
    }

    void updateStackForRoute(const std::string& packetId, const std::vector<std::string>& route) {
        Stack s;
        for (auto it = route.rbegin(); it != route.rend(); ++it) {
            s.push(*it);
        }
        routeStacks[packetId] = s;
    }

    bool handleInTransitArrival(Packet& pk, std::vector<Packet>& delayed) {
        // Case C: If the packet's current node went offline while it was in transit,
        // the packet is trapped there and cannot be forwarded. Drop it.
        Node* curNode = network.getNode(pk.currentNode);
        if (!curNode || curNode->status == NodeStatus::INACTIVE) {
            dropPacket(pk, "Current node " + pk.currentNode + " went offline while packet was in transit");
            return false;
        }

        int nextIdx = pk.hopIndex + 1;
        if (nextIdx >= (int)pk.routeVec.size()) {
            dropPacket(pk, "Route ended before destination");
            return false;
        }

        std::string next = pk.routeVec[nextIdx];
        Node* target = network.getNode(next);
        bool nextBlocked = !target || target->status == NodeStatus::INACTIVE ||
                           !network.isLinkActive(pk.currentNode, next);

        if (nextBlocked) {
            // Case A: Intermediate router failed before the packet reaches it.
            // Reroute from the packet's actual current position.
            pk.failedNextHop = next;
            pk.lastValidNode = pk.currentNode;
            if (reroute(pk)) {
                pk.status = PacketStatus::REROUTED;
                // pk.currentNode stays at the reroute origin — no teleporting
                delayed.push_back(pk);
                indexPacket(pk);
            }
            // else: dropPacket already called inside reroute()
            return false;
        }

        pk.hopIndex = nextIdx;
        pk.currentNode = pk.routeVec[nextIdx];
        pk.lastValidNode = pk.currentNode;
        pk.traversedNodes.push_back(pk.currentNode);

        if (pk.currentNode == pk.destination) {
            // Case D: Verify destination is still online before delivering
            Node* destNode = network.getNode(pk.destination);
            if (!destNode || destNode->status == NodeStatus::INACTIVE) {
                pk.failedNextHop = pk.destination;
                dropPacket(pk, "Destination " + pk.destination + " went offline upon arrival");
                return false;
            }
            deliver(pk);
            return true;
        }

        pk.status = PacketStatus::QUEUED;
        if (!enqueueToNode(pk, pk.currentNode)) {
            dropPacket(pk, "Queue overflow at " + pk.currentNode);
            return false;
        }
        return true;
    }

    // Recompute the route for a packet from its current node.
    // Returns false and drops the packet if no alternate route exists.
    bool reroute(Packet& pk) {
        // Always reroute from the packet's actual current position
        std::string from = pk.currentNode;
        if (from.empty()) {
            from = pk.routeVec.empty() ? pk.source : pk.routeVec[pk.hopIndex];
        }

        // Case D: destination offline — cannot deliver
        Node* destNode = network.getNode(pk.destination);
        if (!destNode || destNode->status == NodeStatus::INACTIVE) {
            dropPacket(pk, "Destination " + pk.destination + " is offline");
            return false;
        }

        // Case C: current node offline — packet is trapped, drop it
        Node* curNode = network.getNode(from);
        if (!curNode || curNode->status == NodeStatus::INACTIVE) {
            dropPacket(pk, "Current node " + from + " is offline — packet cannot be rerouted");
            return false;
        }

        std::vector<std::string> newRoute = network.calculateRoute(from, pk.destination);
        if (newRoute.empty()) {
            dropPacket(pk, "No alternate route from " + from + " to " + pk.destination);
            return false;
        }
        pk.routeVec = newRoute;
        pk.route = joinRoute(newRoute);
        pk.hopIndex = 0;
        pk.currentNode = from;  // Preserve actual position — no teleporting
        pk.rerouteCount++;
        pk.hopsRemaining = (int)newRoute.size() - 1;
        std::string rh = "Rerouted at " + from + " -> new path: " + pk.route;
        pk.rerouteHistory.push_back(rh);
        addTimeline(pk.packetId, rh);
        addAlert("Packet " + pk.packetId + " rerouted: " + pk.route, AlertSeverity::WARNING);
        updateStackForRoute(pk.packetId, newRoute);
        return true;
    }

    bool enqueueToNode(Packet& pk, const std::string& nodeId) {
        Node* n = network.getNode(nodeId);
        if (!n) return false;
        if (n->status == NodeStatus::INACTIVE) return false;
        if (n->queue.isFull()) return false;
        n->queue.enqueue(pk);
        n->updateStatus();
        checkCongestion(*n);
        return true;
    }

    void deliver(Packet& pk) {
        pk.status = PacketStatus::DELIVERED;
        pk.deliveryTime = getCurrentTimeMs();
        pk.delay = pk.deliveryTime - pk.generationTime;
        pk.hopsRemaining = 0;
        Node* destNode = network.getNode(pk.destination);
        if (destNode) destNode->processedPackets++;
        packetHistory.append(pk);
        indexPacket(pk);
        addTimeline(pk.packetId, "DELIVERED to " + pk.destination + " (delay " + std::to_string(pk.delay) + "ms)");
        addAlert("Packet " + pk.packetId + " delivered to " + pk.destination + " [route: " + pk.route + "]", AlertSeverity::INFO);
        if (pk.delay > 8000) {
            addAlert("High network delay detected: packet " + pk.packetId + " took " + std::to_string(pk.delay) + "ms", AlertSeverity::WARNING);
        }
    }

    void dropPacket(Packet& pk, const std::string& reason) {
        pk.status = PacketStatus::DROPPED;
        pk.dropReason = reason;
        pk.hopsRemaining = 0;
        packetHistory.append(pk);
        indexPacket(pk);
        addTimeline(pk.packetId, "DROPPED - " + reason);
        addAlert("Packet " + pk.packetId + " dropped: " + reason, AlertSeverity::CRITICAL);
    }

    void checkCongestion(Node& n) {
        double util = (double)n.queue.size() / n.queue.getCapacity() * 100.0;
        bool& warned = nodeWarned[n.id];
        if (util >= 95.0) {
            if (warned) return;
            warned = true;
            addAlert("CRITICAL: " + n.id + " queue is overflowing (" + std::to_string((int)util) + "%)", AlertSeverity::CRITICAL);
        } else if (util >= 85.0) {
            if (warned) return;
            warned = true;
            addAlert(n.id + " queue nearly full (" + std::to_string((int)util) + "%)", AlertSeverity::WARNING);
        } else {
            warned = false;
        }
    }

    // ---- Random failure injection ----

    void maybeInjectFailure() {
        if (!config.randomFailures || !simulationRunning || simulationPaused) return;

        long long now = getCurrentTimeMs();

        // Restore due items first.
        for (size_t i = 0; i < scheduledNodeRestores.size(); ) {
            if (now >= scheduledNodeRestores[i].second) {
                setNodeActive(scheduledNodeRestores[i].first, true);
                scheduledNodeRestores.erase(scheduledNodeRestores.begin() + i);
            } else {
                i++;
            }
        }
        for (size_t i = 0; i < scheduledLinkRestores.size(); ) {
            if (now >= scheduledLinkRestores[i].second) {
                std::string& linkId = scheduledLinkRestores[i].first;
                size_t sep = linkId.find('|');
                if (sep != std::string::npos) {
                    setLinkActive(linkId.substr(0, sep), linkId.substr(sep + 1), true);
                }
                scheduledLinkRestores.erase(scheduledLinkRestores.begin() + i);
            } else {
                i++;
            }
        }

        if (failureCooldown > 0) { failureCooldown--; return; }
        failureCooldown = 20 + (int)(rng() % 30);

        // Prefer failing a router (60%) otherwise a link.
        std::vector<std::string> routers;
        for (auto& kv : network.getAllNodes()) {
            if (kv.second.isRouter() && kv.second.status != NodeStatus::INACTIVE) {
                routers.push_back(kv.first);
            }
        }

        if (!routers.empty() && (rng() % 100) < 60) {
            std::string id = routers[rng() % routers.size()];
            if (!id.empty()) {
                setNodeActive(id, false);
                long long restoreIn = 10000 + (rng() % 15000);
                scheduledNodeRestores.push_back({id, now + restoreIn});
            }
            return;
        }

        // Fail a random active link.
        std::vector<Link> activeLinks;
        for (auto& l : network.getLinks()) {
            if (l.active) activeLinks.push_back(l);
        }
        if (!activeLinks.empty()) {
            Link pick = activeLinks[rng() % activeLinks.size()];
            setLinkActive(pick.source, pick.target, false);
            long long restoreIn = 8000 + (rng() % 12000);
            scheduledLinkRestores.push_back({pick.id, now + restoreIn});
        }
    }

    // ---- Node / link failures ----

    // Prepare a queued packet for departure. Returns:
    //   "ok"      -> ready to move to the next hop
    //   "deliver" -> reached destination, deliver now
    //   "drop"    -> must be dropped (dropPacket already called)
    std::string prepareForDeparture(Packet& pk, const std::string& nodeId) {
        // Case D: Verify destination is still online
        Node* destCheck = network.getNode(pk.destination);
        if (!destCheck || destCheck->status == NodeStatus::INACTIVE) {
            dropPacket(pk, "Destination " + pk.destination + " is offline");
            return "drop";
        }

        if (pk.routeVec.empty()) {
            std::vector<std::string> r = network.calculateRoute(nodeId, pk.destination);
            if (r.empty()) { dropPacket(pk, "No route to " + pk.destination); return "drop"; }
            pk.routeVec = r;
            pk.route = joinRoute(r);
            pk.hopIndex = 0;
            pk.currentNode = nodeId;
            pk.hopsRemaining = (int)r.size() - 1;
            updateStackForRoute(pk.packetId, r);
        }

        int nextIdx = pk.hopIndex + 1;
        if (nextIdx >= (int)pk.routeVec.size()) {
            return "deliver";
        }

        std::string next = pk.routeVec[nextIdx];
        Node* nn = network.getNode(next);
        bool blocked = !nn || nn->status == NodeStatus::INACTIVE ||
                       !network.isLinkActive(pk.routeVec[pk.hopIndex], next);

        if (blocked) {
            std::vector<std::string> nr = network.calculateRoute(nodeId, pk.destination);
            if (nr.empty()) {
                dropPacket(pk, "No alternate route available from " + nodeId);
                return "drop";
            }
            pk.routeVec = nr;
            pk.route = joinRoute(nr);
            pk.hopIndex = 0;
            pk.currentNode = nodeId;
            pk.rerouteCount++;
            pk.hopsRemaining = (int)nr.size() - 1;
            addTimeline(pk.packetId, "REROUTED - new path: " + pk.route);
            addAlert("Packet " + pk.packetId + " rerouted: " + pk.route, AlertSeverity::WARNING);
            updateStackForRoute(pk.packetId, nr);
        }
        return "ok";
    }

public:
    NetworkManager() : rng((unsigned)std::chrono::system_clock::now().time_since_epoch().count()) {}

    // ========== NETWORK CREATION ==========

    void generateNetwork(int endpoints, int routers) {
        generateNetwork(endpoints, routers, config.queueCapacity, config.routingAlgorithm);
    }

    void generateNetwork(int endpoints, int routers, int queueCapacity, const std::string& algorithm) {
        config.numEndpoints = endpoints;
        config.numRouters = routers;
        config.queueCapacity = queueCapacity;
        config.routingAlgorithm = algorithm;

        // Reset all simulation state.
        simState = SimulationState::STOPPED;
        simulationRunning = false;
        simulationPaused = false;
        tickCount = 0;
        failureCooldown = 0;

        network = Network();
        scheduler = CircularLinkedList();
        packetHistory = DoublyLinkedList();
        priorityQueue = PriorityQueue();
        packetIndex.clear();
        routeStacks.clear();
        timelines.clear();
        alerts.clear();
        nodeWarned.clear();
        scheduledNodeRestores.clear();
        scheduledLinkRestores.clear();
        totalPacketsGenerated = 0;

        std::vector<std::string> routerIds;
        std::vector<std::string> endpointIds;

        for (int i = 1; i <= endpoints; ++i) {
            std::string id = "N" + std::to_string(i);
            endpointIds.push_back(id);
            network.addNode(Node(id, "Node " + std::to_string(i), NodeType::ENDPOINT, config.queueCapacity));
        }
        for (int i = 1; i <= routers; ++i) {
            std::string id = "R" + std::to_string(i);
            routerIds.push_back(id);
            network.addNode(Node(id, "Router " + std::to_string(i), NodeType::ROUTER, config.queueCapacity));
        }

        // Router ring to guarantee full connectivity.
        for (size_t i = 0; i < routerIds.size(); ++i) {
            network.addLink(routerIds[i], routerIds[(i + 1) % routerIds.size()],
                            config.linkWeightMin + (int)(rng() % (config.linkWeightMax - config.linkWeightMin + 1)));
        }
        // Extra router-router links for redundant paths (wraparound "mesh").
        if (routers >= 4) {
            size_t span = (routerIds.size()) / 2;
            for (size_t i = 0; i < routerIds.size(); ++i) {
                size_t j = (i + span) % routerIds.size();
                if (i == j || i == (j + 1) % routerIds.size()) continue;
                if (network.isLinkActive(routerIds[i], routerIds[j])) continue;
                network.addLink(routerIds[i], routerIds[j],
                                config.linkWeightMin + (int)(rng() % (config.linkWeightMax - config.linkWeightMin + 1)));
            }
        }

        // Each endpoint attaches to 1-2 nearby routers for redundancy.
        for (size_t i = 0; i < endpointIds.size(); ++i) {
            if (routerIds.empty()) break;
            size_t r1 = i % routerIds.size();
            network.addLink(endpointIds[i], routerIds[r1],
                            config.linkWeightMin + (int)(rng() % (config.linkWeightMax - config.linkWeightMin + 1)));
            if (routers > 1) {
                size_t r2 = (r1 + 1) % routerIds.size();
                if (!network.isLinkActive(endpointIds[i], routerIds[r2])) {
                    network.addLink(endpointIds[i], routerIds[r2],
                                    config.linkWeightMin + (int)(rng() % (config.linkWeightMax - config.linkWeightMin + 1)));
                }
            }
        }

        // Register all nodes with the round-robin scheduler.
        for (auto& id : endpointIds) scheduler.addNode(id);
        for (auto& id : routerIds) scheduler.addNode(id);

        lastTickTime = getCurrentTimeMs();
        addAlert("Network generated: " + std::to_string(endpoints) + " endpoints, " +
                 std::to_string(routers) + " routers", AlertSeverity::INFO);
    }

    const NetworkConfig& getConfig() const { return config; }

    // ========== SIMULATION CONTROLS ==========

    void startSimulation() {
        if (simState == SimulationState::RUNNING) return;
        simState = SimulationState::RUNNING;
        simulationRunning = true;
        simulationPaused = false;
        lastTickTime = getCurrentTimeMs();
        addAlert("Simulation started", AlertSeverity::INFO);
    }

    void pauseSimulation() {
        if (simState == SimulationState::RUNNING) {
            simState = SimulationState::PAUSED;
            simulationPaused = true;
            addAlert("Simulation paused", AlertSeverity::INFO);
        }
    }

    void resumeSimulation() {
        if (simState == SimulationState::PAUSED) {
            simState = SimulationState::RUNNING;
            simulationPaused = false;
            lastTickTime = getCurrentTimeMs();
            addAlert("Simulation resumed", AlertSeverity::INFO);
        }
    }

    void resetSimulation() {
        // Regenerate the configured network back to its initial state.
        generateNetwork(config.numEndpoints, config.numRouters, config.queueCapacity, config.routingAlgorithm);
        simState = SimulationState::STOPPED;
        simulationRunning = false;
        simulationPaused = false;
        tickCount = 0;
        addAlert("Simulation reset", AlertSeverity::INFO);
    }

    void setSimulationSpeed(int speed) {
        if (speed < 1) speed = 1;
        if (speed > 5) speed = 5;
        simulationSpeed = speed;
    }

    SimulationState getSimulationState() const { return simState; }
    bool isRunning() const { return simState == SimulationState::RUNNING; }
    bool isPaused() const { return simState == SimulationState::PAUSED; }
    bool isStopped() const { return simState == SimulationState::STOPPED; }
    int getSimulationSpeed() const { return simulationSpeed; }
    long long getTickCount() const { return tickCount; }

    // Advance the simulation by the number of ticks that have elapsed.
    // Called periodically by the server event loop and by API handlers
    // so the simulation progresses based on wall-clock time.
    void updateSimulation() {
        if (!simulationRunning || simulationPaused) {
            lastTickTime = getCurrentTimeMs();
            return;
        }
        long long now = getCurrentTimeMs();
        long long elapsed = now - lastTickTime;
        if (elapsed <= 0) return;

        long long interval = 500 / simulationSpeed;
        long long ticks = elapsed / interval;
        if (ticks <= 0) return;
        if (ticks > 60) ticks = 60; // avoid bursts after long idle

        for (long long i = 0; i < ticks; ++i) {
            advanceOneTick();
            maybeInjectFailure();
        }
        lastTickTime = now;
    }

    void advanceOneTick() {
        if (!simulationRunning || simulationPaused) return;
        tickCount++;

        // ---- 1. Arrivals: packets in transit arrive at their next hop. ----
        std::vector<Packet> delayed;
        int arrived = 0;
        while (!priorityQueue.isEmpty() && arrived < config.transitCapacity) {
            Packet pk = priorityQueue.dequeue();
            arrived++;
            handleInTransitArrival(pk, delayed);
        }
        // Packets that were rerouted mid-flight arrive on the next tick.
        for (auto& d : delayed) priorityQueue.enqueue(d);

        // ---- 2. Departures: nodes forward packets (round-robin order). ----
        std::vector<std::pair<std::string, bool>> nodeOrder = scheduler.getNodes();
        if (nodeOrder.empty()) return;

        std::string scheduledNode = scheduler.getNextActiveNode();
        int startIdx = 0;
        for (size_t i = 0; i < nodeOrder.size(); ++i) {
            if (nodeOrder[i].first == scheduledNode) { startIdx = (int)i; break; }
        }

        int processedNodes = 0;
        for (size_t step = 0; step < nodeOrder.size() && processedNodes < (size_t)config.maxNodesPerTick; ++step) {
            size_t idx = (startIdx + step) % nodeOrder.size();
            Node* node = network.getNode(nodeOrder[idx].first);
            if (!node || node->status == NodeStatus::INACTIVE || node->queue.isEmpty()) continue;

            std::string nodeId = node->id;
            Packet front = node->queue.peek();
            std::string outcome = prepareForDeparture(front, nodeId);

            if (outcome == "deliver") {
                node->queue.dequeue();
                node->updateStatus();
                deliver(front);
                continue;
            }
            if (outcome == "drop") {
                node->queue.dequeue();
                node->updateStatus();
                continue;
            }

            // "ok" (possibly rerouted): move the packet into transit.
            node->queue.dequeue();
            node->updateStatus();

            front.status = PacketStatus::IN_TRANSIT;
            front.currentNode = front.routeVec[front.hopIndex];
            front.hopsRemaining = (int)front.routeVec.size() - front.hopIndex - 1;

            std::string nextNode = (front.hopIndex + 1 < (int)front.routeVec.size())
                                       ? front.routeVec[front.hopIndex + 1] : front.destination;
            addTimeline(front.packetId, "IN TRANSIT from " + front.currentNode + " to " + nextNode);
            priorityQueue.enqueue(front);
            indexPacket(front);
            processedNodes++;
        }
    }

    // ========== PACKET GENERATION ==========

    std::string generatePacket(const std::string& src, const std::string& dest, int priInt, int size) {
        totalPacketsGenerated++;
        std::string id = "P" + std::to_string(100 + totalPacketsGenerated);

        Priority prio = static_cast<Priority>(priInt >= 2 ? 2 : (priInt <= 0 ? 0 : 1));
        Packet p(id, src, dest, prio, size, getCurrentTimeMs());
        p.currentNode = src;

        addTimeline(id, "CREATED by " + src);

        Node* srcNode = network.getNode(src);
        if (!srcNode || srcNode->status == NodeStatus::INACTIVE) {
            dropPacket(p, "Source " + src + " is offline");
            return id;
        }
        Node* dstNode = network.getNode(dest);
        if (dstNode && dstNode->status == NodeStatus::INACTIVE) {
            dropPacket(p, "Destination " + dest + " is offline");
            return id;
        }
        if (src == dest) {
            dropPacket(p, "Source and destination are the same node");
            return id;
        }

        std::vector<std::string> route = network.calculateRoute(src, dest);
        if (route.empty()) {
            dropPacket(p, "No route from " + src + " to " + dest);
            return id;
        }

        p.routeVec = route;
        p.route = joinRoute(route);
        p.initialRoute = p.route;
        p.initialRouteVec = route;
        p.traversedNodes = {src};
        p.lastValidNode = src;
        p.hopIndex = 0;
        p.hopsRemaining = (int)route.size() - 1;
        p.status = PacketStatus::QUEUED;

        if (!enqueueToNode(p, src)) {
            dropPacket(p, "Source queue full at " + src);
            return id;
        }

        updateStackForRoute(id, route);
        indexPacket(p);
        addTimeline(id, "QUEUED at " + src);
        addTimeline(id, "Route computed: " + p.route);
        return id;
    }

    std::string autoGeneratePacket() {
        std::vector<std::string> endpoints;
        for (auto& kv : network.getAllNodes()) {
            if (!kv.second.isRouter()) endpoints.push_back(kv.first);
        }
        if (endpoints.size() < 2) return "";
        std::string src = endpoints[rng() % endpoints.size()];
        std::string dest;
        do { dest = endpoints[rng() % endpoints.size()]; } while (dest == src);

        std::uniform_int_distribution<int> prioDist(0, 2);
        std::uniform_int_distribution<int> sizeDist(64, 1500);
        return generatePacket(src, dest, prioDist(rng), sizeDist(rng));
    }

    std::vector<std::string> bulkGenerate(int count) {
        std::vector<std::string> ids;
        for (int i = 0; i < count; ++i) ids.push_back(autoGeneratePacket());
        return ids;
    }

    std::vector<std::string> getEndpointIds() const {
        std::vector<std::string> ids;
        // Preserve insertion order by scanning configuration counts.
        NetworkManager* self = const_cast<NetworkManager*>(this);
        for (auto& kv : self->network.getAllNodes()) {
            if (!kv.second.isRouter()) ids.push_back(kv.first);
        }
        std::sort(ids.begin(), ids.end(), [](const std::string& a, const std::string& b) {
            auto num = [](const std::string& s) {
                int n = 0; for (char c : s) if (isdigit(c)) n = n * 10 + (c - '0'); return n;
            };
            return num(a) < num(b);
        });
        return ids;
    }

    // ========== FAILURES ==========

    void setNodeActive(const std::string& nodeId, bool active) {
        network.setNodeStatus(nodeId, active ? NodeStatus::ACTIVE : NodeStatus::INACTIVE);
        scheduler.setNodeStatus(nodeId, active);
        Node* n = network.getNode(nodeId);
        if (n) n->updateStatus();

        if (!active) {
            addAlert("Node " + nodeId + " is OFFLINE", AlertSeverity::CRITICAL);

            // Case B: Drop all packets queued at this now-offline node.
            if (n && !n->queue.isEmpty()) {
                auto stranded = n->queue.drain();
                n->updateStatus();
                for (auto& pk : stranded) {
                    dropPacket(pk, "Node " + nodeId + " went offline while packet was queued");
                }
            }

            // Re-validate in-transit packets that reference this offline node.
            revalidateInTransitPackets();
        } else {
            addAlert("Node " + nodeId + " restored", AlertSeverity::INFO);
        }
    }

    void setLinkActive(const std::string& src, const std::string& tgt, bool active) {
        network.setLinkActive(src, tgt, active);
        if (!active) {
            addAlert("Link " + src + " - " + tgt + " failed", AlertSeverity::CRITICAL);
            // Re-validate in-transit packets that may use this link.
            revalidateInTransitPackets();
        } else {
            addAlert("Link " + src + " - " + tgt + " restored", AlertSeverity::INFO);
        }
    }

private:
    // Drain the priority queue (in-transit packets) and re-validate each one
    // against current network state. Packets whose next hop is blocked are
    // rerouted; packets whose current node is offline are dropped.
    void revalidateInTransitPackets() {
        auto allTransit = priorityQueue.drainAll();
        for (auto& pk : allTransit) {
            // Skip already-terminal packets
            if (pk.status == PacketStatus::DELIVERED || pk.status == PacketStatus::DROPPED) {
                continue;
            }

            // Case C: current node offline — packet is trapped
            Node* cur = network.getNode(pk.currentNode);
            if (!cur || cur->status == NodeStatus::INACTIVE) {
                dropPacket(pk, "Node " + pk.currentNode + " went offline — in-transit packet dropped");
                continue;
            }

            // Check if next hop is still valid
            int nextIdx = pk.hopIndex + 1;
            bool needsReroute = false;
            if (nextIdx < (int)pk.routeVec.size()) {
                std::string next = pk.routeVec[nextIdx];
                Node* nn = network.getNode(next);
                needsReroute = !nn || nn->status == NodeStatus::INACTIVE ||
                               !network.isLinkActive(pk.currentNode, next);
            } else {
                // Route ended or points to destination
                Node* dest = network.getNode(pk.destination);
                needsReroute = !dest || dest->status == NodeStatus::INACTIVE;
            }

            if (needsReroute) {
                if (reroute(pk)) {
                    pk.status = PacketStatus::REROUTED;
                    priorityQueue.enqueue(pk);
                    indexPacket(pk);
                }
                // else: dropPacket already called inside reroute()
            } else {
                // Still valid — re-enqueue
                priorityQueue.enqueue(pk);
            }
        }
    }

public:

    // ========== QUERYING ==========

    std::vector<Packet> getAllPackets() const {
        std::vector<Packet> result;
        for (auto& kv : packetIndex) result.push_back(kv.second);
        return result;
    }

    std::vector<Packet> getActivePackets() const {
        std::vector<Packet> result;
        for (auto& kv : packetIndex) {
            PacketStatus s = kv.second.status;
            if (s == PacketStatus::CREATED || s == PacketStatus::QUEUED ||
                s == PacketStatus::IN_TRANSIT || s == PacketStatus::REROUTED) {
                result.push_back(kv.second);
            }
        }
        return result;
    }

    std::vector<Packet> searchPackets(const std::string& query) const {
        return Searching::searchByQuery(getAllPackets(), query);
    }

    std::vector<Packet> sortPackets(const std::string& by, const std::string& order) {
        std::vector<Packet> all = getAllPackets();

        Sorting::SortField sf = Sorting::SortField::ID;
        if (by == "priority") sf = Sorting::SortField::PRIORITY;
        else if (by == "delay") sf = Sorting::SortField::DELAY;
        else if (by == "size") sf = Sorting::SortField::SIZE;
        else if (by == "timestamp") sf = Sorting::SortField::TIMESTAMP;
        else if (by == "status") sf = Sorting::SortField::STATUS;
        else if (by == "source") sf = Sorting::SortField::SOURCE;
        else if (by == "destination") sf = Sorting::SortField::DESTINATION;

        Sorting::SortOrder so = (order == "desc") ? Sorting::SortOrder::DESCENDING : Sorting::SortOrder::ASCENDING;
        Sorting::sortPackets(all, sf, so);
        return all;
    }

    std::vector<std::string> previewRoute(const std::string& src, const std::string& dest) {
        return network.calculateRoute(src, dest);
    }

    NetworkStats computeStats() const {
        NetworkStats s;
        s.totalGenerated = totalPacketsGenerated;
        s.totalDelivered = 0;
        s.totalDropped = 0;
        s.totalRerouted = 0;
        long long totalDelaySum = 0;
        long long deliveredCount = 0;

        int queueUsage = 0, queueCapacity = 0, routerQueueUsage = 0, routerQueueCapacity = 0;

        NetworkManager* self = const_cast<NetworkManager*>(this);
        for (auto& kv : packetIndex) {
            const Packet& p = kv.second;
            if (p.status == PacketStatus::DELIVERED) {
                s.totalDelivered++;
                totalDelaySum += p.delay;
                deliveredCount++;
            } else if (p.status == PacketStatus::DROPPED) {
                s.totalDropped++;
            } else if (p.status == PacketStatus::IN_TRANSIT) {
                s.totalInTransit++;
            }
            if (p.rerouteCount > 0) s.totalRerouted++;
        }
        s.totalPending = (int)packetIndex.size() - s.totalDelivered - s.totalDropped;
        s.averageDelay = deliveredCount > 0 ? totalDelaySum / deliveredCount : 0;

        int activeNodes = 0, failedNodes = 0;
        for (auto& kv : self->network.getAllNodes()) {
            Node& n = kv.second;
            queueUsage += n.queue.size();
            queueCapacity += n.queue.getCapacity();
            if (n.isRouter()) {
                routerQueueUsage += n.queue.size();
                routerQueueCapacity += n.queue.getCapacity();
            }
            if (n.status == NodeStatus::INACTIVE) failedNodes++;
            else activeNodes++;
        }
        s.activeNodes = activeNodes;
        s.failedNodes = failedNodes;
        s.networkUtilization = queueCapacity > 0 ? (double)queueUsage / queueCapacity * 100.0 : 0.0;
        s.routerQueueUtilization = routerQueueCapacity > 0 ? (double)routerQueueUsage / routerQueueCapacity * 100.0 : 0.0;
        s.deliveryRate = s.totalGenerated > 0 ? (double)s.totalDelivered / s.totalGenerated * 100.0 : 0.0;
        s.lossRate = s.totalGenerated > 0 ? (double)s.totalDropped / s.totalGenerated * 100.0 : 0.0;
        s.totalInTransit = priorityQueue.size();
        s.priorityQueueSize = priorityQueue.size();
        s.historySize = packetHistory.size();
        s.endpointCount = config.numEndpoints;
        s.routerCount = config.numRouters;
        s.totalPending = s.totalPending < 0 ? 0 : s.totalPending;
        return s;
    }

    // ========== GETTERS FOR API ==========

    Network& getNetwork() { return network; }
    PriorityQueue& getPriorityQueue() { return priorityQueue; }
    CircularLinkedList& getScheduler() { return scheduler; }
    DoublyLinkedList& getHistory() { return packetHistory; }
    std::vector<Alert>& getAlerts() { return alerts; }
    std::unordered_map<std::string, Stack>& getRouteStacks() { return routeStacks; }
    std::unordered_map<std::string, PacketTimeline>& getTimelines() { return timelines; }
    std::unordered_map<std::string, Packet>& getPacketIndex() { return packetIndex; }
    const std::unordered_map<std::string, Packet>& getPacketIndex() const { return packetIndex; }

    Packet* findPacket(const std::string& id) {
        std::string trimmed = id;
        while (!trimmed.empty() && (trimmed.front() == ' ' || trimmed.front() == '\t')) trimmed.erase(trimmed.begin());
        while (!trimmed.empty() && (trimmed.back() == ' ' || trimmed.back() == '\t' || trimmed.back() == '\r')) trimmed.pop_back();
        if (trimmed.empty()) return nullptr;

        auto it = packetIndex.find(trimmed);
        if (it != packetIndex.end()) return &it->second;

        std::string upper = trimmed;
        std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
        for (auto& kv : packetIndex) {
            std::string keyUpper = kv.first;
            std::transform(keyUpper.begin(), keyUpper.end(), keyUpper.begin(), ::toupper);
            if (keyUpper == upper) return &kv.second;
        }
        return nullptr;
    }

    int getTotalGenerated() const { return totalPacketsGenerated; }

    std::string getPriorityStr(Priority p) { return priorityToString(p); }
    std::string getStatusStr(PacketStatus s) { return statusToString(s); }
    std::string getNodeStatusStr(NodeStatus s) { return nodeStatusToString(s); }
};

#endif // NETWORKMANAGER_H