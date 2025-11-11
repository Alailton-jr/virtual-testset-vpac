#include <iostream>
#include "logger.hpp"
#include "metrics.hpp"

int main() {
    std::cout << "Starting minimal test..." << std::endl;
    
    std::cout << "Initializing Logger..." << std::endl;
    Logger::init(LogLevel::INFO, "");
    
    std::cout << "Initializing Metrics..." << std::endl;
    Metrics::init();
    
    std::cout << "Success! Shutting down..." << std::endl;
    Logger::shutdown();
    
    return 0;
}
