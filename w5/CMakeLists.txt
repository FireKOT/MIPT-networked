cmake_minimum_required(VERSION 3.13)

project(w5)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

SET(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(W5_CLIENT_SOURCES
    client.cpp
    protocol.cpp
    entity.cpp
    )

set(W5_SERVER_SOURCES
    server.cpp
    protocol.cpp
    entity.cpp
    )


include_directories("../3rdParty/enet/include")

add_executable(client ${W5_CLIENT_SOURCES})
target_link_libraries(client PUBLIC project_options project_warnings)
target_link_libraries(client PUBLIC raylib enet)

add_executable(server ${W5_SERVER_SOURCES})
target_link_libraries(server PUBLIC project_options project_warnings)
target_link_libraries(server PUBLIC enet)

