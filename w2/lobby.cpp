#include <enet/enet.h>
#include <cstring>

#include "NetworkUtils.hpp"


class Lobby {

public:

    Lobby () {

        if (enet_initialize() != 0) {

            fprintf(stderr, "Cannot init ENet");
        }

        lobbyAddress_.host = ENET_HOST_ANY;
        lobbyAddress_.port = 11111;

        lobby_ = enet_host_create(&lobbyAddress_, 32, 2, 0, 0);
        if (!lobby_) {

            fprintf(stderr, "Cannot create ENet lobby\n");
        }
    }

    ~Lobby () {

        enet_host_destroy(lobby_);
        atexit (enet_deinitialize);
    }

    void processInvents () {

        ENetEvent event;
        while (enet_host_service(lobby_, &event, 1) > 0) {

            switch (event.type) {

                case ENET_EVENT_TYPE_CONNECT: {

                    printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);

                    if (gameStarted) {

                        hotJoin(event.peer);
                    }

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

    void hotJoin (ENetPeer *peer) {

        size_t msgSize = 0;
        NetworkMessage *msg = createServerAddresMsg(msgSize);

        ENetPacket *packet = enet_packet_create(msg, msgSize, 0);
        enet_peer_send(peer, 0, packet);

        free(msg);
    }

    void processPacket (const ENetEvent &event) {

        NetworkMessage *msg = reinterpret_cast<NetworkMessage*>(event.packet->data);
    
        switch (msg->type) {
    
            case NetworkMessageType::Start: {
                
                onStartMsg(msg);
                break;
            }
            
            default:
                break;
        }
    }

    void onStartMsg (NetworkMessage *msgg) {

        size_t msgSize = 0;
        NetworkMessage *msg = createServerAddresMsg(msgSize);

        ENetPacket *packet = enet_packet_create(msg, msgSize, 0);
        enet_host_broadcast(lobby_, 0, packet);

        free(msg);

        gameStarted = true;
    }

    NetworkMessage* createServerAddresMsg (size_t &msgSize) {

        msgSize = sizeof(NetworkMessage) + sizeof(ServerAddres);
        NetworkMessage *msg = reinterpret_cast<NetworkMessage*>(calloc(1, msgSize));

        msg->type = NetworkMessageType::ServerAddres;

        ServerAddres *payload = reinterpret_cast<ServerAddres*>(msg->payload);
        std::strncpy(payload->hostName, hostName_.c_str(), MAX_NICKNAME_LENGTH);
        payload->port = hostPort_;

        return msg;
    }

private:

    ENetHost *lobby_ = nullptr;
    ENetAddress lobbyAddress_ = {};

    const std::string hostName_ = "localhost";
    const uint16_t hostPort_ = 12345;

    bool gameStarted = false;
};


int main () {

    Lobby lobby;

    while (true) {

        lobby.processInvents();
    }
}