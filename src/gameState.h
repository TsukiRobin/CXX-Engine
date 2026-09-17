#pragma once
#include "spriteLibrary.h"
#include "imgui/imgui_internal.h"
#include "levels.h"
#include "entity.h"
#include "input.h"
#include "camera.h"
#include "levelEditor.h"



struct GameData {
  uint32_t command_timestamp;
  const float* dt;
  ImGuiContext* imGui_context;
  Sprite* spriteBuffer;
  Input input;
  Camera camera;
  Memory::Arena* arena_levels;
  Memory::Arena* arena_images;
  Memory::Arena* arena_entities;
  Memory::Arena* arena_input;
  float player_jump_power;
  float gravity;  
  float player_dash_power;
  float wall_friction;
  LevelData* levels;
  int currentLevelIndex;
  int levelCount;
  int currentLevel;
  bool edit_level;
  Editor editorData;


  int score; 
  bool* keys_previous;
  LevelData* GetCurrentLevel(){
    return &levels[currentLevelIndex];
  }

  Position* input_buffer;
  int input_buffer_capacity;
  int input_buffer_write_count;
  int input_buffer_read_count;
    
};



