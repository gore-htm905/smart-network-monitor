#include "ApiServer.h"
#include <iostream>

// ========== JSON SERIALIZATION ==========

Json ApiServer::packetToJson(const Packet& p) {
    Json obj = Json::object();
    obj.set("id", p.packetId);
    obj.set("packetId", p.packetId);
    obj.set("source", p.source);
    obj.set("destination", p.destination);
    obj.set("size", p.size);
    obj.set("priority", manager.getPriorityStr(p.priority));
    obj.set("priorityInt", (int)p.priority);
    obj.set("status", manager.getStatusStr(p.status));
    obj.set("statusInt", (int)p.status);
    obj.set("delay", (long long)p.delay);
    obj.set("route", p.route);
    obj.set("currentNode", p.currentNode);
    obj.set("hopIndex", p.hopIndex);
    obj.set("hopsRemaining", p.hopsRemaining);
    obj.set("rerouteCount", p.rerouteCount);
    obj.set("dropReason", p.dropReason);
    obj.set("generationTime", p.generationTime);
    obj.set("timestamp", p.generationTime);
    obj.set("transmissionTime", p.transmissionTime);
    obj.set("deliveryTime", p.deliveryTime);

    obj.set("initialRoute", p.initialRoute.empty() ? p.route : p.initialRoute);
    Json initRouteArr = Json::array();
    const auto& initVec = p.initialRouteVec.empty() ? p.routeVec : p.initialRouteVec;
    for (size_t i = 0; i < initVec.size(); ++i) {
        initRouteArr.push(Json::string(initVec[i]));
    }
    obj.set("initialRouteArr", initRouteArr);

    Json routeArr = Json::array();
    for (size_t i = 0; i < p.routeVec.size(); ++i) {
        routeArr.push(Json::string(p.routeVec[i]));
    }
    obj.set("routeArr", routeArr);

    Json traversedArr = Json::array();
    for (size_t i = 0; i < p.traversedNodes.size(); ++i) {
        traversedArr.push(Json::string(p.traversedNodes[i]));
    }
    obj.set("traversedNodes", traversedArr);

    obj.set("lastValidNode", p.lastValidNode.empty() ? p.currentNode : p.lastValidNode);
    obj.set("failedNextHop", p.failedNextHop);

    Json reroutesArr = Json::array();
    for (size_t i = 0; i < p.rerouteHistory.size(); ++i) {
        reroutesArr.push(Json::string(p.rerouteHistory[i]));
    }
    obj.set("rerouteHistory", reroutesArr);
    return obj;
}

Json ApiServer::nodeToJson(Node& n) {
    Json obj = Json::object();
    obj.set("id", n.id);
    obj.set("name", n.name);
    obj.set("type", n.type == NodeType::ROUTER ? "Router" : "Endpoint");
    obj.set("status", manager.getNodeStatusStr(n.status));
    obj.set("isRouter", n.type == NodeType::ROUTER);
    obj.set("queueSize", n.queue.size());
    obj.set("queueCapacity", n.queue.getCapacity());
    double util = n.queue.getCapacity() > 0 ? (double)n.queue.size() / n.queue.getCapacity() * 100.0 : 0;
    obj.set("queueUtilization", util);
    obj.set("processedPackets", n.processedPackets);
    obj.set("droppedPackets", n.droppedPackets);
    return obj;
}

Json ApiServer::linkToJson(const Link& l) {
    Json obj = Json::object();
    obj.set("id", l.id);
    obj.set("source", l.source);
    obj.set("target", l.target);
    obj.set("active", l.active);
    obj.set("weight", l.weight);
    return obj;
}

