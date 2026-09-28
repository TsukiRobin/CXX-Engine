#include "spriteLibrary.h"
#include "SDL3/SDL_render.h"
#include "SDL3_image/SDL_image.h"
#include <cassert>



const char* FALLBACK_PATH = "assets/sprites/fallback.png";

static const SpriteDataEntry all_sprite_data[] = {
  {SPRITE_ID::Fallback, FALLBACK_PATH, 1},
  {SPRITE_ID::Wall, "assets/sprites/wall.png", 1},
  {SPRITE_ID::Player, "assets/sprites/player.png", 6},
  {SPRITE_ID::Rock, "assets/sprites/box.png",1},
  {SPRITE_ID::Ground, "assets/sprites/ground.png",1 },
  {SPRITE_ID::Medusa, "assets/sprites/medusa.png",1},
};



Sprite* GetSpriteFromID(ID id, Sprite* spriteBuffer){
  switch(id){
    case ID::NONE:
      return nullptr;
    case ID::GROUND:
      return &spriteBuffer[(int)SPRITE_ID::Ground];
    case ID::WALL:
      return &spriteBuffer[(int)SPRITE_ID::Wall];
    case ID::PLAYER:
      return &spriteBuffer[(int)SPRITE_ID::Player];
      break;

    default:
      return &spriteBuffer[(int)SPRITE_ID::Fallback];
    break;
    }
}


namespace AssetManagement{
  void LoadAllSprites(Sprite* spriteBuffer, SDL_Renderer* renderer){
    for (SpriteDataEntry entry: all_sprite_data){
      LoadSprite(spriteBuffer, entry, renderer);
    }
  }


  void LoadSprite(Sprite* spriteBuffer, SpriteDataEntry entry, SDL_Renderer* renderer){
    SDL_Surface* surface = IMG_Load(entry.path);
    if(surface == nullptr){
      surface = IMG_Load(FALLBACK_PATH);
    }

    assert(surface != nullptr);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    Sprite* sprite = &spriteBuffer[(int)entry.id];
    sprite->texture = texture;
    sprite->frames = entry.frames > 0 ? entry.frames : 1;
    sprite->height = texture->h;
    sprite->width = texture->w / sprite->frames;


    SDL_DestroySurface(surface);
  }
}
