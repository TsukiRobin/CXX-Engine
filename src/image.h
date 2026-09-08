
#pragma once
#include "SDL3/SDL_render.h"
#include "arena.h"

// Skapar Image structen som innehåller
// Texture pointer (.png etc.)
// Bredd och höjd på bilden

struct Image{
  SDL_Texture* texture;
  int width;
  int height;
};

// Skapar namespace för funktionen att ladda in våran bild
// och allokera bilden i våran memory arena
namespace AssetManagement
{
  Image* LoadSprite(Memory::Arena* arena, SDL_Renderer* renderer, const char* path);
}
