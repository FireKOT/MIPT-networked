#include "protocol.h"
#include <cstring>
#include "bitstream.hpp"


void sendPacket (ENetPeer *peer, const Bitstream &bs, enet_uint8 channalID = 0, enet_uint32 packetFlags = ENET_PACKET_FLAG_RELIABLE) {

    ENetPacket *packet = enet_packet_create(bs.data(), bs.size(), packetFlags);
    enet_peer_send(peer, channalID, packet);
}


void send_join(ENetPeer *peer)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t), ENET_PACKET_FLAG_RELIABLE);
  *packet->data = E_CLIENT_TO_SERVER_JOIN;

  enet_peer_send(peer, 0, packet);
}

void send_new_entity(ENetPeer *peer, const Entity &ent)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t) + sizeof(Entity),
                                                   ENET_PACKET_FLAG_RELIABLE);
  uint8_t *ptr = packet->data;
  *ptr = E_SERVER_TO_CLIENT_NEW_ENTITY; ptr += sizeof(uint8_t);
  memcpy(ptr, &ent, sizeof(Entity)); ptr += sizeof(Entity);

  enet_peer_send(peer, 0, packet);
}

void send_set_controlled_entity(ENetPeer *peer, uint16_t eid)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t) + sizeof(uint16_t),
                                                   ENET_PACKET_FLAG_RELIABLE);
  uint8_t *ptr = packet->data;
  *ptr = E_SERVER_TO_CLIENT_SET_CONTROLLED_ENTITY; ptr += sizeof(uint8_t);
  memcpy(ptr, &eid, sizeof(uint16_t)); ptr += sizeof(uint16_t);

  enet_peer_send(peer, 0, packet);
}

void send_entity_input(ENetPeer *peer, uint16_t eid, float thr, float steer)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t) + sizeof(uint16_t) +
                                                   2 * sizeof(float),
                                                   ENET_PACKET_FLAG_UNSEQUENCED);
  uint8_t *ptr = packet->data;
  *ptr = E_CLIENT_TO_SERVER_INPUT; ptr += sizeof(uint8_t);
  memcpy(ptr, &eid, sizeof(uint16_t)); ptr += sizeof(uint16_t);
  memcpy(ptr, &thr, sizeof(float)); ptr += sizeof(float);
  memcpy(ptr, &steer, sizeof(float)); ptr += sizeof(float);

  enet_peer_send(peer, 1, packet);
}

void send_snapshot(ENetPeer *peer, uint16_t eid, float x, float y, float ori, uint64_t tick, float omega, float vx, float vy) {

    Bitstream bs;
    bs.write(E_SERVER_TO_CLIENT_SNAPSHOT);
    bs.write(eid);
    bs.write(x);
    bs.write(y);
    bs.write(ori);
    bs.write(tick);
    bs.write(omega);
    bs.write(vx);
    bs.write(vy);

    sendPacket(peer, bs, 1, ENET_PACKET_FLAG_UNSEQUENCED);
}

void send_time_msec(ENetPeer *peer, uint32_t timeMsec)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t) + sizeof(uint32_t),
                                                   ENET_PACKET_FLAG_RELIABLE);
  uint8_t *ptr = packet->data;
  *ptr = E_SERVER_TO_CLIENT_TIME_MSEC; ptr += sizeof(uint8_t);
  memcpy(ptr, &timeMsec, sizeof(uint32_t)); ptr += sizeof(uint32_t);

  enet_peer_send(peer, 0, packet);
}

MessageType get_packet_type(ENetPacket *packet)
{
  return (MessageType)*packet->data;
}

void deserialize_new_entity(ENetPacket *packet, Entity &ent)
{
  uint8_t *ptr = packet->data; ptr += sizeof(uint8_t);
  ent = *(Entity*)(ptr); ptr += sizeof(Entity);
}

void deserialize_set_controlled_entity(ENetPacket *packet, uint16_t &eid)
{
  uint8_t *ptr = packet->data; ptr += sizeof(uint8_t);
  eid = *(uint16_t*)(ptr); ptr += sizeof(uint16_t);
}

void deserialize_entity_input(ENetPacket *packet, uint16_t &eid, float &thr, float &steer)
{
  uint8_t *ptr = packet->data; ptr += sizeof(uint8_t);
  eid = *(uint16_t*)(ptr); ptr += sizeof(uint16_t);
  thr = *(float*)(ptr); ptr += sizeof(float);
  steer = *(float*)(ptr); ptr += sizeof(float);
}

void deserialize_snapshot(ENetPacket *packet, uint16_t &eid, float &x, float &y, float &ori, uint64_t &tick, float &omega, float &vx, float &vy) {

    Bitstream bs(packet->data, packet->dataLength);
    bs.skip<uint8_t>();

    bs.read(eid);
    bs.read(x);
    bs.read(y);
    bs.read(ori);
    bs.read(tick);
    bs.read(omega);
    bs.read(vx);
    bs.read(vy);
}

void deserialize_time_msec(ENetPacket *packet, uint32_t &timeMsec)
{
  uint8_t *ptr = packet->data; ptr += sizeof(uint8_t);
  timeMsec = *(uint32_t*)(ptr); ptr += sizeof(uint32_t);
}

