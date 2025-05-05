#include <enet/enet.h>
#include <iostream>
#include "entity.h"
#include "protocol.h"
#include "mathUtils.h"
#include <stdlib.h>
#include <vector>
#include <deque>
#include <map>


struct snapshot {

    float x = 0.f, vx = 0.f, y = 0.f, vy = 0.f, ori = 0.f, omega = 0.f;
    uint16_t packetID = 0;
};


struct ServerEntity {

    Entity entity;

    snapshot lastSended;
    std::deque <snapshot> states;
};


static std::vector<ServerEntity> entities;
static std::map<uint16_t, ENetPeer*> controlledMap;


LODTypes getLODTypeByDistance (float distance) {

    float LOD0Dist = 30.f * 30.f, LOD1Dist = 60.f * 60.f, LOD2Dist = 90.f * 90.f;

    if (distance < LOD0Dist) {

        return LODTypes::LOD0;
    }
    else if (distance < LOD1Dist) {

        return LODTypes::LOD1;
    }
    else if (distance < LOD2Dist) {

        return LODTypes::LOD2;
    }

    return LODTypes::LOD3;
}

bool isDeltaEq (float lhs, float rhs, float delta) {

    return lhs - rhs <= delta && lhs - rhs >= -delta;
}

bool isLastSendedDataOutdated (const ServerEntity &ent, LODTypes LODType) {

    float xGapSize = 0.f, yGapSize = 0.f, oriGapSize = 0.f;

    switch (LODType) {

        case LODTypes::LOD0: {

            xGapSize = 1.f;
            yGapSize = 1.f;
            oriGapSize = 0.1f;
            break;
        }

        case LODTypes::LOD1: {

            xGapSize = 3.f;
            yGapSize = 3.f;
            oriGapSize = 0.3f;
            break;
        }

        case LODTypes::LOD2: {

            xGapSize = 5.f;
            yGapSize = 5.f;
            oriGapSize = 0.5f;
            break;
        }

        case LODTypes::LOD3: {

            xGapSize = 7.f;
            yGapSize = 7.f;
            oriGapSize = 0.7f;
            break;
        }

        default:
            break;
    }

    return !isDeltaEq(ent.entity.x, ent.lastSended.x, xGapSize) || !isDeltaEq(ent.entity.y, ent.lastSended.y, yGapSize) ||\
           !isDeltaEq(ent.entity.ori, ent.lastSended.ori, oriGapSize);
}

void updateLastSendedInfo (ServerEntity &ent, uint16_t packedID) {

    fprintf(stderr, "ent: %d rcv pID: %d\n", ent.entity.eid, packedID);

    if (ent.states.empty()) {

        fprintf(stderr, "Something went wrong, snapshots are empty\n");
        return;
    }

    if (ent.states.front().packetID > packedID) {

        //we revieved outdated ack
        return;
    }

    if (ent.states.back().packetID < packedID) {

        fprintf(stderr, "Back to the future! Something went wrong, client sent invalid packedID\n");
        return;
    }

    while (ent.states.front().packetID < packedID) {

        fprintf(stderr, "deleted pID: %d\n", ent.states.front().packetID);

        ent.states.pop_front();
    }

    if (ent.states.front().packetID != packedID) {

        fprintf(stderr, "Something went wrong, we somehow lost the snapshot \n");
        return;
    }

    ent.lastSended = ent.states.front();
}

void on_join(ENetPacket *packet, ENetPeer *peer, ENetHost *host)
{
  // send all entities
  for (const ServerEntity &ent : entities)
    send_new_entity(peer, ent.entity);

  // find max eid
  uint16_t maxEid = entities.empty() ? invalid_entity : entities[0].entity.eid;
  for (const ServerEntity &e : entities)
    maxEid = std::max(maxEid, e.entity.eid);
  uint16_t newEid = maxEid + 1;
  uint32_t color = 0x000000ff +
                   0x44000000 * (rand() % 4 + 1) +
                   0x00440000 * (rand() % 4 + 1) +
                   0x00004400 * (rand() % 4 + 1);
  float x = (rand() % 4) * 5.f;
  float y = (rand() % 4) * 5.f;
  Entity ent = {color, false, x, y, 0.f, (rand() / RAND_MAX) * 3.141592654f, 0.f, 0.f, 0.f, 0.f, newEid};
  entities.push_back({ent});

  controlledMap[newEid] = peer;


  // send info about new entity to everyone
  for (size_t i = 0; i < host->peerCount; ++i)
    send_new_entity(&host->peers[i], ent);
  // send info about controlled entity
  send_set_controlled_entity(peer, newEid);
}

