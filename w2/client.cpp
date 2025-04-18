#include "raylib.h"
#include <enet/enet.h>
#include <iostream>
#include <cstring>
#include <map>
#include <cstdlib>
#include <ctime> 

#include "NetworkUtils.hpp"


struct PeerPlayerData {

    std::string nickname = "";

    uint8_t ping = 0;

    float x = 0.f, y = 0.f;
}; 


struct LocalPlayerData {

    LocalPlayerData (float posX = 0.f, float posY = 0.f, float velX = 0.f, float velY = 0.f): 
        posx(posX), posy(posY), velx(velX), vely(velY), id(0), nickname("") {}

    float posx, posy;
    float velx, vely;

    size_t id;
    std::string nickname;
}; 


struct ClientData {

    ClientData (const LocalPlayerData &data): localPlayer(data), players() {}

    LocalPlayerData localPlayer;

    std::map <size_t, PeerPlayerData> players;
};


class Drawer {

public:

    Drawer (const ClientData &client, int width = 800, int height = 600): width_(width), height_(height), client_(client) {

        InitWindow(width_, height_, "Client");
        SetTargetFPS(60);
    }

    void drawFrame () {

        BeginDrawing();

        ClearBackground(BLACK);

        DrawText("Nickname", 20, 20, 20, WHITE);
        DrawText("|  Ping",    150, 20, 20, WHITE);

        DrawText("--------------------", 20, 40, 20, WHITE);

        size_t offset = 60;
        for (auto &[id, peerPlayerData]: client_.players) {

            DrawText(peerPlayerData.nickname.c_str(),        20, offset, 20, WHITE);
            DrawText(TextFormat("|  %d", peerPlayerData.ping), 150, offset, 20, WHITE);

            offset += 20;
        }


        DrawCircleV(Vector2{ client_.localPlayer.posx, client_.localPlayer.posy }, 10.f, MAGENTA);

        for (auto &[id, peerPlayerData]: client_.players) {

            if (id != client_.localPlayer.id) {

                DrawCircleV(Vector2{ peerPlayerData.x, peerPlayerData.y }, 10.f, WHITE);
            }
        }
        
        EndDrawing();
    }

    std::string getPlayers () {

        std::string players = "";

        for (auto &[_, peerPlayerData]: client_.players) {

            players += " " + peerPlayerData.nickname + ": " + std::to_string(peerPlayerData.ping) + " ";
        }

        return players;
    }

private:

    int width_, height_;

    const ClientData &client_;
};  


class Network {

public:

    Network (ClientData &clientData): clientData_(clientData) {

        if (enet_initialize() != 0) {

            fprintf(stderr, "Cannot init ENet");
        }

        clientLobby_ = enet_host_create(nullptr, 1, 2, 0, 0);
        if (!clientLobby_) {

            fprintf(stderr, "Cannot create ENet client\n");
        }

        enet_address_set_host(&lobbyAddress_, "localhost");
        lobbyAddress_.port = 11111;

        lobby_ = enet_host_connect(clientLobby_, &lobbyAddress_, 2, 0);
        if (!lobby_) {

            fprintf(stderr, "Cannot connect to lobby");
        }
    }

    ~Network () {

        enet_host_destroy(clientLobby_);
        enet_host_destroy(clientServer_);
        atexit (enet_deinitialize);
    }

    void processInvents () {

        if (connected_) {

            ENetEvent event;
            while (enet_host_service(clientServer_, &event, 1) > 0) {

                switch (event.type) {

                    case ENET_EVENT_TYPE_CONNECT: {

                        printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
                        break;
                    }

                    case ENET_EVENT_TYPE_RECEIVE: {

                        processPacket(event);

                        enet_packet_destroy(event.packet);
                        break;
                    }

                    default:
                        break;
                };
            }
        }
        else {

            ENetEvent event;
            while (enet_host_service(clientLobby_, &event, 1) > 0) {

                switch (event.type) {

                    case ENET_EVENT_TYPE_CONNECT: {

                        printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
                        break;
                    }

                    case ENET_EVENT_TYPE_RECEIVE: {

                        processPacket(event);

                        enet_packet_destroy(event.packet);
                        break;
                    }

                    default:
                        break;
                };
            }
        }
    }

    void sendInfo () {

        if (!connected_) return;

        static uint32_t timeStart = enet_time_get();
        static uint32_t elapsed = 0;

        uint32_t dt = enet_time_get() - timeStart;
        elapsed += dt;
        if (elapsed > 100) {

            elapsed = 0;

            size_t msgSize = 0;
            NetworkMessage *msg = createPlayerPositionMsg(clientData_.localPlayer.id, msgSize);

            ENetPacket *packet = enet_packet_create(msg, msgSize, 0);
            enet_peer_send(server_, 1, packet);

            free(msg);
        }
    }

    NetworkMessage* createPlayerPositionMsg (size_t id, size_t &msgSize) {

        msgSize = sizeof(NetworkMessage) + sizeof(PlayerPosition);
        NetworkMessage *msg = reinterpret_cast<NetworkMessage*>(calloc(1, msgSize));

        msg->type = NetworkMessageType::PlayerPosition;

        PlayerPosition *payload = reinterpret_cast<PlayerPosition*>(msg->payload);
        payload->id = id;
        payload->x = clientData_.localPlayer.posx;
        payload->y = clientData_.localPlayer.posy;

        return msg;
    }

