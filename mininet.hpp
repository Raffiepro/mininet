#pragma once

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <stdint.h>
#include <stdio.h>

static bool startedWsa=false;
static void initNet() {
    if(!startedWsa) {
        WSADATA wsaData;
        int result = WSAStartup(MAKEWORD(2,2), &wsaData);
        if (result != 0) {
            printf("WSAStartup failed with error: %i",result);
            exit(EXIT_FAILURE);
        }
        startedWsa=true;
    }
}

#else

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

static bool startedUnix = false;
static void initNet() {
    if(!startedUnix) {
        signal(SIGPIPE, SIG_IGN); // Ignore SIGPIPE globally
        startedUnix = true;
    }
}

#endif

#include <stdlib.h>
#include <stdio.h>
#include <cstring>
#include <string>
#include <vector>

struct TCPServer {
    std::vector<int> clients;
    int socket;
    sockaddr_in address;
    
    inline TCPServer(uint16_t port) {
        start(port);
    }
    inline TCPServer() {}
    inline ~TCPServer() {
        stop();
    }

    bool setBlocking(bool blocking)
    {
    #ifdef _WIN32
        unsigned long mode = blocking ? 0 : 1;
        return (ioctlsocket(socket, FIONBIO, &mode) == 0);
    #else
        int flags = fcntl(socket, F_GETFL, 0);
        if (flags == -1) return false;
        flags = blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
        return (fcntl(socket, F_SETFL, flags) == 0);
    #endif
    }
    void start(uint16_t port) {
        initNet();
        socket = ::socket(AF_INET, SOCK_STREAM, 0);
        address.sin_family = AF_INET;
        address.sin_port = htons(port);
        address.sin_addr.s_addr = INADDR_ANY;
        bind(socket, (struct sockaddr*)&address,
            sizeof(address));
        listen(socket, 5);
    }
    int accept() {
        int clientSocket = ::accept(socket, nullptr, nullptr);
        clients.push_back(clientSocket);
        return clientSocket;
    }
    inline ssize_t recv(size_t client, void* buff, size_t n) {
        #ifdef _WIN32
        return ::recv(clients[client], (char*)buff, n, 0);
        #else
        return ::recv(clients[client], buff, n, 0);
        #endif
    }
    inline void send(size_t client, const void* buff, size_t n) {
        #ifdef _WIN32
        ::send(clients[client], (const char*)buff, n, 0);
        #else
        ::send(clients[client], buff, n, 0);
        #endif
    }
    inline void send(size_t client, const std::string s) {
        #ifdef _WIN32
        ::send(clients[client], s.data(), s.length(), 0);
        #else
        ::send(clients[client], (const void*)s.data(), s.length(), 0);
        #endif
    }
    inline void stop() {
        #ifdef _WIN32
        closesocket(socket);
        #else
        close(socket);
        #endif
    }
};
struct TCPClient {
    int socket;
    sockaddr_in address;
    
    inline TCPClient(const char* ip, uint16_t port) {
        start(ip, port);
    }
    inline TCPClient() {}
    inline ~TCPClient() {
        stop();
    }

    bool setBlocking(bool blocking)
    {
    #ifdef _WIN32
        unsigned long mode = blocking ? 0 : 1;
        return (ioctlsocket(socket, FIONBIO, &mode) == 0);
    #else
        int flags = fcntl(socket, F_GETFL, 0);
        if (flags == -1) return false;
        flags = blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
        return (fcntl(socket, F_SETFL, flags) == 0);
    #endif
    }
    void start(const char* ip, uint16_t port) {
        initNet();
        socket = ::socket(AF_INET, SOCK_STREAM, 0);
        if (socket < 0) {
            printf("socket error");
            exit(EXIT_FAILURE);
        }

        address.sin_family = AF_INET;
        address.sin_port = htons(port);
        if (inet_pton(AF_INET, ip, &address.sin_addr) <= 0) {
            printf("inet_pton error");
            #ifdef _WIN32
            closesocket(socket);
            #else
            close(socket);
            #endif
            exit(EXIT_FAILURE);
        }
        
        connect(socket, (struct sockaddr*)&address,
        sizeof(address));
    }
    inline ssize_t recv(void* buff, size_t n) const {
        #ifdef _WIN32
        return ::recv(socket, (char*)buff, n, 0);
        #else
        return ::recv(socket, buff, n, 0);
        #endif
    }
    inline void send(const void* buff, size_t n) const {
        #ifdef _WIN32
        ::send(socket, (const char*)buff, n, 0);
        #else
        ::send(socket, buff, n, 0);
        #endif
    }
    inline void send(const std::string s) const {
        #ifdef _WIN32
        ::send(socket, s.data(), s.length(), 0);
        #else
        ::send(socket, (const void*)s.data(), s.length(), 0);
        #endif
    }
    inline void stop() {
        #ifdef _WIN32
        closesocket(socket);
        #else
        close(socket);
        #endif
    }
};

