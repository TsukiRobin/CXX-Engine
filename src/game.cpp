
#include <cstdio>
#include <cmath> 
#include "game.h"
#include "SDL3/SDL_scancode.h"
#include "entity.h"
#include "gameState.h"
#include "imgui/imgui.h"
#include "levelRenderer.h"
#include "levels.h"
#include "dev_gui.h"
#include "common.h"
#include "cannonRoom.h"

bool IsWall(LevelData* level, float x, float y) {
    if (x < 0 || y < 0 || x >= level->w || y >= level->h) return true;
    return level->GetCellID((int)x, (int)y) == (uint8_t)ID::WALL;
}



void Jump(Entity* entity, GameData* data) {
    if (entity->jumps_left > 0) {
        LevelData* level = data->GetCurrentLevel();
        
        bool wall_left  = IsWall(level, entity->x - 0.1f, entity->y + 0.5f);
        bool wall_right = IsWall(level, entity->x + 1.1f, entity->y + 0.5f);

        // Spara undan om vi stod på marken INNAN vi nollställer det!
        bool is_grounded = entity->on_floor;

        entity->jumps_left--; 
        entity->on_floor = false;
        
        if (is_grounded) {
            entity->vy = data->player_jump_power; 
        } 
        else if (wall_left) {
            entity->vy = data->player_jump_power - 10.0f; 
            entity->vx = 25.0f; 
            entity->facing_left = false; // Vänd spelaren utåt!
        } 
        else if (wall_right) {
            entity->vy = data->player_jump_power - 10.0f; 
            entity->vx = -25.0f; 
            entity->facing_left = true; // Vänd spelaren utåt!
        } 
        else {
            entity->vy = data->player_jump_power; 
        }
    }
}
// Väljer animation utifrån spelarens tillstånd och stegar fram rutorna.
// Hopparket (player_jump.png, 8 rutor): 0-1 avstamp, 2 sträckt uppåt,
// 3 toppen, 4-5 fall, 6-7 landning.
void UpdatePlayerAnimation(Entity* e, bool touching_wall, float dt) {
    PlayerAnim next;
    if (e->dash_timer > 0) {
        next = PlayerAnim::Dash;
    } else if (touching_wall && !e->on_floor && e->vy > 0) {
        next = PlayerAnim::WallGlide;
    } else if (!e->on_floor) {
        next = PlayerAnim::Jump;
    } else if (std::abs(e->vx) > 0.5f) {
        next = PlayerAnim::Run;
    } else {
        next = PlayerAnim::Idle;
    }

    if (next != e->anim) {
        e->anim = next;
        e->current_frame = 0;
        e->anim_timer = 0.0f;
    }

    switch (e->anim) {
        case PlayerAnim::Jump:
            // Rutan styrs av hastigheten i stället för tiden,
            // så posen alltid matchar var i hoppbågen spelaren är.
            if      (e->vy < -15.0f) e->current_frame = 2;
            else if (e->vy <  -3.0f) e->current_frame = 3;
            else if (e->vy <   8.0f) e->current_frame = 4;
            else                     e->current_frame = 5;
            break;
        case PlayerAnim::Dash:
            e->current_frame = 2; // sträckt pose
            break;
        case PlayerAnim::WallGlide:
            e->current_frame = 0;
            break;
        case PlayerAnim::Run:
        case PlayerAnim::Idle: {
            // Sekunder per bildruta
            const float frame_time = (e->anim == PlayerAnim::Run) ? 0.08f : 0.15f;
            e->anim_timer += dt;
            while (e->anim_timer >= frame_time) {
                e->anim_timer -= frame_time;
                e->current_frame++; // RenderSprite_World loopar med % frames
            }
            break;
        }
    }
}

