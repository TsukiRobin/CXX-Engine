#include <cassert>
#include <string>
#include "SDL3/SDL_render.h"
#include "SDL3_image/SDL_image.h"
#include "image.h"
#include "arena.h"

using namespace std;

// Grundmappen för våra sprites.
  const char* DIRECTORY = "assets/sprites/";
// Våran fallback png som används om någon annan sprite inte laddar.
const char* FALLBACK  = "assets/sprites/fallback.png";



Image* AssetManagement::LoadSprite(Memory::Arena* arena, SDL_Renderer* renderer, const char* name){

 // Bygg path till bilden vi vill ladda in
  string path = DIRECTORY;
  path = path.append(name);

  SDL_Surface* surface = IMG_Load(path.c_str());
  
  // Fallback logik
  if(surface == nullptr){
    surface = IMG_Load(FALLBACK);
  }
  // Säkerställ att surface inte är en nullptr och faktiskt har ett värde
  printf("30");
  assert(surface != nullptr);
  printf("Fungerar fint att ladda in Surface\n");
  // konvertera surface som hanteras av CPU till texure som hanteras av GPU
  // Men vi måste använda SDL_DestroySurface() för att frigöra minnet som allokerades
  // till surface
  SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);

  if (texture == nullptr){
    printf("Kunde inte skapa textur\n");
    SDL_DestroySurface(surface);
    return nullptr;
  }
  
  // Skapa en image struct och sätt värdena från texure till våran struct. 
  Image* img = (Image*)Memory::Allocate(arena, sizeof(Image));
  if (img == nullptr){
    printf("Arena har slut på minne");
    SDL_DestroyTexture(texture);
    SDL_DestroySurface(surface);
    return nullptr;
  }
  img->texture = texture;
  img->height = texture->h;
  img->width = texture->w;

  SDL_DestroySurface(surface);
  

  return img;  
}
