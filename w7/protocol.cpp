#include "protocol.h"
#include "quantisation.h"
#include <cstring> // memcpy
#include <iostream>
#include "bitstream.hpp"


namespace {


template <LODTypes T>
void QuantPackSnapShot (Bitstream &bs, uint16_t eid, float x, float vx, float y, float vy, float ori, float omega, uint16_t packetID) {

    QuantPackSnapShot<LODTypes::LOD0>(bs, eid, x, vx, y, vy, ori, omega, packetID);
}

template <>
void QuantPackSnapShot <LODTypes::LOD0> (Bitstream &bs, uint16_t eid, float x, float vx, float y, float vy, float ori, float omega, uint16_t packetID) {

    bs.write(E_SERVER_TO_CLIENT_SNAPSHOT);
    bs.write(LODTypes::LOD0);
    bs.write(eid);
    bs.write(x);
    bs.write(vx);
    bs.write(y);
    bs.write(vy);
    bs.write(ori);
    bs.write(omega);
    bs.write(packetID);
}

template <>
void QuantPackSnapShot <LODTypes::LOD1> (Bitstream &bs, uint16_t eid, float x, float vx, float y, float vy, float ori, float omega, uint16_t packetID) {

    PackedFloat<uint16_t, 16> xPacked(x, -worldSize, worldSize);
    PackedFloat<uint16_t, 16> vxPacked(vx, -10.f, 10.f);
    PackedFloat<uint16_t, 16> yPacked(y, -worldSize, worldSize);
    PackedFloat<uint16_t, 16> vyPacked(vy, -10.f, 10.f);
    PackedFloat<uint8_t, 8> oriPacked(ori, -PI, PI);
    PackedFloat<uint8_t, 8> omegaPacked(omega, -10.f, 10.f);

    bs.write(E_SERVER_TO_CLIENT_SNAPSHOT);
    bs.write(LODTypes::LOD1);
    bs.write(eid);

    bs.write(xPacked.packedVal);
    bs.write(vxPacked.packedVal);
    bs.write(yPacked.packedVal);
    bs.write(vyPacked.packedVal);
    bs.write(oriPacked.packedVal);
    bs.write(omegaPacked.packedVal);

    bs.write(packetID);
}

template <>
void QuantPackSnapShot <LODTypes::LOD2> (Bitstream &bs, uint16_t eid, float x, float vx, float y, float vy, float ori, float omega, uint16_t packetID) {

    PackedFloat<uint16_t, 12> xPacked(x, -worldSize, worldSize);
    PackedFloat<uint16_t, 12> vxPacked(vx, -10.f, 10.f);
    PackedFloat<uint16_t, 12> yPacked(y, -worldSize, worldSize);
    PackedFloat<uint16_t, 12> vyPacked(vy, -10.f, 10.f);
    PackedFloat<uint8_t, 8> oriPacked(ori, -PI, PI);
    PackedFloat<uint8_t, 8> omegaPacked(omega, -10.f, 10.f);

    // //do now write exact bits in order to read fast
    bs.write(E_SERVER_TO_CLIENT_SNAPSHOT);
    bs.write(LODTypes::LOD2);
    bs.write(eid);
    
    bs.write(xPacked.packedVal);
    bs.write(vxPacked.packedVal);
    bs.write(yPacked.packedVal);
    bs.write(vyPacked.packedVal);
    bs.write(oriPacked.packedVal);
    bs.write(omegaPacked.packedVal);

    bs.write(packetID);
}

template <>
void QuantPackSnapShot <LODTypes::LOD3> (Bitstream &bs, uint16_t eid, float x, float vx, float y, float vy, float ori, float omega, uint16_t packetID) {

    PackedFloat<uint8_t, 8> xPacked(x, -worldSize, worldSize);
    PackedFloat<uint8_t, 8> vxPacked(vx, -10.f, 10.f);
    PackedFloat<uint8_t, 8> yPacked(y, -worldSize, worldSize);
    PackedFloat<uint8_t, 8> vyPacked(vy, -10.f, 10.f);
    PackedFloat<uint8_t, 8> oriPacked(ori, -PI, PI);
    PackedFloat<uint8_t, 8> omegaPacked(omega, -10.f, 10.f);

    bs.write(E_SERVER_TO_CLIENT_SNAPSHOT);
    bs.write(LODTypes::LOD3);
    bs.write(eid);
    
    bs.write(xPacked.packedVal);
    bs.write(vxPacked.packedVal);
    bs.write(yPacked.packedVal);
    bs.write(vyPacked.packedVal);
    bs.write(oriPacked.packedVal);
    bs.write(omegaPacked.packedVal);

    bs.write(packetID);
}


template <LODTypes T>
void QuantUnpackSnapShot (Bitstream &bs, uint16_t &eid, float &x, float &vx, float &y, float &vy, float &ori, float &omega, uint16_t &packetID) {

    QuantUnpackSnapShot<LODTypes::LOD0>(bs, eid, x, vx, y, vy, ori, omega, packetID);
}

template <>
void QuantUnpackSnapShot <LODTypes::LOD0> (Bitstream &bs, uint16_t &eid, float &x, float &vx, float &y, float &vy, float &ori, float &omega, uint16_t &packetID) {

    bs.skip<uint8_t>();
    bs.skip<uint8_t>();
    bs.read(eid);
    bs.read(x);
    bs.read(vx);
    bs.read(y);
    bs.read(vy);
    bs.read(ori);
    bs.read(omega);
    bs.read(packetID);
}

template <>
void QuantUnpackSnapShot <LODTypes::LOD1> (Bitstream &bs, uint16_t &eid, float &x, float &vx, float &y, float &vy, float &ori, float &omega, uint16_t &packetID) {

    uint16_t xPacked = 0;
    uint16_t vxPacked = 0;
    uint16_t yPacked = 0;
    uint16_t vyPacked = 0;
    uint8_t oriPacked = 0;
    uint8_t omegaPacked = 0;

    bs.skip<uint8_t>();
    bs.skip<uint8_t>();
    bs.read(eid);
    bs.read(xPacked);

    bs.read(vxPacked);
    bs.read(yPacked);
    bs.read(vyPacked);
    bs.read(oriPacked);
    bs.read(omegaPacked);

    bs.read(packetID);

    PackedFloat<uint16_t, 16> xPackedVal(xPacked);
    PackedFloat<uint16_t, 16> vxPackedVal(vxPacked);
    PackedFloat<uint16_t, 16> yPackedVal(yPacked);
    PackedFloat<uint16_t, 16> vyPackedVal(vyPacked);
    PackedFloat<uint8_t, 8> oriPackedVal(oriPacked);
    PackedFloat<uint8_t, 8> omegaPackedVal(omegaPacked);

    x     = xPackedVal.unpack(-worldSize, worldSize);
    vx    = vxPackedVal.unpack(-10.f, 10.f);
    y     = yPackedVal.unpack(-worldSize, worldSize);
    vy    = vyPackedVal.unpack(-10.f, 10.f);
    ori   = oriPackedVal.unpack(-PI, PI);
    omega = omegaPackedVal.unpack(-10.f, 10.f);
}

template <>
void QuantUnpackSnapShot <LODTypes::LOD2> (Bitstream &bs, uint16_t &eid, float &x, float &vx, float &y, float &vy, float &ori, float &omega, uint16_t &packetID) {

    uint16_t xPacked = 0;
    uint16_t vxPacked = 0;
    uint16_t yPacked = 0;
    uint16_t vyPacked = 0;
    uint8_t oriPacked = 0;
    uint8_t omegaPacked = 0;

    bs.skip<uint8_t>();
    bs.skip<uint8_t>();
    bs.read(eid);
    bs.read(xPacked);

    bs.read(vxPacked);
    bs.read(yPacked);
    bs.read(vyPacked);
    bs.read(oriPacked);
    bs.read(omegaPacked);

    bs.read(packetID);

    PackedFloat<uint16_t, 12> xPackedVal(xPacked);
    PackedFloat<uint16_t, 12> vxPackedVal(vxPacked);
    PackedFloat<uint16_t, 12> yPackedVal(yPacked);
    PackedFloat<uint16_t, 12> vyPackedVal(vyPacked);
    PackedFloat<uint8_t, 8> oriPackedVal(oriPacked);
    PackedFloat<uint8_t, 8> omegaPackedVal(omegaPacked);

    x     = xPackedVal.unpack(-worldSize, worldSize);
    vx    = vxPackedVal.unpack(-10.f, 10.f);
    y     = yPackedVal.unpack(-worldSize, worldSize);
    vy    = vyPackedVal.unpack(-10.f, 10.f);
    ori   = oriPackedVal.unpack(-PI, PI);
    omega = omegaPackedVal.unpack(-10.f, 10.f);
}

template <>
void QuantUnpackSnapShot <LODTypes::LOD3> (Bitstream &bs, uint16_t &eid, float &x, float &vx, float &y, float &vy, float &ori, float &omega, uint16_t &packetID) {

    uint8_t xPacked = 0;
    uint8_t vxPacked = 0;
    uint8_t yPacked = 0;
    uint8_t vyPacked = 0;
    uint8_t oriPacked = 0;
    uint8_t omegaPacked = 0;

    bs.skip<uint8_t>();
    bs.skip<uint8_t>();
    bs.read(eid);
    bs.read(xPacked);

    bs.read(vxPacked);
    bs.read(yPacked);
    bs.read(vyPacked);
    bs.read(oriPacked);
    bs.read(omegaPacked);

    bs.read(packetID);

    PackedFloat<uint8_t, 8> xPackedVal(xPacked);
    PackedFloat<uint8_t, 8> vxPackedVal(vxPacked);
    PackedFloat<uint8_t, 8> yPackedVal(yPacked);
    PackedFloat<uint8_t, 8> vyPackedVal(vyPacked);
    PackedFloat<uint8_t, 8> oriPackedVal(oriPacked);
    PackedFloat<uint8_t, 8> omegaPackedVal(omegaPacked);

    x     = xPackedVal.unpack(-worldSize, worldSize);
    vx    = vxPackedVal.unpack(-10.f, 10.f);
    y     = yPackedVal.unpack(-worldSize, worldSize);
    vy    = vyPackedVal.unpack(-10.f, 10.f);
    ori   = oriPackedVal.unpack(-PI, PI);
    omega = omegaPackedVal.unpack(-10.f, 10.f);
}


};


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
                                                   sizeof(uint8_t),
                                                   ENET_PACKET_FLAG_UNSEQUENCED);
  uint8_t *ptr = packet->data;
  *ptr = E_CLIENT_TO_SERVER_INPUT; ptr += sizeof(uint8_t);
  memcpy(ptr, &eid, sizeof(uint16_t)); ptr += sizeof(uint16_t);
  float4bitsQuantized thrPacked(thr, -1.f, 1.f);
  float4bitsQuantized steerPacked(steer, -1.f, 1.f);
  uint8_t thrSteerPacked = (thrPacked.packedVal << 4) | steerPacked.packedVal;
  memcpy(ptr, &thrSteerPacked, sizeof(uint8_t)); ptr += sizeof(uint8_t);
  /*
  memcpy(ptr, &thrPacked, sizeof(uint8_t)); ptr += sizeof(uint8_t);
  memcpy(ptr, &oriPacked, sizeof(uint8_t)); ptr += sizeof(uint8_t);
  */

  enet_peer_send(peer, 1, packet);
}

