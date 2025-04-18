cmake_minimum_required(VERSION 3.13)

project(w2)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

SET(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(W2_CLIENT_SOURCES
    client.cpp
    )

set(W2_LOBBY_SOURCES
    lobby.cpp
    )

set(W2_SERVER_SOURCES
    server.cpp
    )


include_directories("../3rdParty/enet/include")

add_executable(client ${W2_CLIENT_SOURCES})
target_link_libraries(client PUBLIC project_options project_warnings)
target_link_libraries(client PUBLIC raylib enet)

add_executable(lobby ${W2_LOBBY_SOURCES})
target_link_libraries(lobby PUBLIC project_options project_warnings)
target_link_libraries(lobby PUBLIC enet)

add_executable(server ${W2_SERVER_SOURCES})
target_link_libraries(server PUBLIC project_options project_warnings)
target_link_libraries(server PUBLIC enet)



