

project(tests)

add_library(${PROJECT_NAME} 
    src/transient.cpp
    src/tests.cpp
)

target_include_directories( ${PROJECT_NAME}
    PUBLIC 
        ${PROJECT_SOURCE_DIR}/include
# Homebrew nlohmann-json include path for macOS
        $<$<PLATFORM_ID:Darwin>:/opt/homebrew/opt/nlohmann-json/include>
)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        tools
        sampledValue
        protocols
        sniffer
)