void Dash(Entity* e, GameData* d) {
    if(e->jumps_left > 0) {
        e->jumps_left--;
        e->on_floor = false;
        
        e->AddBehaviour(IGNORE_GRAVITY); 
        e->dash_timer = d->player_dash_time;

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
    
    data->gravity = 95.0f;
    data->player_jump_power = -25.0;
    // Halva farten mot tidigare (50 i 0.1 s), men längre tid,
    // så dashen blir ca 4 rutor lång i stället för 5 och syns bättre.
    data->player_dash_power = 25.0f;
    data->player_dash_time = 0.16f;
    data->wall_friction = 0.45f;

    printf("Creating Level\n");
    GenerateRoom(data->arena_levels, &data->levels[0], 18, 12);

    LevelData* level = &data->levels[0];
    Entity* player = GetNextAvailableEntity(level);    
    player->id = ID::PLAYER;
    player->x = 5.0f;
    player->y = 5.0f;
    player->InitializeBaseBehaviour();
    player->jumps_left = 2;    
    player->current_frame = 0;
    player->anim_timer = 0.0f;

    CANNON_ROOM::Setup(data, player->x, player->y);
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
    Entity* player = nullptr;

    for (int i = 0; i < level->entityCount; i++) {
        Entity* e = &level->entityBuffer[i];
        if (e->id == ID::NONE) continue;
        if (e->id == ID::PLAYER) {
            if (KeyHeld(&data->input, SDL_SCANCODE_RIGHT) || (KeyHeld(&data->input, SDL_SCANCODE_D))){
               e->vx += ACCELERATION * dt; 
               e->facing_left = false;
            } 
            if (KeyHeld(&data->input, SDL_SCANCODE_LEFT) || (KeyHeld(&data->input, SDL_SCANCODE_A))){
                e->vx -= ACCELERATION * dt;
                e->facing_left = true;
            }  
            
            if(KeyPressed(&data->input, SDL_SCANCODE_SPACE)) Jump(e, data);      
            if(KeyPressed(&data->input, SDL_SCANCODE_LSHIFT)) Dash(e, data);
        }

        e->on_floor = false; 
        bool touching_wall = false; 

        if (e->HasBehaviour(CAN_MOVE)) {
            if(e->dash_timer <= 0){
                e->vx *= std::pow(FRICTION, dt * 60.0f); 
            }

            e->x += e->vx * dt;
            
            float check_x = e->x + (e->vx > 0 ? 0.8f : 0.2f);
            if (IsWall(level, check_x, e->y + 0.5f)) {
                e->x -= e->vx * dt;
                e->wall_on_right = e->vx > 0; // måste läsas innan vx nollas
                e->vx = 0;
                touching_wall = true;

                if (e->vy > 0) {
                    e->vy *= std::pow(data->wall_friction, dt * 60.0f); 
                    printf("We are touching a wall right now\n");
                }
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

                e->vx *= 0.1f;
                e->vy *= 0.1f;
            }
        }

        if (!e->HasBehaviour(IGNORE_GRAVITY)) {
            e->vy += data->gravity * dt;
        }

        if (e->on_floor) {
            e->jumps_left = 2;
        }

        // --- ANIMERING ---
        if (e->id == ID::PLAYER) {
            UpdatePlayerAnimation(e, touching_wall, dt);
            player = e;
        }
    }

    if (!data->edit_level) {
        CANNON_ROOM::Update(data, player, dt);
    }
}

void Draw(GameData* data, SDL_Renderer* renderer){
    DEV::PreDraw(data->imGui_context);
    SDL_SetRenderDrawColor(renderer, 40, 40, 50, 255); 
    SDL_RenderClear(renderer);

    RenderLevel(data, renderer);
    CANNON_ROOM::Draw(data, renderer);
    RenderEntities(data, renderer);
    CANNON_ROOM::DrawOverlay(data, renderer);

    DEV::Draw(data, renderer);
    SDL_RenderPresent(renderer);
}

void OnQuit(SDL_Renderer* renderer){
    SDL_DestroyRenderer(renderer);
}

}