typedef PackedFloat<uint16_t, 11> PositionXQuantized;
typedef PackedFloat<uint16_t, 10> PositionYQuantized;

void send_snapshot(ENetPeer *peer, uint16_t eid, float x, float vx, float y, float vy, float ori, float omega, uint16_t packetID, LODTypes type) {

    Bitstream bs;

    switch (type) {

        case LODTypes::LOD0: {

            QuantPackSnapShot<LODTypes::LOD0>(bs, eid, x, vx, y, vy, ori, omega, packetID);
            break;
        }

        case LODTypes::LOD1: {

            QuantPackSnapShot<LODTypes::LOD1>(bs, eid, x, vx, y, vy, ori, omega, packetID);
            break;
        }

        case LODTypes::LOD2: {

            QuantPackSnapShot<LODTypes::LOD2>(bs, eid, x, vx, y, vy, ori, omega, packetID);
            break;
        }

        case LODTypes::LOD3: {

            QuantPackSnapShot<LODTypes::LOD3>(bs, eid, x, vx, y, vy, ori, omega, packetID);
            break;
        }
        
        default:
            break;
    }

    sendPacket(peer, bs, 1, ENET_PACKET_FLAG_UNSEQUENCED);
}

void sendSnapshotAck (ENetPeer *peer, uint16_t eid, uint16_t packetID) {

    Bitstream bs;
    bs.write(E_CLIENT_TO_SERVER_ACK);
    bs.write(eid);
    bs.write(packetID);

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
  uint8_t thrSteerPacked = *(uint8_t*)(ptr); ptr += sizeof(uint8_t);
  /*
  uint8_t thrPacked = *(uint8_t*)(ptr); ptr += sizeof(uint8_t);
  uint8_t oriPacked = *(uint8_t*)(ptr); ptr += sizeof(uint8_t);
  */
  static uint8_t neutralPackedValue = pack_float<uint8_t>(0.f, -1.f, 1.f, 4);
  static uint8_t nominalPackedValue = pack_float<uint8_t>(1.f, 0.f, 1.2f, 4);
  float4bitsQuantized thrPacked(thrSteerPacked >> 4);
  float4bitsQuantized steerPacked(thrSteerPacked & 0x0f);
  thr = thrPacked.packedVal == neutralPackedValue ? 0.f : thrPacked.unpack(-1.f, 1.f);
  steer = steerPacked.packedVal == neutralPackedValue ? 0.f : steerPacked.unpack(-1.f, 1.f);
}