    void processPacket (const ENetEvent &event) {

        NetworkMessage *msg = reinterpret_cast<NetworkMessage*>(event.packet->data);
    
        switch (msg->type) {

            case NetworkMessageType::ServerAddres: {

                processServerAddresMsg(msg);
                break;
            }
    
            case NetworkMessageType::PlayerIdentifyer: {

                processPlayerIdentifyerMsg(msg);
                break;
            }

            case NetworkMessageType::PlayerSelfIdentifyer: {

                processPlayerSelfIdentifyerMsg(msg);
                break;
            }

            case NetworkMessageType::PlayerPing: {

                processPlayerPingMsg(msg);
                break;
            }

            case NetworkMessageType::PlayerPosition: {

                processPlayerPositionMsg(msg);
                break;
            }
            
            default:
                break;
        }
    }

    void processServerAddresMsg (NetworkMessage *msg) {

        ServerAddres *payload = reinterpret_cast<ServerAddres*>(msg->payload);

        fprintf(stderr, "Server information recieved %s:%u\n", payload->hostName, payload->port);

        clientServer_ = enet_host_create(nullptr, 1, 2, 0, 0);
        if (!clientServer_) {

            fprintf(stderr, "Cannot create ENet client\n");
        }
        
        enet_address_set_host(&serverAddress_, payload->hostName);
        serverAddress_.port = payload->port;

        server_ = enet_host_connect(clientServer_, &serverAddress_, 2, 0);
        if (!server_) {

            fprintf(stderr, "Cannot connect to server\n");
        }
        else {

            connected_ = true;
        }
    }

    void processPlayerIdentifyerMsg (NetworkMessage *msg) {

        PlayerIdentifyer *payload = reinterpret_cast<PlayerIdentifyer*>(msg->payload);
        
        PeerPlayerData player;
        player.nickname = payload->nickname;
        
        clientData_.players[payload->id] = player;

        printf("Recieved information about other player. id: %d, nickname: %s\n", payload->id, payload->nickname);
    }

    void processPlayerSelfIdentifyerMsg (NetworkMessage *msg) {

        PlayerSelfIdentifyer *payload = reinterpret_cast<PlayerSelfIdentifyer*>(msg->payload);
        
        clientData_.localPlayer.id       = payload->id;
        clientData_.localPlayer.nickname = payload->nickname;

        printf("Recieved information about me. id: %d, nickname: %s\n", payload->id, payload->nickname);
    }

    void processPlayerPingMsg (NetworkMessage *msg) {

        PlayerPing *payload = reinterpret_cast<PlayerPing*>(msg->payload);
        
        clientData_.players[payload->id].ping = payload->ping;

        //printf("Recieved information about other player. id: %d, ping: %d\n", payload->id, payload->ping);
    }

    void processPlayerPositionMsg (NetworkMessage *msg) {

        PlayerPosition *payload = reinterpret_cast<PlayerPosition*>(msg->payload);
        
        clientData_.players[payload->id].x = payload->x;
        clientData_.players[payload->id].y = payload->y;

        //printf("Recieved information about other player. id: %d, x: %lg, y: %lg\n", payload->id, payload->x, payload->y);
    }

    void startGame () {

        size_t msgSize = sizeof(NetworkMessage);
        NetworkMessage *msg = reinterpret_cast<NetworkMessage*>(calloc(1, msgSize));

        msg->type = NetworkMessageType::Start;

        ENetPacket *packet = enet_packet_create(msg, msgSize, 0);
        enet_peer_send(lobby_, 0, packet);

        free(msg);
    }

private:

    ClientData &clientData_;

    ENetHost *clientLobby_ = nullptr;
    ENetHost *clientServer_ = nullptr;

    ENetAddress lobbyAddress_ = {};
    ENetPeer *lobby_ = nullptr;

    ENetAddress serverAddress_ = {};
    ENetPeer *server_ = nullptr;

    bool connected_ = false;
};


class Game {

public:

    Game (const LocalPlayerData &data): client_(data), drawer_(client_), network_(client_) {

        client_.localPlayer.posx = 100 + std::rand() % 200;
        client_.localPlayer.posy = 100 + std::rand() % 200;
    }

    void run () {

        while (!WindowShouldClose()) {

            const float dt = GetFrameTime();

            network_.processInvents();

            update(dt);

            network_.sendInfo();

            drawer_.drawFrame();
        }
    }

private:

    void update (float dt) {

        if (IsKeyDown(KEY_ENTER)) {

            network_.startGame();
        }

        bool left = IsKeyDown(KEY_LEFT);
        bool right = IsKeyDown(KEY_RIGHT);
        bool up = IsKeyDown(KEY_UP);
        bool down = IsKeyDown(KEY_DOWN);
        constexpr float accel = 30.f;
        client_.localPlayer.velx += ((left ? -1.f : 0.f) + (right ? 1.f : 0.f)) * dt * accel;
        client_.localPlayer.vely += ((up ? -1.f : 0.f) + (down ? 1.f : 0.f)) * dt * accel;
        client_.localPlayer.posx += client_.localPlayer.velx * dt;
        client_.localPlayer.posy += client_.localPlayer.vely * dt;
        client_.localPlayer.velx *= 0.99f;
        client_.localPlayer.vely *= 0.99f;
    }

private:

    ClientData client_;
    Drawer drawer_;
    Network network_;
};


int main(int argc, const char **argv) {

    std::srand(std::time(nullptr));

    Game game(LocalPlayerData(0.f, 0.f));

    game.run();  
}
