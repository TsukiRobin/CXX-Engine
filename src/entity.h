#pragma once
#include <cstdint>
#include <cassert>

enum Behaviour : uint32_t {
  NONE = 0, // 1
  CAN_MOVE = 1 << 0, // 2
  IS_PLAYER = 1 << 1, // 4
  RESPOND_TO_INPUT = 1 << 2,
  IGNORE_GRAVITY = 1 << 3
};


enum class ID : uint8_t {
  NONE = 0,
  GROUND = 3,
  PLAYER = 4,
  WALL = 5,
};

// Vilken animation spelaren visar just nu. Väljs varje frame i Update().
enum class PlayerAnim : uint8_t {
  Idle = 0,
  Run,
  Jump,
  WallGlide,
  Dash,
};

struct Position{
  int x;
  int y;
};

struct Entity {
  ID  id;
  float x;
  float y;
  float vx;
  float vy;
  bool on_floor = false;
  Behaviour behaviour;
  int jumps_left;
  float dash_timer;
  bool facing_left;

  int current_frame = 0;
  float anim_timer = 0.0f;
  PlayerAnim anim = PlayerAnim::Idle;
  bool wall_on_right = false; // vilken sida väggen var på vid senaste väggkontakt


  bool HasBehaviour(Behaviour flags){
    return (behaviour & flags) == flags;
  }

  void SetBehaviour(Behaviour flags){
    behaviour = flags;
  }

  void AddBehaviour(Behaviour flags){
    behaviour = (Behaviour)(behaviour | flags);
  }

  void RemoveBehaviour(Behaviour flags){
    behaviour = (Behaviour)(behaviour & ~flags);
  }

  void InitializeBaseBehaviour(){
    assert(id != ID::NONE);
    switch(id) {
      default:
        SetBehaviour(NONE);
        break;
      case ID::PLAYER:
        SetBehaviour((Behaviour)(CAN_MOVE | IS_PLAYER | RESPOND_TO_INPUT ));
        jumps_left = 2;
        break;
          

    }
  }
};
