

project(api)

add_library(${PROJECT_NAME} 
    src/tcp_server.cpp
    src/http_server.cpp
    src/ws_server.cpp
)

target_include_directories( ${PROJECT_NAME}
    PUBLIC 
        ${PROJECT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_SOURCE_DIR}/third_party/cpp-httplib
        ${CMAKE_SOURCE_DIR}/third_party/json/include
        ${CMAKE_SOURCE_DIR}/src/core/include
        ${CMAKE_SOURCE_DIR}/src/sequence/include
        ${CMAKE_SOURCE_DIR}/src/analyzer/include
        ${CMAKE_SOURCE_DIR}/src/testers/include
)

# Disable strict warnings for third-party code (websocketpp/asio)
target_include_directories( ${PROJECT_NAME} SYSTEM PRIVATE
    ${CMAKE_SOURCE_DIR}/third_party/websocketpp
    ${CMAKE_SOURCE_DIR}/third_party/asio/asio/include
)

# Disable conversion warnings for WebSocket code (due to ASIO library warnings)
target_compile_options(${PROJECT_NAME} PRIVATE
    -Wno-sign-conversion
    -Wno-conversion
)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        vts_core
    PRIVATE
        tools
        sequence
        vts_analyzer
        vts_testers
        Threads::Threads
)