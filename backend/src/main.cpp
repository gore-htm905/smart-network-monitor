
#include "ApiServer.h"
#include "NetworkManager.h"
#include <cstdlib>
#include <iostream>

int main() {
    std::cout << "Starting Smart Communication Network Monitoring Backend..."
              << std::endl;

    NetworkManager manager;
    manager.generateNetwork(10, 4);

    ApiServer server(manager);

    int port = 8085;

    const char* portEnv = std::getenv("PORT");
    if (portEnv != nullptr && *portEnv != '\0') {
        port = std::atoi(portEnv);
    }

    std::cout << "Server listening on port " << port << std::endl;

    server.start(port);

    return 0;
}