void deserialize_snapshot(ENetPacket *packet, uint16_t &eid, float &x, float &vx, float &y, float &vy, float &ori, float &omega, uint16_t &packetID) {

    Bitstream bs(packet->data, packet->dataLength);

    LODTypes type = *reinterpret_cast<LODTypes*>(packet->data + sizeof(uint8_t)); 
    switch (type) {

        case LODTypes::LOD0: {

            QuantUnpackSnapShot<LODTypes::LOD0>(bs, eid, x, vx, y, vy, ori, omega, packetID);
            break;
        }

        case LODTypes::LOD1: {

            QuantUnpackSnapShot<LODTypes::LOD1>(bs, eid, x, vx, y, vy, ori, omega, packetID);
            break;
        }

        case LODTypes::LOD2: {

            QuantUnpackSnapShot<LODTypes::LOD2>(bs, eid, x, vx, y, vy, ori, omega, packetID);
            break;
        }

        case LODTypes::LOD3: {

            QuantUnpackSnapShot<LODTypes::LOD3>(bs, eid, x, vx, y, vy, ori, omega, packetID);
            break;
        }
        
        default:
            break;
    }
}

void deserialize_time_msec(ENetPacket *packet, uint32_t &timeMsec)
{
  uint8_t *ptr = packet->data; ptr += sizeof(uint8_t);
  timeMsec = *(uint32_t*)(ptr); ptr += sizeof(uint32_t);
}

void deserializeSnapshotAck (ENetPacket *packet, uint16_t &eid, uint16_t &packetID) {

    Bitstream bs(packet->data, packet->dataLength);
    bs.skip<uint8_t>();
    bs.read(eid);
    bs.read(packetID);
}