void create_server_entity(ENetHost *host)
{
  // find max eid
  uint16_t maxEid = entities.empty() ? invalid_entity : entities[0].entity.eid;
  for (const ServerEntity &e : entities)
    maxEid = std::max(maxEid, e.entity.eid);
  uint16_t newEid = maxEid + 1;
  uint32_t color = 0xff000000 +
                   0x00440000 * (rand() % 5) +
                   0x00004400 * (rand() % 5) +
                   0x00000044 * (rand() % 5);
  float x = rand() % int(worldSize * 2) - worldSize;
  float y = rand() % int(worldSize * 2) - worldSize;
  Entity ent = {color, true, x, y, 0.f, (rand() / RAND_MAX) * 3.141592654f, 0.f, 0.f, 0.f, 0.f, newEid};
  entities.push_back({ent});

  // send info about new entity to everyone
  for (size_t i = 0; i < host->peerCount; ++i)
    send_new_entity(&host->peers[i], ent);
}


void on_input(ENetPacket *packet)
{
  uint16_t eid = invalid_entity;
  float thr = 0.f; float steer = 0.f;
  deserialize_entity_input(packet, eid, thr, steer);
  for (ServerEntity &e : entities)
    if (e.entity.eid == eid)
    {
      e.entity.thr = thr;
      e.entity.steer = steer;
    }
}

void onAck (ENetPacket *packet) {

    uint16_t eid = invalid_entity, packetID = 0;
    deserializeSnapshotAck(packet, eid, packetID);
    for (ServerEntity &e : entities) {

        if (e.entity.eid == eid) {
            
            updateLastSendedInfo(e, packetID);
        }
    }
}

static void update_net(ENetHost* server)
{
  ENetEvent event;
  while (enet_host_service(server, &event, 0) > 0)
  {
    switch (event.type)
    {
    case ENET_EVENT_TYPE_CONNECT:
      printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
      break;
    case ENET_EVENT_TYPE_RECEIVE:
      switch (get_packet_type(event.packet))
      {
        case E_CLIENT_TO_SERVER_JOIN:
          on_join(event.packet, event.peer, server);
          break;
        case E_CLIENT_TO_SERVER_INPUT:
          on_input(event.packet);
          break;
        case E_CLIENT_TO_SERVER_ACK:
          onAck(event.packet);
          break;
      };
      enet_packet_destroy(event.packet);
      break;
    default:
      break;
    };
  }
}

static void update_ai(Entity& e, float dt)
{
  // small random chance to enable or disable throttle
  if (rand() % 100 == 0)
    e.thr = e.thr > 0.f ? 0.f : 1.f;
  // small random chance to enable or disable steering
  if (rand() % 10 == 0)
    e.steer = e.steer != 0.f ? 0.f : ((rand() % 2) * 2.f - 1.f);
}

static void simulate_world(ENetHost* server, float dt)
{
  for (ServerEntity &e : entities)
  {
    if (e.entity.serverControlled)
      update_ai(e.entity, dt);
    // simulate
    simulate_entity(e.entity, dt);
    // send
    for (ServerEntity &other : entities)
    {
      if (!controlledMap.contains(other.entity.eid)) continue;
    
      ENetPeer *peer = controlledMap[other.entity.eid];
      
      float distance = (e.entity.x - other.entity.x) * (e.entity.x - other.entity.x) + (e.entity.y - other.entity.y) * (e.entity.y - other.entity.y);
      LODTypes curLODType = getLODTypeByDistance(distance);
      if (isLastSendedDataOutdated(e, curLODType)) {

        uint16_t packetID = 0;
        if (!e.states.empty()) {

            packetID = e.states.back().packetID + 1;
        }
        snapshot sh = {

            .x     = e.entity.x,
            .vx    = e.entity.vx,
            .y     = e.entity.y,
            .vy    = e.entity.vy,
            .ori   = e.entity.ori,
            .omega = e.entity.omega,

            .packetID = packetID,
        };
        e.states.emplace_back(sh);
        send_snapshot(peer, e.entity.eid, e.entity.x, e.entity.vx, e.entity.y, e.entity.vy, e.entity.ori, e.entity.omega, packetID, curLODType);
      }
    }
  }
}

static void update_time(ENetHost* server, uint32_t curTime)
{
  // We can send it less often too
  for (size_t i = 0; i < server->peerCount; ++i)
    send_time_msec(&server->peers[i], curTime);
}

int main(int argc, const char **argv)
{
  if (enet_initialize() != 0)
  {
    printf("Cannot init ENet");
    return 1;
  }
  ENetAddress address;

  address.host = ENET_HOST_ANY;
  address.port = 10131;

  ENetHost *server = enet_host_create(&address, 32, 2, 0, 0);

  if (!server)
  {
    printf("Cannot create ENet server\n");
    return 1;
  }

  constexpr size_t numShips = 100;
  for (size_t i = 0; i < numShips; ++i)
    create_server_entity(server);

  uint32_t lastTime = enet_time_get();
  while (true)
  {
    uint32_t curTime = enet_time_get();
    float dt = (curTime - lastTime) * 0.001f;
    lastTime = curTime;

    update_net(server);
    simulate_world(server, dt);
    update_time(server, curTime);
    usleep(10000);
  }

  enet_host_destroy(server);

  atexit(enet_deinitialize);
  return 0;
}


