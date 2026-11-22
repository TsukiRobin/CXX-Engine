#include "cannonRoom.h"
#include <cmath>
#include <cstdio>
#include "SDL3/SDL_stdinc.h"
#include "gameState.h"
#include "rendering.h"
#include "common.h"

bool IsWall(LevelData* level, float x, float y); // game.cpp

// --- Inställningar ---
const float WARN_TIME = 0.8f;          // hur länge en kanon varnar innan den skjuter
const float START_INTERVAL = 1.8f;     // sekunder mellan skott i början
const float MIN_INTERVAL = 0.45f;      // tätaste skottintervallet
const float INTERVAL_RAMP = 0.02f;     // hur mycket intervallet krymper per sekund
const float START_BALL_SPEED = 8.0f;   // rutor per sekund
const float MAX_EXTRA_SPEED = 6.0f;
const float SPEED_RAMP = 0.1f;
const float HIT_FLASH_TIME = 0.35f;
const float INVULNERABLE_TIME = 1.0f;

static void SetWall(LevelData* level, int x, int y) {
  if (x <= 0 || y <= 0 || x >= level->w - 1 || y >= level->h - 1) return;
  level->cells[y * level->w + x] = (uint8_t)ID::WALL;
}

static void AddCannon(CannonRoom* room, int x, int y, bool faces_right) {
  if (room->cannon_count >= MAX_CANNONS) return;
  Cannon* c = &room->cannons[room->cannon_count++];
  c->cell_x = x;
  c->cell_y = y;
  c->faces_right = faces_right;
  c->warn_timer = 0.0f;
}

static void SpawnGem(CannonRoom* room, LevelData* level, const Entity* player) {
  for (int attempt = 0; attempt < 64; attempt++) {
    int x = 1 + SDL_rand(level->w - 2);
    int y = 1 + SDL_rand(level->h - 2);
    if (level->GetCellID(x, y) != (uint8_t)ID::NONE) continue;
    float dx = x - player->x;
    float dy = y - player->y;
    if (dx * dx + dy * dy < 16.0f) continue; // inte för nära spelaren
    room->gem_x = (float)x;
    room->gem_y = (float)y;
    return;
  }
}

static void ResetRound(CannonRoom* room, Entity* player) {
  if (room->elapsed > room->best_time) room->best_time = room->elapsed;
  if (room->gems > room->best_gems) room->best_gems = room->gems;
  room->elapsed = 0.0f;
  room->gems = 0;
  room->fire_timer = 1.5f;
  for (int i = 0; i < MAX_BALLS; i++) room->balls[i].alive = false;
  for (int i = 0; i < room->cannon_count; i++) room->cannons[i].warn_timer = 0.0f;

  player->x = room->spawn_x;
  player->y = room->spawn_y;
  player->vx = 0.0f;
  player->vy = 0.0f;
  player->dash_timer = 0.0f;
  player->RemoveBehaviour(IGNORE_GRAVITY);
}

static bool Overlaps(float ax0, float ay0, float ax1, float ay1,
                     float bx0, float by0, float bx1, float by1) {
  return ax0 < bx1 && ax1 > bx0 && ay0 < by1 && ay1 > by0;
}

