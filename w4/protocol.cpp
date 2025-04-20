#include "protocol.h"
#include <cstring>

#include "bitstream.hpp"


void sendPacket (ENetPeer *peer, const Bitstream &bs, enet_uint8 channalID = 0, enet_uint32 packetFlags = ENET_PACKET_FLAG_RELIABLE) {

    ENetPacket *packet = enet_packet_create(bs.data(), bs.size(), packetFlags);
    enet_peer_send(peer, channalID, packet);
}


void send_join(ENetPeer *peer) {

    Bitstream bs;
    bs.write(E_CLIENT_TO_SERVER_JOIN);

    sendPacket(peer, bs);
}

void send_new_entity(ENetPeer *peer, const Entity &ent) {

    Bitstream bs;
    bs.write(E_SERVER_TO_CLIENT_NEW_ENTITY);
    bs.write(ent);

    sendPacket(peer, bs);
}

void send_set_controlled_entity(ENetPeer *peer, uint16_t eid) {

    Bitstream bs;
    bs.write(E_SERVER_TO_CLIENT_SET_CONTROLLED_ENTITY);
    bs.write(eid);

    sendPacket(peer, bs);
}

void send_entity_state(ENetPeer *peer, uint16_t eid, float x, float y) {

    Bitstream bs;
    bs.write(E_CLIENT_TO_SERVER_STATE);
    bs.write(eid);
    bs.write(x);
    bs.write(y);

    sendPacket(peer, bs, 1, ENET_PACKET_FLAG_UNSEQUENCED);
}

void send_snapshot(ENetPeer *peer, uint16_t eid, float x, float y, float r, int32_t points) {

    Bitstream bs;
    bs.write(E_SERVER_TO_CLIENT_SNAPSHOT);
    bs.write(eid);
    bs.write(x);
    bs.write(y);
    bs.write(r);
    bs.write(points);

    sendPacket(peer, bs, 1, ENET_PACKET_FLAG_UNSEQUENCED);
}

MessageType get_packet_type(ENetPacket *packet) {

    return (MessageType)*packet->data;
}

void deserialize_new_entity(ENetPacket *packet, Entity &ent) {

    Bitstream bs(packet->data, packet->dataLength);
    bs.skip<uint8_t>();

    bs.read(ent);
}

void deserialize_set_controlled_entity(ENetPacket *packet, uint16_t &eid) {

    Bitstream bs(packet->data, packet->dataLength);
    bs.skip<uint8_t>();

    bs.read(eid);
}

void deserialize_entity_state(ENetPacket *packet, uint16_t &eid, float &x, float &y) {

    Bitstream bs(packet->data, packet->dataLength);
    bs.skip<uint8_t>();

    bs.read(eid);
    bs.read(x);
    bs.read(y);
}

void deserialize_snapshot(ENetPacket *packet, uint16_t &eid, float &x, float &y, float &r, int32_t &points) {

    Bitstream bs(packet->data, packet->dataLength);
    bs.skip<uint8_t>();

    bs.read(eid);
    bs.read(x);
    bs.read(y);
    bs.read(r);
    bs.read(points);
}

