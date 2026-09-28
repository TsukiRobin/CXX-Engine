#pragma once

#include "SDL3/SDL_render.h"
#include "entity.h"


struct Sprite{
  SDL_Texture* texture;
  int height;
  int width;
  int frames = 1;
};

enum class SPRITE_ID{
  Fallback,
  Ground,
  Wall,
  Rock,
  Player,
  Medusa,
  Golem,
  Ghost
};


struct SpriteDataEntry{
  SPRITE_ID id;
  const char* path;
  int frames = 1;
  
};
Sprite* GetSpriteFromID(ID id, Sprite* spriteBuffer);

namespace AssetManagement{

  void LoadSprite(Sprite* spriteBuffer, SpriteDataEntry entry, SDL_Renderer* render);
  void LoadAllSprites(Sprite* spriteBuffer, SDL_Renderer* renderer);
}
