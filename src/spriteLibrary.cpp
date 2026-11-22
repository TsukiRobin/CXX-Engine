#include "spriteLibrary.h"
#include "SDL3/SDL_render.h"
#include "SDL3_image/SDL_image.h"
#include <cassert>



const char* FALLBACK_PATH = "assets/sprites/fallback.png";

static const SpriteDataEntry all_sprite_data[] = {
  {SPRITE_ID::Fallback, FALLBACK_PATH, 1},
  // 4 rutor = 2x2-bitar av en 64x64 stenmur, väljs efter cellens position
  // så att teglet fortsätter sömlöst mellan celler (se RenderLevel).
  {SPRITE_ID::Wall, "assets/sprites/wall.png", 4},
  {SPRITE_ID::Player, "assets/sprites/player.png", 6},
  {SPRITE_ID::Rock, "assets/sprites/box.png",1},
  {SPRITE_ID::Ground, "assets/sprites/ground.png",1 },
  {SPRITE_ID::Medusa, "assets/sprites/medusa.png",1},
  // Spelaranimationer (SpriteCook, 36x36 per ruta, se spritecook-assets.json)
  {SPRITE_ID::PlayerIdle, "assets/sprites/player_idle.png", 6},
  {SPRITE_ID::PlayerRun, "assets/sprites/player_run.png", 8},
  {SPRITE_ID::PlayerJump, "assets/sprites/player_jump.png", 8},
  {SPRITE_ID::PlayerWallGlide, "assets/sprites/player_wall_glide.png", 1},
  // Kanonrummet
  {SPRITE_ID::Cannon, "assets/sprites/cannon.png", 1},
  {SPRITE_ID::Cannonball, "assets/sprites/canonball.png", 1},
  {SPRITE_ID::Gem, "assets/sprites/gem.png", 1},
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

Sprite* GetEntitySprite(const Entity* entity, Sprite* spriteBuffer){
  if (entity->id != ID::PLAYER) {
    return GetSpriteFromID(entity->id, spriteBuffer);
  }
  switch (entity->anim) {
    case PlayerAnim::Run:       return &spriteBuffer[(int)SPRITE_ID::PlayerRun];
    case PlayerAnim::Jump:      // Dash återanvänder hopparket (sträckt pose)
    case PlayerAnim::Dash:      return &spriteBuffer[(int)SPRITE_ID::PlayerJump];
    case PlayerAnim::WallGlide: return &spriteBuffer[(int)SPRITE_ID::PlayerWallGlide];
    case PlayerAnim::Idle:
    default:                    return &spriteBuffer[(int)SPRITE_ID::PlayerIdle];
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
