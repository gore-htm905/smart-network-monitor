#ifndef APISERVER_H
#define APISERVER_H

#include "NetworkManager.h"
#include "SimpleHttp.h"
#include "SimpleJson.h"

class ApiServer {
private:
    SimpleHttpServer svr;
    NetworkManager& manager;

    Json packetToJson(const Packet& p);
    Json nodeToJson(Node& n);
    Json linkToJson(const Link& l);
    Json statsToJson(const NetworkStats& s);
    void setupRoutes();

public:
    ApiServer(NetworkManager& mgr) : manager(mgr) {
        setupRoutes();
        svr.setTickCallback([this]() {
            manager.updateSimulation();
        });
    }

    void start(int port) {
        svr.listen(port);
    }

    void stop() {
        svr.stop();
    }
};

#endif // APISERVER_H