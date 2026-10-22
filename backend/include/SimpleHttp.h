#ifndef SIMPLEHTTP_H
#define SIMPLEHTTP_H

// Minimal HTTP server for Windows using Winsock2
// Compatible with GCC 6.3+ and C++14

#ifdef _WIN32
  #ifndef _WIN32_WINNT
    #define _WIN32_WINNT 0x0501
  #endif
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #pragma comment(lib, "ws2_32.lib")
#else
  #include <sys/socket.h>
  #include <sys/select.h>
  #include <netinet/in.h>
  #include <unistd.h>
  #define SOCKET int
  #define INVALID_SOCKET -1
  #define SOCKET_ERROR -1
  #define closesocket close
#endif

#include <string>
#include <functional>
#include <map>
#include <sstream>
#include <iostream>
#include <cstring>
#include <vector>
#include <algorithm>

struct HttpRequest {
    std::string method;
    std::string path;
    std::string body;
    std::map<std::string, std::string> params; // query params
    std::map<std::string, std::string> headers;
};

struct HttpResponse {
    int statusCode = 200;
    std::string body;
    std::string contentType = "application/json";
    std::map<std::string, std::string> headers;
    
    void setJson(const std::string& json) {
        body = json;
        contentType = "application/json";
    }
};

// Simple URL decode
inline std::string urlDecode(const std::string& str) {
    std::string result;
    for (size_t i = 0; i < str.size(); ++i) {
        if (str[i] == '%' && i + 2 < str.size()) {
            int val;
            std::istringstream iss(str.substr(i + 1, 2));
            if (iss >> std::hex >> val) {
                result += static_cast<char>(val);
                i += 2;
            } else {
                result += str[i];
            }
        } else if (str[i] == '+') {
            result += ' ';
        } else {
            result += str[i];
        }
    }
    return result;
}

typedef std::function<void(const HttpRequest&, HttpResponse&)> RouteHandler;
typedef std::function<void()> TickCallback;

class SimpleHttpServer {
private:
    SOCKET serverSocket;
    int port;
    bool running;
    
    // Route storage: method+path -> handler
    std::map<std::string, RouteHandler> routes;
    // Optional tick callback invoked periodically by the event loop
    TickCallback tickCb;

    void parseQueryParams(const std::string& query, HttpRequest& req) {
        std::istringstream stream(query);
        std::string pair;
        while (std::getline(stream, pair, '&')) {
            size_t eq = pair.find('=');
            if (eq != std::string::npos) {
                req.params[urlDecode(pair.substr(0, eq))] = urlDecode(pair.substr(eq + 1));
            }
        }
    }

