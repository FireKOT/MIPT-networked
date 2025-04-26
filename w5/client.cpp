#include <functional>
#include "raylib.h"
#include <enet/enet.h>
#include <math.h>
#include <iostream>
#include <deque>

#include <vector>
#include "entity.h"
#include "protocol.h"
#include "PhysConsts.hpp"


constexpr uint8_t DISPLAY_DELAY = 200;   //ms


struct EntityState {

    Entity state = {};
    uint64_t tick = 0;
};


class ClientEntity {

public:

    explicit ClientEntity (const Entity &ent): entity_(ent) {}

    Entity getDisplayEntity (uint64_t curTime) {

        if (states_.empty()) return entity_;

        Entity lerpEntity = {};
        uint64_t displayTime = curTime - DISPLAY_DELAY;

        if (states_.size() == 1 || \
            states_.back().tick * PHYS_TICK_TIME + DISPLAY_DELAY <= displayTime) {

            lerpEntity = states_.back().state;
        }
        else if (states_.front().tick * PHYS_TICK_TIME >= displayTime) {

            lerpEntity = states_.front().state;
        }
        else {

            uint64_t id = 1;
            while (states_[id].tick * PHYS_TICK_TIME < displayTime) {

                ++id;
            }

            uint64_t time1 = states_[id - 1].tick * PHYS_TICK_TIME;
            uint64_t time2 = states_[id    ].tick * PHYS_TICK_TIME;
            float t = static_cast<float>((displayTime - time1)) / (time2 - time1);

            lerpEntity = lerp(states_[id - 1].state, states_[id].state, t);

            for (uint64_t i = 1; i < id; ++i) {

                states_.pop_front();
            }
        }

        return lerpEntity;
    }

    void addNewState (const Entity &ent, uint64_t tick) {

        //assume that we don't recieve outdated info

        if (states_.size() < 2) {

            states_.push_back({ent, tick});
            return;
        } 

        while (states_.back().tick > tick && states_.size() >= 2) {

            states_.pop_back();
        }

        if (states_.back().tick >= tick) {

            states_.back() = {ent, tick};
        }
        else {

            states_.push_back({ent, tick});
        }

        for (int i = 0; i < 12; ++i) {

            entity_ = states_.back().state;
            entity_.thr = thr_;
            entity_.steer = steer_;
            simulate_entity(entity_, PHYS_TICK_TIME * 0.001f);
            states_.push_back({entity_, states_.back().tick + 1});
        }
    }

    void setLastInput (float thr, float steer) {

        thr_ = thr;
        steer_ = steer;
    }

private:

    Entity entity_= {};

    std::deque <EntityState> states_ = {};

    float thr_ = 0.f, steer_ = 0.f;
};


static std::vector<ClientEntity> entities;
static std::unordered_map<uint16_t, size_t> indexMap;
static uint16_t my_entity = invalid_entity;

void on_new_entity_packet(ENetPacket *packet)
{
  Entity newEntity;
  deserialize_new_entity(packet, newEntity);
  auto itf = indexMap.find(newEntity.eid);
  if (itf != indexMap.end())
    return; // don't need to do anything, we already have entity
  indexMap[newEntity.eid] = entities.size();
  entities.push_back(ClientEntity(newEntity));
}

void on_set_controlled_entity(ENetPacket *packet)
{
  deserialize_set_controlled_entity(packet, my_entity);
}

template<typename Callable>
static void get_entity(uint16_t eid, Callable c)
{
  auto itf = indexMap.find(eid);
  if (itf != indexMap.end())
    c(entities[itf->second]);
}

void on_snapshot(ENetPacket *packet)
{
    uint16_t eid = invalid_entity;
    float x = 0.f; float y = 0.f; float ori = 0.f, omega = 0.f;
    float vx = 0.f, vy = 0.f;
    uint64_t tick = 0;
    deserialize_snapshot(packet, eid, x, y, ori, tick, omega, vx, vy);
    get_entity(eid, [&](ClientEntity& clientEntity) {
        
        Entity ent;

        ent.x = x;
        ent.y = y;
        ent.vx = vx;
        ent.vy = vy;
        ent.ori = ori;
        ent.omega = omega;

        clientEntity.addNewState(ent, tick);
    });
}

static void on_time(ENetPacket *packet, ENetPeer* peer)
{
  uint32_t timeMsec;
  deserialize_time_msec(packet, timeMsec);
  enet_time_set(timeMsec + peer->lastRoundTripTime / 2);
}

