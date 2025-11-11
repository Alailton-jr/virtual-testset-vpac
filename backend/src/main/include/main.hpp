#ifndef MAIN_HPP
#define MAIN_HPP

#include <iostream>
#include <vector>

#include "sniffer.hpp"
#include "tests.hpp"
#include "raw_socket_platform.hpp"

#include <thread>
#include <atomic>
#include <cstring>      
#include <unistd.h>     
#include <arpa/inet.h>  
#include <sys/socket.h> 
#include <netinet/in.h> 

#include <nlohmann/json.hpp>
#include <utility>


class TCPServer {
    public:
        TCPServer(int port);
        ~TCPServer();
        Tests_Class testSet;
        void start();
        void stop();
    
    private:
        void run();
        void handleClient(int clientSocket);
    
        int serverSocket;
        int port;
        std::atomic<bool> isRunning;
        std::vector<std::thread> clientThreads;
};

#endif // MAIN_HPP