    HttpRequest parseRequest(const std::string& raw) {
        HttpRequest req;
        size_t headerEnd = raw.find("\r\n\r\n");
        size_t headerSepLen = 4;
        if (headerEnd == std::string::npos) {
            headerEnd = raw.find("\n\n");
            headerSepLen = 2;
        }

        std::string headerPart = (headerEnd != std::string::npos) ? raw.substr(0, headerEnd) : raw;
        std::istringstream stream(headerPart);
        std::string line;
        
        // Parse request line
        if (std::getline(stream, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            std::istringstream reqLine(line);
            std::string pathWithQuery;
            reqLine >> req.method >> pathWithQuery;
            
            // Split path and query
            size_t qmark = pathWithQuery.find('?');
            if (qmark != std::string::npos) {
                req.path = pathWithQuery.substr(0, qmark);
                parseQueryParams(pathWithQuery.substr(qmark + 1), req);
            } else {
                req.path = pathWithQuery;
            }
        }
        
        // Parse headers
        int contentLength = 0;
        while (std::getline(stream, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (line.empty()) continue;
            size_t colon = line.find(':');
            if (colon != std::string::npos) {
                std::string key = line.substr(0, colon);
                std::string val = line.substr(colon + 1);
                // trim leading/trailing whitespace
                while (!val.empty() && (val.front() == ' ' || val.front() == '\t')) val.erase(val.begin());
                while (!val.empty() && (val.back() == ' ' || val.back() == '\t' || val.back() == '\r')) val.pop_back();
                req.headers[key] = val;
                
                std::string lower_key = key;
                std::transform(lower_key.begin(), lower_key.end(), lower_key.begin(), ::tolower);
                if (lower_key == "content-length") {
                    try {
                        contentLength = std::stoi(val);
                    } catch (...) {}
                }
            }
        }
        
        // Extract body
        if (headerEnd != std::string::npos) {
            size_t bodyStart = headerEnd + headerSepLen;
            if (contentLength > 0 && bodyStart + contentLength <= raw.size()) {
                req.body = raw.substr(bodyStart, contentLength);
            } else if (bodyStart < raw.size()) {
                req.body = raw.substr(bodyStart);
            }
        }
        return req;
    }

    std::string buildResponse(const HttpResponse& res) {
        std::ostringstream ss;
        ss << "HTTP/1.1 " << res.statusCode << " OK\r\n";
        ss << "Content-Type: " << res.contentType << "\r\n";
        ss << "Content-Length: " << res.body.size() << "\r\n";
        ss << "Access-Control-Allow-Origin: *\r\n";
        ss << "Access-Control-Allow-Methods: GET, POST, PUT, OPTIONS\r\n";
        ss << "Access-Control-Allow-Headers: Content-Type\r\n";
        for (auto& h : res.headers) {
            ss << h.first << ": " << h.second << "\r\n";
        }
        ss << "Connection: close\r\n";
        ss << "\r\n";
        ss << res.body;
        return ss.str();
    }

    void handleClient(SOCKET clientSocket) {
        std::string rawRequest;
        char buffer[8192];
        
        // Read until full header is received
        size_t headerEnd = std::string::npos;
        size_t headerSepLen = 4;
        while (headerEnd == std::string::npos) {
            int bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
            if (bytesRead <= 0) break;
            rawRequest.append(buffer, bytesRead);
            headerEnd = rawRequest.find("\r\n\r\n");
            if (headerEnd != std::string::npos) {
                headerSepLen = 4;
                break;
            }
            headerEnd = rawRequest.find("\n\n");
            if (headerEnd != std::string::npos) {
                headerSepLen = 2;
                break;
            }
        }
        
        if (rawRequest.empty()) {
            closesocket(clientSocket);
            return;
        }

        // Check Content-Length and Expect headers
        int contentLength = 0;
        std::string lowerRaw = rawRequest;
        for (char& c : lowerRaw) c = (char)::tolower((unsigned char)c);

        if (lowerRaw.find("expect: 100-continue") != std::string::npos) {
            const char* cont = "HTTP/1.1 100 Continue\r\n\r\n";
            send(clientSocket, cont, (int)strlen(cont), 0);
        }

        size_t clPos = lowerRaw.find("content-length:");
        if (clPos != std::string::npos) {
            size_t valStart = clPos + 15;
            size_t valEnd = lowerRaw.find('\n', valStart);
            std::string valStr = lowerRaw.substr(valStart, (valEnd != std::string::npos ? valEnd - valStart : 20));
            while (!valStr.empty() && (valStr.front() == ' ' || valStr.front() == '\t')) valStr.erase(valStr.begin());
            while (!valStr.empty() && (valStr.back() == ' ' || valStr.back() == '\t' || valStr.back() == '\r')) valStr.pop_back();
            try {
                contentLength = std::stoi(valStr);
            } catch (...) {}
        }

        if (headerEnd != std::string::npos && contentLength > 0) {
            size_t bodyStart = headerEnd + headerSepLen;
            while (rawRequest.size() - bodyStart < (size_t)contentLength) {
                int bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
                if (bytesRead <= 0) break;
                rawRequest.append(buffer, bytesRead);
            }
        }

        HttpRequest req = parseRequest(rawRequest);
        HttpResponse res;
        
        // Handle OPTIONS (CORS preflight)
        if (req.method == "OPTIONS") {
            res.body = "";
            std::string response = buildResponse(res);
            send(clientSocket, response.c_str(), response.size(), 0);
            closesocket(clientSocket);
            return;
        }
        
        // Find matching route
        std::string routeKey = req.method + " " + req.path;
        auto it = routes.find(routeKey);
        
        if (it != routes.end()) {
            it->second(req, res);
        } else {
            res.statusCode = 404;
            res.body = "{\"error\":\"Not found\"}";
        }
        
        std::string response = buildResponse(res);
        send(clientSocket, response.c_str(), response.size(), 0);
        closesocket(clientSocket);
    }

public:
    SimpleHttpServer() : serverSocket(INVALID_SOCKET), port(8080), running(false) {}

    void Get(const std::string& path, RouteHandler handler) {
        routes["GET " + path] = handler;
    }
    
    void Post(const std::string& path, RouteHandler handler) {
        routes["POST " + path] = handler;
    }

    // Register a periodic callback (e.g. to advance the simulation)
    void setTickCallback(TickCallback cb) {
        tickCb = cb;
    }

    bool listen(int p) {
        port = p;
        
#ifdef _WIN32
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            std::cerr << "WSAStartup failed" << std::endl;
            return false;
        }
#endif
        
        serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (serverSocket == INVALID_SOCKET) {
            std::cerr << "Socket creation failed" << std::endl;
            return false;
        }
        
        // Allow port reuse
        int opt = 1;
        setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
        
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);
        
        if (bind(serverSocket, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
            std::cerr << "Bind failed" << std::endl;
            closesocket(serverSocket);
            return false;
        }
        
        if (::listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
            std::cerr << "Listen failed" << std::endl;
            closesocket(serverSocket);
            return false;
        }
        
        running = true;
        std::cout << "Server listening on http://localhost:" << port << std::endl;

        while (running) {
#ifdef _WIN32
            fd_set rfds;
            FD_ZERO(&rfds);
            FD_SET(serverSocket, &rfds);
            timeval tv;
            tv.tv_sec = 0;
            tv.tv_usec = 100000; // 100ms
            int sel = select(0, &rfds, NULL, NULL, &tv);
            if (sel > 0 && FD_ISSET(serverSocket, &rfds)) {
                SOCKET clientSocket = accept(serverSocket, NULL, NULL);
                if (clientSocket != INVALID_SOCKET) {
                    handleClient(clientSocket);
                }
            }
#else
            fd_set rfds;
            FD_ZERO(&rfds);
            FD_SET(serverSocket, &rfds);
            timeval tv;
            tv.tv_sec = 0;
            tv.tv_usec = 100000;
            int sel = select(serverSocket + 1, &rfds, NULL, NULL, &tv);
            if (sel > 0 && FD_ISSET(serverSocket, &rfds)) {
                SOCKET clientSocket = accept(serverSocket, NULL, NULL);
                if (clientSocket != INVALID_SOCKET) {
                    handleClient(clientSocket);
                }
            }
#endif
            if (tickCb) tickCb();
        }

        closesocket(serverSocket);
#ifdef _WIN32
        WSACleanup();
#endif
        return true;
    }
    
    void stop() {
        running = false;
        closesocket(serverSocket);
    }
};

#endif // SIMPLEHTTP_H
