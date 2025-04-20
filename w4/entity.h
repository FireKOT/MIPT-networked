#pragma once
#include <cstdint>

constexpr uint16_t invalid_entity = -1;
struct Entity {

  uint16_t eid = invalid_entity;

  float x = 0.f;
  float y = 0.f;
  float r = 1.f;
  
  uint32_t color = 0xff00ffff;
  int32_t points = 0;

  bool serverControlled = false;
  float targetX = 0.f;
  float targetY = 0.f;
};

