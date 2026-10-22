#ifndef NETWORK_H
#define NETWORK_H

#include "Node.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <queue>
#include <algorithm>
#include <limits>
#include <stdexcept>

// A link between two nodes in the network graph
struct Link {
    std::string id;        // canonical "u|v"
    std::string source;
    std::string target;
    bool active;
    int weight;            // latency/cost of the link

    Link() : active(true), weight(1) {}
    Link(const std::string& i, const std::string& s, const std::string& t, int w)
        : id(i), source(s), target(t), active(true), weight(w) {}
};

// Graph representation of the network using an adjacency list.
// Routing is performed with Dijkstra's shortest path algorithm.
class Network {
private:
    std::unordered_map<std::string, Node> nodes;
    // Adjacency list: node_id -> list of connected node_ids
    std::unordered_map<std::string, std::vector<std::string>> adjList;
    // Links keyed by canonical id ("u|v" with u < v lexicographically)
    std::unordered_map<std::string, Link> links;

    std::string canonical(const std::string& u, const std::string& v) const {
        return u < v ? u + "|" + v : v + "|" + u;
    }

public:
    void addNode(const Node& node) {
        nodes[node.id] = node;
    }

    void addLink(const std::string& u, const std::string& v, int weight = 1) {
        if (nodes.find(u) != nodes.end() && nodes.find(v) != nodes.end()) {
            if (std::find(adjList[u].begin(), adjList[u].end(), v) == adjList[u].end()) {
                adjList[u].push_back(v);
                adjList[v].push_back(u); // Undirected graph
            }
            std::string key = canonical(u, v);
            if (links.find(key) == links.end()) {
                links[key] = Link(key, u, v, weight);
            }
        }
    }

    Node* getNode(const std::string& id) {
        if (nodes.find(id) != nodes.end()) return &nodes[id];
        return nullptr;
    }

    void setNodeStatus(const std::string& id, NodeStatus status) {
        if (nodes.find(id) != nodes.end()) nodes[id].status = status;
    }

    std::unordered_map<std::string, Node>& getAllNodes() {
        return nodes;
    }

    bool nodeExists(const std::string& id) const {
        return nodes.find(id) != nodes.end();
    }

    // ---- Link management ----

    void setLinkActive(const std::string& u, const std::string& v, bool active) {
        std::string key = canonical(u, v);
        if (links.find(key) != links.end()) links[key].active = active;
    }

    bool isLinkActive(const std::string& u, const std::string& v) {
        std::string key = canonical(u, v);
        auto it = links.find(key);
        if (it == links.end()) return false;
        return it->second.active;
    }

    Link* getLink(const std::string& u, const std::string& v) {
        std::string key = canonical(u, v);
        if (links.find(key) != links.end()) return &links[key];
        return nullptr;
    }

    std::vector<Link> getLinks() const {
        std::vector<Link> result;
        for (auto& kv : links) result.push_back(kv.second);
        return result;
    }

    std::unordered_map<std::string, std::vector<std::string>>& getTopology() {
        return adjList;
    }

    // ---- Dijkstra's Shortest Path Routing Algorithm ----
    // Returns the ordered shortest path from source to destination,
    // skipping inactive nodes and inactive links. Empty if unreachable.
    std::vector<std::string> calculateRoute(const std::string& source, const std::string& destination) {
        std::vector<std::string> empty;
        if (nodes.find(source) == nodes.end() || nodes.find(destination) == nodes.end()) return empty;
        if (nodes[source].status == NodeStatus::INACTIVE) return empty;
        if (nodes[destination].status == NodeStatus::INACTIVE) return empty;

        std::unordered_map<std::string, int> dist;
        std::unordered_map<std::string, std::string> prev;
        std::unordered_set<std::string> visited;

        for (auto& kv : nodes) dist[kv.first] = std::numeric_limits<int>::max();
        dist[source] = 0;

        typedef std::pair<int, std::string> PQItem;
        std::priority_queue<PQItem, std::vector<PQItem>, std::greater<PQItem>> pq;
        pq.push({0, source});

        while (!pq.empty()) {
            auto top = pq.top(); pq.pop();
            int d = top.first;
            std::string u = top.second;

            if (visited.count(u)) continue;
            visited.insert(u);

            if (u == destination) break;

            for (const std::string& v : adjList[u]) {
                auto it = nodes.find(v);
                if (it == nodes.end()) continue;
                if (it->second.status == NodeStatus::INACTIVE) continue;     // inactive node
                if (!isLinkActive(u, v)) continue;                            // inactive link

                Link* link = getLink(u, v);
                int w = link ? link->weight : 1;

                int nd = d + w;
                if (nd < dist[v]) {
                    dist[v] = nd;
                    prev[v] = u;
                    pq.push({nd, v});
                }
            }
        }

        if (dist[destination] == std::numeric_limits<int>::max()) return empty;

        std::vector<std::string> path;
        std::string cur = destination;
        while (cur != "") {
            path.push_back(cur);
            if (prev.find(cur) == prev.end()) break;
            cur = prev[cur];
        }
        std::reverse(path.begin(), path.end());
        if (path.front() != source) return empty;
        return path;
    }
};

#endif // NETWORK_H