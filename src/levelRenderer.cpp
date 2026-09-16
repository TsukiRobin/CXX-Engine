
#include <cmath>
#include <cstdint>
#include "levelRenderer.h"
#include "common.h"
#include "rendering.h"
#include "spriteLibrary.h"

void RenderLevel(GameData* gameData, SDL_Renderer* renderer){
  LevelData lvl = gameData->levels[gameData->currentLevel];

  for (int x = 0; x < lvl.w; x++){
    for(int y = 0; y < lvl.h; y++){
      uint8_t cellType = lvl.GetCellID(x, y);
      Sprite* sprite = GetSpriteFromID((ID)cellType, gameData->spriteBuffer);

      if (sprite != nullptr) {
        RenderSprite_Grid(sprite, &lvl, renderer, &gameData->camera, x, y);
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

    Sprite* sprite = GetSpriteFromID(entity.id, data->spriteBuffer);

    if (sprite != nullptr) {
        RenderSprite_Grid(sprite, &lvlData, renderer, &data->camera, entity.x, entity.y);    
    }
  }  
}
