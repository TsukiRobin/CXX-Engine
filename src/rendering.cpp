#include "rendering.h"
#include "SDL3/SDL_render.h"
#include "SDL3/SDL_surface.h"
#include "common.h"
#include "spriteLibrary.h"

void RenderSprite_World(Sprite* sprite, SDL_Renderer* renderer, const Camera* camera,
                        float x, float y, float scale, float alpha, SDL_FlipMode flip){
    SDL_FRect rect;
    rect.x = x;
    rect.y = y;
    rect.h = sprite->height * UPSCALE_FACTOR * scale;
    rect.w = sprite->width * UPSCALE_FACTOR * scale;
    rect.x -= camera->camera_x;
    rect.y -= camera->camera_y;
    
    SDL_SetTextureAlphaModFloat(sprite->texture, alpha);
    
    // Ordningen: Renderer, Textur, Source (nullptr), Dest (&rect), Vinkel (0.0), Centrum (nullptr), Flip
    SDL_RenderTextureRotated(renderer, sprite->texture, nullptr, &rect, 0.0, nullptr, flip);
}

void RenderSprite_Grid(Sprite* sprite, LevelData* lvl, SDL_Renderer* renderer, const
                       Camera* camera, float x, float y, float scale, float alpha, SDL_FlipMode flip){
    camera::GridToWorld(&x, &y, lvl);
    RenderSprite_World(sprite, renderer, camera, x, y, scale, alpha, flip);
}
