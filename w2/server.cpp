#include <enet/enet.h>
#include <iostream>
#include <map>
#include <vector>
#include <cstring>

#include "NetworkUtils.hpp"


struct PlayerData {

    char nickname[MAX_NICKNAME_LENGTH] = {};

    float x = 0.f, y = 0.f;

    ENetPeer *peer = nullptr;
};

static const std::vector<std::string> nicknames {

    "KOT",
    "kmun",
    "Eron_White",
    "NULL_IQ",
    "Vounder_VVV",
    "Generalus",
    "The_Eclipse",
};


class Server {

  public:
  
    Server () {

        if (enet_initialize() != 0) {

            fprintf(stderr, "Cannot init ENet");
        }

        serverAddress_.host = ENET_HOST_ANY;
        serverAddress_.port = 12345;

        server_ = enet_host_create(&serverAddress_, 32, 2, 0, 0);
        if (!server_) {

            fprintf(stderr, "Cannot create ENet server\n");
        }
    }

    ~Server () {

        enet_host_destroy(server_);
        atexit (enet_deinitialize);
    }

    void processInvents () {

        ENetEvent event;
        while (enet_host_service(server_, &event, 1) > 0) {

            switch (event.type) {

                case ENET_EVENT_TYPE_CONNECT: {

                    onConnection(event);
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

    void onConnection (const ENetEvent &event) {

        printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);

        size_t id = addNewPlayer(event.peer);
        
        notifyEveryoneAboutPeer(id);
        notifyPeerAboutOther(id);
        nofifyPeerAboutThemselves(id);
    }

    void onPlayerPositionMsg (NetworkMessage *msg) {

        PlayerPosition *payload = reinterpret_cast<PlayerPosition*>(msg->payload);

        players_[payload->id].x = payload->x;
        players_[payload->id].y = payload->y;

        printf("%s's position recieved\n", players_[payload->id].nickname);
    }

    void notifyEveryoneAboutPeer (size_t id) {

        size_t msgSize = 0;
        NetworkMessage *msg = createPlayerIdentifyerMsg(id, msgSize);

        ENetPacket *packet = enet_packet_create(msg, msgSize, ENET_PACKET_FLAG_RELIABLE);
        enet_host_broadcast(server_, 0, packet);

        printf("Information about new player sent to everyone\n");

        free(msg);
    }

    void notifyPeerAboutOther (size_t peerID) {

        for (auto &[id, playerData]: players_) {

            if (id != peerID) {

                size_t msgSize = 0;
                NetworkMessage *msg = createPlayerIdentifyerMsg(id, msgSize);

                ENetPacket *packet = enet_packet_create(msg, msgSize, ENET_PACKET_FLAG_RELIABLE);
                enet_peer_send(players_[peerID].peer, 0, packet);

                free(msg);
            }
        }

        printf("Information about other players sent to the new just connected one\n");
    }

    void nofifyPeerAboutThemselves (size_t peerID) {

        size_t msgSize = 0;
        NetworkMessage *msg = createPlayerSelfIdentifyerMsg(peerID, msgSize);

        ENetPacket *packet = enet_packet_create(msg, msgSize, ENET_PACKET_FLAG_RELIABLE);
        enet_peer_send(players_[peerID].peer, 0, packet);

        printf("Just connected player was informated about their id and nickname\n");

        free(msg);
    }  

    void processPacket (const ENetEvent &event) {

        NetworkMessage *msg = reinterpret_cast<NetworkMessage*>(event.packet->data);
    
        switch (msg->type) {
    
            case NetworkMessageType::PlayerPosition: {
                
                onPlayerPositionMsg(msg);
                break;
            }
            
            default:
                break;
        }
    }

    size_t addNewPlayer (ENetPeer *peer) {

        static size_t id = 0;

        PlayerData newPlayer;
        newPlayer.peer = peer;
        std::strncpy(newPlayer.nickname, nicknames[id % nicknames.size()].c_str(), MAX_NICKNAME_LENGTH - 1);

        players_[id] = newPlayer;

        printf("Just connected player got id: %d, nickname: %s\n", id, players_[id].nickname);

        return id++;
    }

    NetworkMessage* createPlayerIdentifyerMsg (size_t id, size_t &msgSize) {

        msgSize = sizeof(NetworkMessage) + sizeof(PlayerIdentifyer);
        NetworkMessage *msg = reinterpret_cast<NetworkMessage*>(calloc(1, msgSize));

        msg->type = NetworkMessageType::PlayerIdentifyer;

        PlayerIdentifyer *payload = reinterpret_cast<PlayerIdentifyer*>(msg->payload);
        payload->id = id;
        std::strncpy(payload->nickname, players_[id].nickname, MAX_NICKNAME_LENGTH);

        return msg;
    }

    NetworkMessage* createPlayerSelfIdentifyerMsg (size_t id, size_t &msgSize) {

        msgSize = sizeof(NetworkMessage) + sizeof(PlayerSelfIdentifyer);
        NetworkMessage *msg = reinterpret_cast<NetworkMessage*>(calloc(1, msgSize));

        msg->type = NetworkMessageType::PlayerSelfIdentifyer;

        PlayerSelfIdentifyer *payload = reinterpret_cast<PlayerSelfIdentifyer*>(msg->payload);
        payload->id = id;
        std::strncpy(payload->nickname, players_[id].nickname, MAX_NICKNAME_LENGTH);

        return msg;
    }

    NetworkMessage* createPlayerPingMsg (size_t id, size_t &msgSize) {

        msgSize = sizeof(NetworkMessage) + sizeof(PlayerPing);
        NetworkMessage *msg = reinterpret_cast<NetworkMessage*>(calloc(1, msgSize));

        msg->type = NetworkMessageType::PlayerPing;

        PlayerPing *payload = reinterpret_cast<PlayerPing*>(msg->payload);
        payload->id = id;
        payload->ping = players_[id].peer->roundTripTime;

        return msg;
    }

    NetworkMessage* createPlayerPositionMsg (size_t id, size_t &msgSize) {

        msgSize = sizeof(NetworkMessage) + sizeof(PlayerPosition);
        NetworkMessage *msg = reinterpret_cast<NetworkMessage*>(calloc(1, msgSize));

        msg->type = NetworkMessageType::PlayerPosition;

        PlayerPosition *payload = reinterpret_cast<PlayerPosition*>(msg->payload);
        payload->id = id;
        payload->x = players_[id].x;
        payload->y = players_[id].y;

        return msg;
    }

    void sendPingToEveryone () {

        for (auto &[id, PlayerData]: players_) {

            size_t msgSize = 0;
            NetworkMessage *msg = createPlayerPingMsg(id, msgSize);

            ENetPacket *packet = enet_packet_create(msg, msgSize, 0);
            enet_host_broadcast(server_, 1, packet);

            free(msg);
        }
    }

    void sendPositionsToEveryone () {

        for (auto &[id, PlayerData]: players_) {

            size_t msgSize = 0;
            NetworkMessage *msg = createPlayerPositionMsg(id, msgSize);

            ENetPacket *packet = enet_packet_create(msg, msgSize, 0);
            enet_host_broadcast(server_, 1, packet);

            free(msg);
        }
    }

    void sendInfo () {

        sendPingToEveryone();
        sendPositionsToEveryone();
    }

private:

    ENetHost *server_ = nullptr;
    ENetAddress serverAddress_ = {};

    std::map <size_t, PlayerData> players_;
};


int main(int argc, const char **argv) {

    Server server;

    while (true) {

        server.processInvents();
        server.sendInfo();
    }
}

