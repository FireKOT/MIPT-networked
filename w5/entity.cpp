#include "entity.h"
#include "mathUtils.h"
#include <cmath>

constexpr float worldSize = 30.f;

float tile_val(float val, float border)
{
  if (val < -border)
    return val + 2.f * border;
  else if (val > border)
    return val - 2.f * border;
  return val;
}

void simulate_entity(Entity &e, float dt)
{
  bool isBraking = sign(e.thr) < 0.f;
  float accel = isBraking ? 6.f : 1.5f;
  float va = clamp(e.thr, -0.3, 1.f) * accel;
  e.vx += cosf(e.ori) * va * dt;
  e.vy += sinf(e.ori) * va * dt;
  e.omega += e.steer * dt * 0.3f;
  e.ori += e.omega * dt;
  e.x += e.vx * dt;
  e.y += e.vy * dt;

  e.x = tile_val(e.x, worldSize);
  e.y = tile_val(e.y, worldSize);
}

Entity lerp (const Entity &ent1, const Entity &ent2, float t) {

    Entity lerpEntity = ent1;
    lerpEntity.x     = std::lerp(ent1.x,     ent2.x,     t);
    lerpEntity.y     = std::lerp(ent1.y,     ent2.y,     t);
    lerpEntity.vx    = std::lerp(ent1.vx,    ent2.vx,    t);
    lerpEntity.vy    = std::lerp(ent1.vy,    ent2.vy,    t);
    lerpEntity.ori   = std::lerp(ent1.ori,   ent2.ori,   t);
    lerpEntity.omega = std::lerp(ent1.omega, ent2.omega, t);

    return lerpEntity;
}
