#pragma once


#include <stdint.h>
#include <string>


constexpr int MAX_NICKNAME_LENGTH = 50;


enum class NetworkMessageType {

    Start,
    ServerAddres,
    PlayerIdentifyer,
    PlayerSelfIdentifyer,
    PlayerPing,
    PlayerPosition,
};


struct NetworkMessage {

    NetworkMessageType type;
    uint8_t payload[];
};


struct ServerAddres {

    char hostName[MAX_NICKNAME_LENGTH];
    uint16_t port;
}; 


struct PlayerIdentifyer {

    size_t id = 0;
    char nickname[MAX_NICKNAME_LENGTH];
}; 

struct PlayerSelfIdentifyer {

    size_t id = 0;
    char nickname[MAX_NICKNAME_LENGTH];
}; 

struct PlayerPing {

    size_t id = 0;
    uint8_t ping = 0;
};

struct PlayerPosition {

    size_t id = 0;
    float x = 0.f, y = 0.f;
};