struct UDPServer {
    int socket;
    sockaddr_in address;
    
    inline UDPServer(uint16_t port) {
        start(port);
    }
    inline UDPServer() {}
    inline ~UDPServer() {
        stop();
    }

    bool setBlocking(bool blocking)
    {
    #ifdef _WIN32
        unsigned long mode = blocking ? 0 : 1;
        return (ioctlsocket(socket, FIONBIO, &mode) == 0);
    #else
        int flags = fcntl(socket, F_GETFL, 0);
        if (flags == -1) return false;
        flags = blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
        return (fcntl(socket, F_SETFL, flags) == 0);
    #endif
    }
    void start(uint16_t port) {
        initNet();
        socket = ::socket(AF_INET, SOCK_DGRAM, 0);
        memset(&address, 0, sizeof(address));
        address.sin_family = AF_INET;
        address.sin_port = htons(port);
        address.sin_addr.s_addr = INADDR_ANY;
        bind(socket, (struct sockaddr*)&address,
            sizeof(address));
    }
    inline ssize_t recv(sockaddr_in* client, void* buff, size_t n) {
        socklen_t len = sizeof(sockaddr_in);
        #ifdef _WIN32
        return recvfrom(socket,(char*)buff,n,
                0, ( struct sockaddr *) client,
                &len);
        #else
        return recvfrom(socket,(char*)buff,n,
                0, ( struct sockaddr *) client,
                &len);
        #endif
    }
    inline void send(sockaddr_in* client, const void* buff, size_t n) {
        #ifdef _WIN32
        sendto(socket, (const char*)buff, n,
            0, (const struct sockaddr *) client,
                sizeof(sockaddr));
        #else
        sendto(socket, (const void *)buff, n,
            0, (const struct sockaddr *) client,
                sizeof(sockaddr));
        #endif
    }
    inline void send(sockaddr_in* client, const std::string s) {
        #ifdef _WIN32
        sendto(socket, s.data(), s.length(),
            0, (const struct sockaddr *) client,
                sizeof(sockaddr));
        #else
        sendto(socket, (const void *)s.data(), s.length(),
            0, (const struct sockaddr *) client,
                sizeof(sockaddr));
        #endif
    }
    inline void stop() {
        #ifdef _WIN32
        closesocket(socket);
        #else
        close(socket);
        #endif
    }
};
struct UDPClient {
    int socket;
    sockaddr_in address;
    
    inline UDPClient(const char* ip, uint16_t port) {
        start(ip, port);
    }
    inline UDPClient() {}
    inline ~UDPClient() {
        stop();
    }

    bool setBlocking(bool blocking)
    {
    #ifdef _WIN32
        unsigned long mode = blocking ? 0 : 1;
        return (ioctlsocket(socket, FIONBIO, &mode) == 0);
    #else
        int flags = fcntl(socket, F_GETFL, 0);
        if (flags == -1) return false;
        flags = blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
        return (fcntl(socket, F_SETFL, flags) == 0);
    #endif
    }
    void start(const char* ip, uint16_t port) {
        initNet();
        socket = ::socket(AF_INET, SOCK_DGRAM, 0);
        if (socket < 0) {
            printf("socket error");
            exit(EXIT_FAILURE);
        }

        address.sin_family = AF_INET;
        address.sin_port = htons(port);
        if (inet_pton(AF_INET, ip, &address.sin_addr) <= 0) {
            printf("inet_pton error");
            #ifdef _WIN32
            closesocket(socket);
            #else
            close(socket);
            #endif
            exit(EXIT_FAILURE);
        }
    }
    inline ssize_t recv(void* buff, size_t n) const {
        socklen_t len = sizeof(address);
        #ifdef _WIN32
        return recvfrom(socket, (char*)buff, n, 
				0, (struct sockaddr *) &address, 
				&len);
        #else
        return recvfrom(socket, buff, n, 
				0, (struct sockaddr *) &address, 
				&len);
        #endif
    }
    inline void send(const void* buff, size_t n) const {
        #ifdef _WIN32
        sendto(socket, (const char*)buff, n, 
		0, (const struct sockaddr *) &address, 
			sizeof(address));
        #else
        sendto(socket, buff, n, 
		0, (const struct sockaddr *) &address, 
			sizeof(address));
        #endif
    }
    inline void send(const std::string s) const {
        #ifdef _WIN32
        sendto(socket, s.data(), s.length(), 
		0, (const struct sockaddr *) &address, 
			sizeof(address));
        #else
        sendto(socket, (const void*)s.data(), s.length(), 
		0, (const struct sockaddr *) &address, 
			sizeof(address));
        #endif
    }
    inline void stop() {
        #ifdef _WIN32
        closesocket(socket);
        #else
        close(socket);
        #endif
    }
};
