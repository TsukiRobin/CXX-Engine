
#include <cstdint>
#include "levelRenderer.h"
#include "SDL3/SDL_surface.h"
#include "rendering.h"
#include "spriteLibrary.h"
#include "common.h"

void RenderLevel(GameData* gameData, SDL_Renderer* renderer){
  LevelData lvl = gameData->levels[gameData->currentLevel];

  for (int x = 0; x < lvl.w; x++){
    for(int y = 0; y < lvl.h; y++){
      uint8_t cellType = lvl.GetCellID(x, y);
      // Väggarket är 2x2 bitar av samma mur, så grannceller fortsätter teglet.
      int tile_frame = (x % 2) + 2 * (y % 2);

      if ((ID)cellType == ID::NONE) {
        // Bakvägg: samma mur, nästan genomskinlig, ger djup åt rummet.
        Sprite* wall = GetSpriteFromID(ID::WALL, gameData->spriteBuffer);
        RenderSprite_Grid(wall, &lvl, renderer, &gameData->camera, x, y, 1.0f, 0.12f, SDL_FLIP_NONE, tile_frame);
        continue;
      }

      Sprite* sprite = GetSpriteFromID((ID)cellType, gameData->spriteBuffer);
      if (sprite != nullptr) {
        RenderSprite_Grid(sprite, &lvl, renderer, &gameData->camera, x, y, 1.0f, 1.0f, SDL_FLIP_NONE, tile_frame);
      }
    }
  }
}

void RenderEntities(GameData* data, SDL_Renderer* renderer){
  LevelData lvlData = data->levels[data->currentLevel];
  
  for(int i = 0; i < lvlData.entityCount; i++){
    Entity entity = lvlData.entityBuffer[i];

    if (entity.id == ID::NONE) {
        continue;
    }

    Sprite* sprite = GetEntitySprite(&entity, data->spriteBuffer);
    SDL_FlipMode flip = entity.facing_left ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
    float wall_push_x = 0.0f;
    float alpha = 1.0f;
    const CannonRoom* room = &data->cannon_room;
    if (entity.id == ID::PLAYER && room->invulnerable > 0.0f) {
        // Blinka under skyddstiden efter en träff.
        alpha = SDL_fmodf(room->invulnerable, 0.15f) < 0.075f ? 0.3f : 1.0f;
    }
    if (entity.id == ID::PLAYER && entity.anim == PlayerAnim::WallGlide) {
        // player_wall_glide.png har kroppen mot vänsterkanten, alltså mot en
        // vägg till vänster. Spegla när väggen är till höger.
        flip = entity.wall_on_right ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
        // Kollisionen i Update() känner av väggen 0.2 in från kanten
        // (x + 0.2 / x + 0.8), så rutan överlappar väggen med upp till 0.2.
        // Flytta ut spriten lika mycket så kroppen ligger an mot väggytan.
        const float WALL_COLLISION_INSET = 0.2f;
        wall_push_x = entity.wall_on_right ? -WALL_COLLISION_INSET : WALL_COLLISION_INSET;
    }

    if (sprite != nullptr) {
        // Rutor större än en cell (t.ex. 36x36) centreras i x och
        // bottenjusteras i y, så att figurens fötter hamnar på samma ställe.
        const float cell = (float)(CELL_SIZE_PX / UPSCALE_FACTOR);
        float offset_x = (cell - sprite->width) / 2.0f / cell;
        float offset_y = (cell - sprite->height) / cell;
        RenderSprite_Grid(sprite, &lvlData, renderer, &data->camera,
                          entity.x + offset_x + wall_push_x, entity.y - 0.2f + offset_y,
                          1.0, alpha, flip, entity.current_frame);
    }
  }  
}
