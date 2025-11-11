#ifndef TCP_SERVER_HPP
#define TCP_SERVER_HPP

#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <cstring>      // For memset
#include <unistd.h>     // For close
#include <arpa/inet.h>  // For inet_addr
#include <sys/socket.h> // For socket functions
#include <netinet/in.h> // For sockaddr_in

class TCPServer {
    public:
        TCPServer(int port);
        ~TCPServer();
        void start();
        void stop();
    
    private:
        void run();
        void handleClient(int clientSocket);
    
        int port;
        int serverSocket;
        std::atomic<bool> isRunning;
        std::vector<std::thread> clientThreads;
};


#endif // TCP_SERVER_HPP