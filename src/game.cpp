
#include <cstdio>
#include <cmath> 
#include <cstdlib>
#include "game.h"
#include "SDL3/SDL_scancode.h"
#include "entity.h"
#include "gameState.h"
#include "imgui/imgui.h"
#include "levelRenderer.h"
#include "levels.h"
#include "dev_gui.h"
#include "common.h"

bool IsWall(LevelData* level, float x, float y) {
    if (x < 0 || y < 0 || x >= level->w || y >= level->h) return true;
    return level->GetCellID((int)x, (int)y) == (uint8_t)ID::WALL;
}

void Jump(Entity* e, GameData* data) {
    if(e->jumps_left > 0) {
        e->jumps_left--; 
        e->on_floor = false;
        
        printf("JUMPING! Jumps left: %d\n", e->jumps_left);
        e->vy = data->player_jump_power; 
    }
}

void Dash(Entity* e, GameData* d) {
    if(e->jumps_left > 0) {
        e->jumps_left--;
        e->on_floor = false;
        
        e->AddBehaviour(IGNORE_GRAVITY); 
        e->dash_timer = 0.2f; 

        float mouse_x = d->input.mouse_x;
        float mouse_y = d->input.mouse_y;
        
        float player_screen_x = e->x;
        float player_screen_y = e->y;
        camera::GridToWorld(&player_screen_x, &player_screen_y, d->GetCurrentLevel());
        player_screen_x -= d->camera.camera_x;
        player_screen_y -= d->camera.camera_y;
        
        float dir_x = mouse_x - player_screen_x;
        float dir_y = mouse_y - player_screen_y;
        
        float length = std::sqrt(dir_x * dir_x + dir_y * dir_y);
        if(length != 0) {
            dir_x /= length;
            dir_y /= length;
        }
        
        e->vx = dir_x * d->player_dash_power;
        e->vy = dir_y * d->player_dash_power;
    }
}

extern "C" {

void Initialize(GameData* data, SDL_Window* window, SDL_Renderer* renderer) {
    DEV::Initialize(window, renderer);
    data->imGui_context = ImGui::GetCurrentContext();

    AssetManagement::LoadAllSprites(data->spriteBuffer, renderer);  
    data->currentLevel = 0;
    data->score = 0; 
    
    data->gravity = 200.0f;
    data->player_jump_power = -40.0f;
    data->player_dash_power = 60.0f; 

    printf("Creating Level\n");
    CreateLevel(data->arena_levels, &data->levels[0], "assets/levels/testLevel.tmj");
    printf("Level Created, Creating entities\n");
    CreateEntities(data->arena_entities, &data->levels[data->currentLevel]);
    printf("Entities Created!\n");
}

bool HandleEvents(GameData *data, SDL_Event event){
    DEV::ProcessEvents(&event);

    if(event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE){
        return false;
    }
    return true;
}

void Update(GameData* data, float dt){

    if(KeyPressed(&data->input, SDL_SCANCODE_F2)){
        data->edit_level = !data->edit_level;
    }
    if(data->edit_level){
        EDITOR::Update(&data->editorData, &data->input, data->GetCurrentLevel());
    }

    LevelData* level = data->GetCurrentLevel();

    for (int i = 0; i < level->entityCount; i++) {
        Entity* e = &level->entityBuffer[i];
        if (e->id == ID::NONE) continue;

        if (e->id == ID::PLAYER) {
            if (KeyHeld(&data->input, SDL_SCANCODE_RIGHT) || (KeyHeld(&data->input, SDL_SCANCODE_D))) e->vx += ACCELERATION * dt;
            if (KeyHeld(&data->input, SDL_SCANCODE_LEFT) || (KeyHeld(&data->input, SDL_SCANCODE_A)))  e->vx -= ACCELERATION * dt;
            
            if(KeyPressed(&data->input, SDL_SCANCODE_SPACE)) Jump(e, data);      
            if(MousePressed(&data->input, MouseButtons::RIGHT)) Dash(e, data);
        }

        e->on_floor = false; 
        bool touching_wall = false; 

        if (e->HasBehaviour(CAN_MOVE)) {
            
            e->vx *= std::pow(FRICTION, dt * 60.0f); 

            e->x += e->vx * dt;
            
            float check_x = e->x + (e->vx > 0 ? 0.8f : 0.2f);
            if (IsWall(level, check_x, e->y + 0.5f)) {
                e->x -= e->vx * dt; 
                e->vx = e->vx * BOUNCE; 
                touching_wall = true; 
            }

            e->y += e->vy * dt;
            
            float check_y = e->y + (e->vy > 0 ? 0.8f : 0.2f);
            if (IsWall(level, e->x + 0.5f, check_y)) {
                if (e->vy > 0) {
                    e->on_floor = true; 
                }
                e->y -= e->vy * dt; 
                e->vy = 0; 
            }
        }

        if (e->dash_timer > 0) {
            e->dash_timer -= dt;
            if (e->dash_timer <= 0) {
                e->RemoveBehaviour(IGNORE_GRAVITY); 
            }
        }

        if (!e->HasBehaviour(IGNORE_GRAVITY)) {
            e->vy += data->gravity * dt;
        }

        if (e->on_floor || touching_wall) {
            e->jumps_left = 2;
        }
    }
}

void Draw(GameData* data, SDL_Renderer* renderer){
    DEV::PreDraw(data->imGui_context);
    SDL_SetRenderDrawColor(renderer, 40, 40, 50, 255); 
    SDL_RenderClear(renderer);

    RenderLevel(data, renderer);
    RenderEntities(data, renderer);

    DEV::Draw(data, renderer);
    SDL_RenderPresent(renderer);
}

void OnQuit(SDL_Renderer* renderer){
    SDL_DestroyRenderer(renderer);
}

}
