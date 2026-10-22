#include "ApiServer.h"
#include "NetworkManager.h"
#include <iostream>

int main() {
    std::cout << "Starting Smart Communication Network Monitoring Backend..." << std::endl;

    NetworkManager manager;
    manager.generateNetwork(10, 4);

    ApiServer server(manager);

    int port = 8085;
    std::cout << "Server listening on http://localhost:" << port << std::endl;
    server.start(port);

    return 0;
}