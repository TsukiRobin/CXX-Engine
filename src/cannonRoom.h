#pragma once
#include "SDL3/SDL_render.h"

// Kanonrummet: kanoner i sidoväggarna varnar och skjuter kanonkulor
// tvärs över rummet. Plattformarna stoppar kulorna och ger skydd.
// Samla kristaller och överlev så länge som möjligt. En träff nollställer
// rundan. Det blir svårare (tätare och snabbare skott) ju längre du klarar dig.

struct GameData;
struct Entity;

constexpr int MAX_CANNONS = 16;
constexpr int MAX_BALLS = 32;

struct Cannon {
  int cell_x;
  int cell_y;
  bool faces_right;
  float warn_timer; // > 0 betyder att kanonen laddar och snart skjuter
};

struct Cannonball {
  bool alive;
  float x;
  float y;
  float vx;
};

struct CannonRoom {
  bool enabled;
  Cannon cannons[MAX_CANNONS];
  int cannon_count;
  Cannonball balls[MAX_BALLS];

  float fire_timer;   // tid tills nästa kanon börjar ladda
  float elapsed;      // överlevd tid i nuvarande runda
  int gems;           // kristaller i nuvarande runda
  float best_time;
  int best_gems;

  float gem_x;
  float gem_y;
  float gem_anim;

  float hit_flash;    // röd skärmblink efter träff
  float invulnerable; // skyddstid efter träff
  float spawn_x;
  float spawn_y;
};

namespace CANNON_ROOM {
  // Bygger plattformar och kanoner i den aktuella nivån. Anropas efter GenerateRoom.
  void Setup(GameData* data, float spawn_x, float spawn_y);
  void Update(GameData* data, Entity* player, float dt);
  // Varningar, kanoner, kristall och kulor. Ritas före spelaren.
  void Draw(GameData* data, SDL_Renderer* renderer);
  // Träffblink och poäng. Ritas efter spelaren.
  void DrawOverlay(GameData* data, SDL_Renderer* renderer);
}
