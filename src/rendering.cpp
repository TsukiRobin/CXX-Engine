#include "rendering.h"
#include "SDL3/SDL_rect.h"
#include "SDL3/SDL_render.h"
#include "SDL3/SDL_surface.h"
#include "common.h"
#include "spriteLibrary.h"

void RenderSprite_World(Sprite* sprite, SDL_Renderer* renderer, const Camera* camera,
                        float x, float y, float scale, float alpha, SDL_FlipMode flip, int frame){
    SDL_FRect rect;
    rect.x = x - camera->camera_x;
    rect.y = y - camera->camera_y;
    rect.h = sprite->height * UPSCALE_FACTOR * scale;
    rect.w = sprite->width * UPSCALE_FACTOR * scale;


    int safe_frame = frame % sprite->frames;
    SDL_FRect src_rect;
    src_rect.x = (float)(safe_frame * sprite->width);
    src_rect.y = 0.0f;
    src_rect.w = (float)sprite->width;
    src_rect.h = (float)sprite->height;
    
    
    SDL_SetTextureAlphaModFloat(sprite->texture, alpha);
    
    // Ordningen: Renderer, Textur, Source (nullptr), Dest (&rect), Vinkel (0.0), Centrum (nullptr), Flip
    SDL_RenderTextureRotated(renderer, sprite->texture, &src_rect, &rect, 0.0, nullptr, flip);
}

void RenderSprite_Grid(Sprite* sprite, LevelData* lvl, SDL_Renderer* renderer, const
                       Camera* camera, float x, float y, float scale, float alpha, SDL_FlipMode flip, int frame){
    camera::GridToWorld(&x, &y, lvl);
    RenderSprite_World(sprite, renderer, camera, x, y, scale, alpha, flip, frame);
}