Json ApiServer::statsToJson(const NetworkStats& s) {
    Json o = Json::object();
    o.set("totalPackets", s.totalGenerated);
    o.set("totalGenerated", s.totalGenerated);
    o.set("delivered", s.totalDelivered);
    o.set("dropped", s.totalDropped);
    o.set("pending", s.totalPending);
    o.set("inTransit", s.totalInTransit);
    o.set("rerouted", s.totalRerouted);
    o.set("averageDelay", s.averageDelay);
    o.set("networkUtilization", s.networkUtilization);
    o.set("deliveryRate", s.deliveryRate);
    o.set("lossRate", s.lossRate);
    o.set("routerQueueUtilization", s.routerQueueUtilization);
    o.set("activeNodes", s.activeNodes);
    o.set("failedNodes", s.failedNodes);
    o.set("endpoints", s.endpointCount);
    o.set("routers", s.routerCount);
    o.set("priorityQueueSize", s.priorityQueueSize);
    o.set("historySize", s.historySize);
    return o;
}

// ========== ROUTE SETUP ==========

void ApiServer::setupRoutes() {

    // ============ NETWORK CONFIGURATION ============

    svr.Post("/api/network/generate", [this](const HttpRequest& req, HttpResponse& res) {
        auto body = Json::parseFlat(req.body);
        int endpoints = body.count("endpoints") ? std::stoi(body["endpoints"]) : 10;
        int routers = body.count("routers") ? std::stoi(body["routers"]) : 4;
        int queueCapacity = body.count("queueCapacity") ? std::stoi(body["queueCapacity"]) : 20;
        int packets = body.count("packets") ? std::stoi(body["packets"]) : 0;
        std::string algorithm = body.count("algorithm") ? body["algorithm"] : "Dijkstra";
        if (endpoints < 2) endpoints = 2;
        if (endpoints > 32) endpoints = 32;
        if (routers < 2) routers = 2;
        if (routers > 16) routers = 16;
        if (queueCapacity < 5) queueCapacity = 5;
        if (queueCapacity > 100) queueCapacity = 100;
        if (packets < 0) packets = 0;
        if (packets > 200) packets = 200;
        if (algorithm.empty()) algorithm = "Dijkstra";

        manager.generateNetwork(endpoints, routers, queueCapacity, algorithm);

        int generatedPackets = 0;
        if (packets > 0) {
            auto ids = manager.bulkGenerate(packets);
            generatedPackets = (int)ids.size();
        }

        Json r = Json::object();
        r.set("message", "Network generated");
        r.set("endpoints", endpoints);
        r.set("routers", routers);
        Json eps = Json::array();
        auto ids = manager.getEndpointIds();
        for (auto& e : ids) eps.push(Json::string(e));
        r.set("endpointIds", eps);
        r.set("config", manager.getConfig().numEndpoints);
        r.set("queueCapacity", queueCapacity);
        r.set("algorithm", algorithm);
        r.set("generatedPackets", generatedPackets);
        res.setJson(r.dump());
    });

    svr.Get("/api/network/config", [this](const HttpRequest&, HttpResponse& res) {
        manager.updateSimulation();
        Json r = Json::object();
        r.set("numEndpoints", manager.getConfig().numEndpoints);
        r.set("numRouters", manager.getConfig().numRouters);
        r.set("queueCapacity", manager.getConfig().queueCapacity);
        r.set("routingAlgorithm", manager.getConfig().routingAlgorithm);
        r.set("randomFailures", manager.getConfig().randomFailures);
        r.set("simulationSpeed", manager.getSimulationSpeed());
        r.set("isRunning", manager.isRunning());
        r.set("tickCount", manager.getTickCount());
        res.setJson(r.dump());
    });

    // ============ SIMULATION CONTROLS ============

    svr.Post("/api/simulation/start", [this](const HttpRequest&, HttpResponse& res) {
        manager.startSimulation();
        Json r = Json::object(); r.set("message", "Simulation started");
        res.setJson(r.dump());
    });

    svr.Post("/api/simulation/pause", [this](const HttpRequest&, HttpResponse& res) {
        manager.pauseSimulation();
        Json r = Json::object(); r.set("message", "Simulation paused");
        res.setJson(r.dump());
    });

    svr.Post("/api/simulation/resume", [this](const HttpRequest&, HttpResponse& res) {
        manager.resumeSimulation();
        Json r = Json::object(); r.set("message", "Simulation resumed");
        res.setJson(r.dump());
    });

    svr.Post("/api/simulation/reset", [this](const HttpRequest&, HttpResponse& res) {
        manager.resetSimulation();
        Json r = Json::object(); r.set("message", "Simulation reset");
        res.setJson(r.dump());
    });

    svr.Post("/api/simulation/speed", [this](const HttpRequest& req, HttpResponse& res) {
        auto body = Json::parseFlat(req.body);
        if (body.count("speed")) {
            manager.setSimulationSpeed(std::stoi(body["speed"]));
        }
        Json r = Json::object(); r.set("message", "Speed updated");
        res.setJson(r.dump());
    });

    // ============ NETWORK STATE / TOPOLOGY ============

    svr.Get("/api/network/state", [this](const HttpRequest&, HttpResponse& res) {
        manager.updateSimulation();
        NetworkStats s = manager.computeStats();
        Json state = statsToJson(s);
        std::string stateStr = "STOPPED";
        if (manager.getSimulationState() == NetworkManager::SimulationState::RUNNING) stateStr = "RUNNING";
        else if (manager.getSimulationState() == NetworkManager::SimulationState::PAUSED) stateStr = "PAUSED";
        state.set("simulationState", stateStr);
        state.set("isRunning", manager.isRunning());
        state.set("isPaused", manager.isPaused());
        state.set("isStopped", manager.isStopped());
        state.set("simulationSpeed", manager.getSimulationSpeed());
        state.set("tickCount", manager.getTickCount());
        res.setJson(state.dump());
    });

    svr.Get("/api/network/topology", [this](const HttpRequest&, HttpResponse& res) {
        manager.updateSimulation();
        Json topology = Json::object();

        Json nodesArr = Json::array();
        for (auto& kv : manager.getNetwork().getAllNodes()) {
            nodesArr.push(nodeToJson(kv.second));
        }
        topology.set("nodes", nodesArr);

        Json linksArr = Json::array();
        for (auto& l : manager.getNetwork().getLinks()) {
            linksArr.push(linkToJson(l));
        }
        topology.set("links", linksArr);

        res.setJson(topology.dump());
    });

    // ============ FAILURE EVENTS ============

    svr.Post("/api/network/failure", [this](const HttpRequest& req, HttpResponse& res) {
        auto body = Json::parseFlat(req.body);
        std::string type = body.count("type") ? body["type"] : "node";
        bool active = body.count("active") ? (body["active"] == "true" || body["active"] == "1") : false;

        if (type == "link") {
            std::string src = body.count("source") ? body["source"] : "";
            std::string tgt = body.count("target") ? body["target"] : "";
            if (src.empty() || tgt.empty()) {
                res.statusCode = 400;
                Json err = Json::object(); err.set("error", "Missing source/target for link");
                res.setJson(err.dump());
                return;
            }
            manager.setLinkActive(src, tgt, active);
        } else {
            std::string nodeId = body.count("nodeId") ? body["nodeId"] : "";
            if (nodeId.empty()) {
                res.statusCode = 400;
                Json err = Json::object(); err.set("error", "Missing nodeId");
                res.setJson(err.dump());
                return;
            }
            manager.setNodeActive(nodeId, active);
        }
        Json r = Json::object(); r.set("message", "Network event applied");
        res.setJson(r.dump());
    });

    // ============ PACKET GENERATION ============

    svr.Post("/api/packets/generate", [this](const HttpRequest& req, HttpResponse& res) {
        auto body = Json::parseFlat(req.body);
        std::string src = body.count("source") ? body["source"] : "N1";
        std::string dest = body.count("destination") ? body["destination"] : "N2";
        int priority = body.count("priority") ? std::stoi(body["priority"]) : 2;
        int size = body.count("size") ? std::stoi(body["size"]) : 512;

        std::string id = manager.generatePacket(src, dest, priority, size);

        Json r = Json::object();
        r.set("message", "Packet generated");
        r.set("packetId", id);
        res.setJson(r.dump());
    });

    svr.Post("/api/packets/bulk-generate", [this](const HttpRequest& req, HttpResponse& res) {
        auto body = Json::parseFlat(req.body);
        int count = body.count("count") ? std::stoi(body["count"]) : 5;
        if (count < 1) count = 1;
        if (count > 50) count = 50;

        auto ids = manager.bulkGenerate(count);
        Json arr = Json::array();
        for (auto& id : ids) arr.push(Json::string(id));

        Json r = Json::object();
        r.set("message", "Bulk packets generated");
        r.set("count", (int)ids.size());
        r.set("packetIds", arr);
        res.setJson(r.dump());
    });

    svr.Post("/api/packets/auto-generate", [this](const HttpRequest&, HttpResponse& res) {
        std::string id = manager.autoGeneratePacket();
        Json r = Json::object();
        r.set("message", "Random packet generated");
        r.set("packetId", id);
        res.setJson(r.dump());
    });

    // ============ PACKET LISTS ============

    svr.Get("/api/packets/all", [this](const HttpRequest&, HttpResponse& res) {
        manager.updateSimulation();
        Json arr = Json::array();
        auto all = manager.getAllPackets();
        for (auto& p : all) arr.push(packetToJson(p));
        Json r = Json::object();
        r.set("count", (int)all.size());
        r.set("packets", arr);
        res.setJson(r.dump());
    });

    svr.Get("/api/packets/filter", [this](const HttpRequest& req, HttpResponse& res) {
        manager.updateSimulation();
        std::string q = req.params.count("q") ? req.params.at("q") : "";
        if (q.empty()) {
            res.statusCode = 400;
            Json err = Json::object(); err.set("error", "Missing q parameter");
            res.setJson(err.dump());
            return;
        }
        Json arr = Json::array();
        auto matches = manager.searchPackets(q);
        for (auto& p : matches) arr.push(packetToJson(p));
        Json r = Json::object();
        r.set("count", (int)matches.size());
        r.set("packets", arr);
        res.setJson(r.dump());
    });

    // ============ SEARCH ============

    svr.Get("/api/packets/search", [this](const HttpRequest& req, HttpResponse& res) {
        manager.updateSimulation();
        std::string id = req.params.count("id") ? req.params.at("id") : "";
        if (id.empty()) {
            res.statusCode = 400;
            Json err = Json::object(); err.set("error", "Missing id parameter");
            res.setJson(err.dump());
            return;
        }

        Packet* found = manager.findPacket(id);
        if (!found) {
            res.statusCode = 404;
            Json err = Json::object(); err.set("error", "Packet not found");
            res.setJson(err.dump());
            return;
        }

        Json result = packetToJson(*found);

        auto& timelines = manager.getTimelines();
        if (timelines.count(id)) {
            Json steps = Json::array();
            for (auto& s : timelines.at(id).steps) steps.push(Json::string(s));
            result.set("timeline", steps);
        }

        auto& routeStacks = manager.getRouteStacks();
        if (routeStacks.count(id)) {
            Json stackArr = Json::array();
            auto elems = routeStacks.at(id).getElements();
            for (auto& e : elems) stackArr.push(Json::string(e));
            result.set("routeStack", stackArr);
        }

        res.setJson(result.dump());
    });

    // ============ SORT ============

    svr.Get("/api/packets/sort", [this](const HttpRequest& req, HttpResponse& res) {
        manager.updateSimulation();
        std::string by = req.params.count("by") ? req.params.at("by") : "id";
        std::string order = req.params.count("order") ? req.params.at("order") : "asc";

        auto sorted = manager.sortPackets(by, order);
        Json arr = Json::array();
        for (auto& p : sorted) arr.push(packetToJson(p));
        Json r = Json::object();
        r.set("count", (int)sorted.size());
        r.set("packets", arr);
        res.setJson(r.dump());
    });

    // ============ QUEUES ============

    svr.Get("/api/queues", [this](const HttpRequest&, HttpResponse& res) {
        manager.updateSimulation();
        Json result = Json::object();

        Json pqArr = Json::array();
        auto pqItems = manager.getPriorityQueue().getElements();
        for (auto& p : pqItems) pqArr.push(packetToJson(p));
        result.set("priorityQueue", pqArr);

        Json nodeQueues = Json::object();
        for (auto& kv : manager.getNetwork().getAllNodes()) {
            Json nq = Json::object();
            nq.set("size", kv.second.queue.size());
            nq.set("capacity", kv.second.queue.getCapacity());
            double util = kv.second.queue.getCapacity() > 0
                              ? (double)kv.second.queue.size() / kv.second.queue.getCapacity() * 100.0
                              : 0.0;
            nq.set("utilization", util);
            Json packets = Json::array();
            auto elems = kv.second.queue.getElements();
            for (auto& p : elems) packets.push(packetToJson(p));
            nq.set("packets", packets);
            nodeQueues.set(kv.first, nq);
        }
        result.set("nodeQueues", nodeQueues);

        res.setJson(result.dump());
    });

    // ============ HISTORY ============

    svr.Get("/api/history", [this](const HttpRequest&, HttpResponse& res) {
        manager.updateSimulation();
        Json arr = Json::array();
        auto history = manager.getHistory().getHistory();
        for (auto& p : history) arr.push(packetToJson(p));
        Json r = Json::object();
        r.set("count", (int)history.size());
        r.set("packets", arr);
        res.setJson(r.dump());
    });

    // ============ SCHEDULER ============

    svr.Get("/api/scheduler", [this](const HttpRequest&, HttpResponse& res) {
        manager.updateSimulation();
        Json arr = Json::array();
        auto nodes = manager.getScheduler().getNodes();
        for (auto& pr : nodes) {
            Json obj = Json::object();
            obj.set("nodeId", pr.first);
            obj.set("isActive", pr.second);
            arr.push(obj);
        }
        Json r = Json::object();
        r.set("count", (int)nodes.size());
        r.set("nodes", arr);
        res.setJson(r.dump());
    });

    // ============ ROUTES (STACKS) ============

    svr.Get("/api/routes", [this](const HttpRequest&, HttpResponse& res) {
        manager.updateSimulation();
        Json result = Json::object();
        auto& stacks = manager.getRouteStacks();
        for (auto& kv : stacks) {
            Json stackArr = Json::array();
            auto elems = kv.second.getElements();
            for (auto& e : elems) stackArr.push(Json::string(e));
            result.set(kv.first, stackArr);
        }
        res.setJson(result.dump());
    });

    // ============ ROUTE PREVIEW ============

    svr.Get("/api/route", [this](const HttpRequest& req, HttpResponse& res) {
        manager.updateSimulation();
        std::string src = req.params.count("source") ? req.params.at("source") : "";
        std::string dst = req.params.count("destination") ? req.params.at("destination") : "";
        Json r = Json::object();
        if (src.empty() || dst.empty()) {
            r.set("error", "Missing source or destination");
            res.setJson(r.dump());
            return;
        }
        auto route = manager.previewRoute(src, dst);
        Json arr = Json::array();
        for (auto& hop : route) arr.push(Json::string(hop));
        r.set("route", arr);
        r.set("source", src);
        r.set("destination", dst);
        if (route.empty()) r.set("message", "No route available");
        else r.set("message", "Shortest path computed via Dijkstra");
        res.setJson(r.dump());
    });

    // ============ STATISTICS ============

    svr.Get("/api/statistics", [this](const HttpRequest&, HttpResponse& res) {
        manager.updateSimulation();
        NetworkStats s = manager.computeStats();
        res.setJson(statsToJson(s).dump());
    });

    // ============ ALERTS ============

    svr.Get("/api/alerts", [this](const HttpRequest&, HttpResponse& res) {
        manager.updateSimulation();
        Json arr = Json::array();
        auto& alerts = manager.getAlerts();
        for (auto& a : alerts) {
            Json obj = Json::object();
            obj.set("message", a.message);
            std::string severity;
            switch (a.severity) {
                case AlertSeverity::INFO: severity = "info"; break;
                case AlertSeverity::WARNING: severity = "warning"; break;
                case AlertSeverity::CRITICAL: severity = "critical"; break;
            }
            obj.set("severity", severity);
            obj.set("timestamp", a.timestamp);
            arr.push(obj);
        }
        Json r = Json::object();
        r.set("count", (int)alerts.size());
        r.set("alerts", arr);
        res.setJson(r.dump());
    });

    // ============ DATA STRUCTURES ============

    svr.Get("/api/datastructures", [this](const HttpRequest&, HttpResponse& res) {
        manager.updateSimulation();
        Json result = Json::object();

        // Graph: nodes + edges
        Json graph = Json::object();
        Json gNodes = Json::array();
        for (auto& kv : manager.getNetwork().getAllNodes()) {
            gNodes.push(nodeToJson(kv.second));
        }
        graph.set("nodes", gNodes);
        Json gLinks = Json::array();
        for (auto& l : manager.getNetwork().getLinks()) {
            gLinks.push(linkToJson(l));
        }
        graph.set("links", gLinks);
        result.set("graph", graph);

        // Hash Map (packet store)
        Json hash = Json::object();
        for (auto& kv : manager.getPacketIndex()) {
            Json entry = packetToJson(kv.second);
            hash.set(kv.first, entry);
        }
        result.set("hashMap", hash);

        // Priority Queue (in transit)
        Json pqArr = Json::array();
        auto pqItems = manager.getPriorityQueue().getElements();
        for (auto& p : pqItems) pqArr.push(packetToJson(p));
        result.set("priorityQueue", pqArr);

        // FIFO Queues (node queues)
        Json queues = Json::object();
        for (auto& kv : manager.getNetwork().getAllNodes()) {
            Json q = Json::object();
            q.set("size", kv.second.queue.size());
            q.set("capacity", kv.second.queue.getCapacity());
            Json pkts = Json::array();
            for (auto& p : kv.second.queue.getElements()) pkts.push(packetToJson(p));
            q.set("packets", pkts);
            queues.set(kv.first, q);
        }
        result.set("queues", queues);

        // Doubly Linked List (history)
        Json dll = Json::array();
        auto historyItems = manager.getHistory().getHistory();
        for (auto& p : historyItems) dll.push(packetToJson(p));
        result.set("doublyLinkedList", dll);

        // Circular Linked List (scheduler)
        Json cll = Json::array();
        auto schedulerNodes = manager.getScheduler().getNodes();
        for (auto& pr : schedulerNodes) {
            Json obj = Json::object();
            obj.set("nodeId", pr.first);
            obj.set("isActive", pr.second);
            cll.push(obj);
        }
        result.set("circularLinkedList", cll);

        // Stacks (route stacks)
        Json stacks = Json::object();
        auto& routeStacks = manager.getRouteStacks();
        for (auto it = routeStacks.begin(); it != routeStacks.end(); ++it) {
            Json stackArr = Json::array();
            auto elems = it->second.getElements();
            for (auto& e : elems) stackArr.push(Json::string(e));
            stacks.set(it->first, stackArr);
        }
        result.set("stacks", stacks);

        // Array/List (in-transit + alert records)
        Json list = Json::object();
        Json transitArr = Json::array();
        for (auto& p : manager.getPriorityQueue().getElements()) {
            Json obj = Json::object();
            obj.set("packetId", p.packetId);
            obj.set("currentNode", p.currentNode);
            obj.set("nextHop", p.hopIndex + 1 < (int)p.routeVec.size() ? p.routeVec[p.hopIndex + 1] : p.destination);
            transitArr.push(obj);
        }
        list.set("inTransit", transitArr);
        list.set("alertCount", (int)manager.getAlerts().size());
        list.set("packetCount", (int)manager.getPacketIndex().size());
        result.set("arrayList", list);

        res.setJson(result.dump());
    });
}