namespace CANNON_ROOM {

void Setup(GameData* data, float spawn_x, float spawn_y) {
  LevelData* level = data->GetCurrentLevel();
  CannonRoom* room = &data->cannon_room;
  *room = {};
  room->enabled = true;
  room->spawn_x = spawn_x;
  room->spawn_y = spawn_y;
  room->fire_timer = 2.0f;

  // Plattformar: ger skydd mot kulor och något att walljumpa/dasha mellan.
  for (int x = 3; x <= 5; x++)   SetWall(level, x, 8);
  for (int x = 12; x <= 14; x++) SetWall(level, x, 8);
  for (int x = 7; x <= 10; x++)  SetWall(level, x, 5);
  SetWall(level, 2, 3);
  SetWall(level, 15, 3);

  // Kanoner i sidoväggarna, förskjutna rader så att det alltid finns en lucka.
  int left_rows[]  = {2, 4, 6, 9};
  int right_rows[] = {3, 5, 7, 10};
  for (int r : left_rows)  AddCannon(room, 0, r, true);
  for (int r : right_rows) AddCannon(room, level->w - 1, r, false);

  room->gem_x = 15.0f;
  room->gem_y = 10.0f;
}

void Update(GameData* data, Entity* player, float dt) {
  CannonRoom* room = &data->cannon_room;
  if (!room->enabled || player == nullptr) return;
  LevelData* level = data->GetCurrentLevel();

  room->gem_anim += dt;
  room->hit_flash = SDL_max(0.0f, room->hit_flash - dt);
  room->invulnerable = SDL_max(0.0f, room->invulnerable - dt);
  room->elapsed += dt;

  // Välj en kanon som börjar ladda.
  room->fire_timer -= dt;
  if (room->fire_timer <= 0.0f && room->cannon_count > 0) {
    for (int attempt = 0; attempt < 8; attempt++) {
      Cannon* c = &room->cannons[SDL_rand(room->cannon_count)];
      if (c->warn_timer <= 0.0f) {
        c->warn_timer = WARN_TIME;
        break;
      }
    }
    room->fire_timer = SDL_max(MIN_INTERVAL, START_INTERVAL - room->elapsed * INTERVAL_RAMP);
  }

  // Kanoner som laddat klart skjuter.
  float speed = START_BALL_SPEED + SDL_min(room->elapsed * SPEED_RAMP, MAX_EXTRA_SPEED);
  for (int i = 0; i < room->cannon_count; i++) {
    Cannon* c = &room->cannons[i];
    if (c->warn_timer <= 0.0f) continue;
    c->warn_timer -= dt;
    if (c->warn_timer > 0.0f) continue;

    for (int b = 0; b < MAX_BALLS; b++) {
      Cannonball* ball = &room->balls[b];
      if (ball->alive) continue;
      ball->alive = true;
      ball->x = (float)c->cell_x + (c->faces_right ? 1.0f : -1.0f);
      ball->y = (float)c->cell_y;
      ball->vx = c->faces_right ? speed : -speed;
      break;
    }
  }

  // Kulorna flyger tills de träffar en vägg eller plattform.
  for (int b = 0; b < MAX_BALLS; b++) {
    Cannonball* ball = &room->balls[b];
    if (!ball->alive) continue;
    ball->x += ball->vx * dt;
    float front_x = ball->x + (ball->vx > 0 ? 0.8f : 0.2f);
    if (IsWall(level, front_x, ball->y + 0.5f)) {
      ball->alive = false;
      continue;
    }

    if (room->invulnerable <= 0.0f &&
        Overlaps(ball->x + 0.2f, ball->y + 0.25f, ball->x + 0.8f, ball->y + 0.75f,
                 player->x + 0.2f, player->y + 0.2f, player->x + 0.8f, player->y + 1.0f)) {
      ResetRound(room, player);
      room->hit_flash = HIT_FLASH_TIME;
      room->invulnerable = INVULNERABLE_TIME;
      return;
    }
  }

  // Plocka kristallen.
  float dx = (player->x + 0.5f) - (room->gem_x + 0.5f);
  float dy = (player->y + 0.6f) - (room->gem_y + 0.5f);
  if (dx * dx + dy * dy < 0.6f * 0.6f) {
    room->gems++;
    SpawnGem(room, level, player);
  }
}

void Draw(GameData* data, SDL_Renderer* renderer) {
  CannonRoom* room = &data->cannon_room;
  if (!room->enabled) return;
  LevelData* level = data->GetCurrentLevel();
  const Camera* cam = &data->camera;
  Sprite* cannon_sprite = &data->spriteBuffer[(int)SPRITE_ID::Cannon];
  Sprite* ball_sprite = &data->spriteBuffer[(int)SPRITE_ID::Cannonball];
  Sprite* gem_sprite = &data->spriteBuffer[(int)SPRITE_ID::Gem];

  // Varningsstrålar: rött band över raden som blir starkare precis innan skottet.
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
  for (int i = 0; i < room->cannon_count; i++) {
    const Cannon* c = &room->cannons[i];
    if (c->warn_timer <= 0.0f) continue;
    float charge = 1.0f - c->warn_timer / WARN_TIME;          // 0 -> 1
    float pulse = 0.5f + 0.5f * std::sin(room->gem_anim * 30.0f);
    float alpha = 30.0f + 110.0f * charge + 30.0f * pulse;

    float x0 = 1.0f, y0 = (float)c->cell_y + 0.3f;
    camera::GridToWorld(&x0, &y0, level);
    SDL_FRect band;
    band.x = x0 - cam->camera_x;
    band.y = y0 - cam->camera_y;
    band.w = (float)((level->w - 2) * CELL_SIZE_PX);
    band.h = 0.4f * CELL_SIZE_PX;
    SDL_SetRenderDrawColor(renderer, 255, 60, 60, (Uint8)SDL_min(alpha, 255.0f));
    SDL_RenderFillRect(renderer, &band);
  }

  // Kanonerna. Laddande kanoner glöder rött.
  for (int i = 0; i < room->cannon_count; i++) {
    const Cannon* c = &room->cannons[i];
    if (c->warn_timer > 0.0f) {
      float charge = 1.0f - c->warn_timer / WARN_TIME;
      Uint8 gb = (Uint8)(255 - 170 * charge);
      SDL_SetTextureColorMod(cannon_sprite->texture, 255, gb, gb);
    }
    // Mynningen sitter högt i bilden, så sänk den så att den pekar mitt på raden.
    RenderSprite_Grid(cannon_sprite, level, renderer, cam,
                      (float)c->cell_x, (float)c->cell_y + 0.25f, 1.0f, 1.0f,
                      c->faces_right ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL);
    SDL_SetTextureColorMod(cannon_sprite->texture, 255, 255, 255);
  }

  // Kristallen guppar lite.
  float bob = std::sin(room->gem_anim * 4.0f) * 0.08f;
  RenderSprite_Grid(gem_sprite, level, renderer, cam, room->gem_x, room->gem_y + bob);

  for (int b = 0; b < MAX_BALLS; b++) {
    const Cannonball* ball = &room->balls[b];
    if (!ball->alive) continue;
    RenderSprite_Grid(ball_sprite, level, renderer, cam, ball->x, ball->y, 1.0f, 1.0f,
                      ball->vx > 0 ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL);
  }
}

static void DrawText(SDL_Renderer* renderer, float x, float y, const char* text) {
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  SDL_RenderDebugText(renderer, x + 1, y + 1, text);
  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
  SDL_RenderDebugText(renderer, x, y, text);
}

void DrawOverlay(GameData* data, SDL_Renderer* renderer) {
  CannonRoom* room = &data->cannon_room;
  if (!room->enabled) return;

  if (room->hit_flash > 0.0f) {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 255, 40, 40, (Uint8)(140.0f * room->hit_flash / HIT_FLASH_TIME));
    SDL_FRect full = {0, 0, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT};
    SDL_RenderFillRect(renderer, &full);
  }

  // Debugfonten är 8x8 och bara ASCII, så inga å/ä/ö här.
  char line[128];
  SDL_snprintf(line, sizeof(line), "TID %5.1f s   KRISTALLER %d   REKORD %.1f s / %d",
               room->elapsed, room->gems, room->best_time, room->best_gems);
  SDL_SetRenderScale(renderer, 2.0f, 2.0f);
  DrawText(renderer, 30.0f, 14.0f, line);
  SDL_SetRenderScale(renderer, 1.0f, 1.0f);
}

}