static void draw_entity(const Entity& e)
{
  const float shipLen = 3.f;
  const float shipWidth = 2.f;
  const Vector2 fwd = Vector2{cosf(e.ori), sinf(e.ori)};
  const Vector2 left = Vector2{-fwd.y, fwd.x};
  DrawTriangle(Vector2{e.x + fwd.x * shipLen * 0.5f, e.y + fwd.y * shipLen * 0.5f},
               Vector2{e.x - fwd.x * shipLen * 0.5f - left.x * shipWidth * 0.5f, e.y - fwd.y * shipLen * 0.5f - left.y * shipWidth * 0.5f},
               Vector2{e.x - fwd.x * shipLen * 0.5f + left.x * shipWidth * 0.5f, e.y - fwd.y * shipLen * 0.5f + left.y * shipWidth * 0.5f},
               GetColor(e.color));
}

static void update_net(ENetHost* client, ENetPeer* serverPeer)
{
  ENetEvent event;
  while (enet_host_service(client, &event, 0) > 0)
  {
    switch (event.type)
    {
    case ENET_EVENT_TYPE_CONNECT:
      printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
      send_join(serverPeer);
      break;
    case ENET_EVENT_TYPE_RECEIVE:
      switch (get_packet_type(event.packet))
      {
      case E_SERVER_TO_CLIENT_NEW_ENTITY:
        on_new_entity_packet(event.packet);
        break;
      case E_SERVER_TO_CLIENT_SET_CONTROLLED_ENTITY:
        on_set_controlled_entity(event.packet);
        break;
      case E_SERVER_TO_CLIENT_SNAPSHOT:
        on_snapshot(event.packet);
        break;
      case E_SERVER_TO_CLIENT_TIME_MSEC:
        on_time(event.packet, event.peer);
        break;
      };
      enet_packet_destroy(event.packet);
      break;
    default:
      break;
    };
  }
}

static void simulate_world(ENetPeer* serverPeer)
{
  if (my_entity != invalid_entity)
  {
    bool left = IsKeyDown(KEY_LEFT);
    bool right = IsKeyDown(KEY_RIGHT);
    bool up = IsKeyDown(KEY_UP);
    bool down = IsKeyDown(KEY_DOWN);
    get_entity(my_entity, [&](ClientEntity& e)
    {
        // Update
        float thr = (up ? 1.f : 0.f) + (down ? -1.f : 0.f);
        float steer = (left ? -1.f : 0.f) + (right ? 1.f : 0.f);
        e.setLastInput(thr, steer);

        // Send
        send_entity_input(serverPeer, my_entity, thr, steer);
    });
  }
}

static void draw_world(const Camera2D& camera)
{
  BeginDrawing();
    ClearBackground(GRAY);
    BeginMode2D(camera);

        for (ClientEntity &clientEntity : entities) {

            Entity ent = clientEntity.getDisplayEntity(enet_time_get());
            draw_entity(ent);
        }

    EndMode2D();
  EndDrawing();
}

int main(int argc, const char **argv)
{
  if (enet_initialize() != 0)
  {
    printf("Cannot init ENet");
    return 1;
  }

  ENetHost *client = enet_host_create(nullptr, 1, 2, 0, 0);
  if (!client)
  {
    printf("Cannot create ENet client\n");
    return 1;
  }

  ENetAddress address;
  enet_address_set_host(&address, "localhost");
  address.port = 10131;

  ENetPeer *serverPeer = enet_host_connect(client, &address, 2, 0);
  if (!serverPeer)
  {
    printf("Cannot connect to server");
    return 1;
  }

  int width = 600;
  int height = 600;

  InitWindow(width, height, "w5 networked MIPT");

  const int scrWidth = GetMonitorWidth(0);
  const int scrHeight = GetMonitorHeight(0);
  if (scrWidth < width || scrHeight < height)
  {
    width = std::min(scrWidth, width);
    height = std::min(scrHeight - 150, height);
    SetWindowSize(width, height);
  }

  Camera2D camera = { {0, 0}, {0, 0}, 0.f, 1.f };
  camera.target = Vector2{ 0.f, 0.f };
  camera.offset = Vector2{ width * 0.5f, height * 0.5f };
  camera.rotation = 0.f;
  camera.zoom = 10.f;

  SetTargetFPS(60);               // Set our game to run at 60 frames-per-second

  while (!WindowShouldClose())
  {
    float dt = GetFrameTime(); // for future use and making it look smooth

    update_net(client, serverPeer);
    simulate_world(serverPeer);
    draw_world(camera);
    //printf("%d\n", enet_time_get());
  }

  CloseWindow();
  return 0;
}
