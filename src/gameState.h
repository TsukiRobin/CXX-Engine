#pragma once
#include "command.h"
#include "image.h"
#include "imgui/imgui_internal.h"
#include "levels.h"
#include "entity.h"

struct GameData {
  uint32_t command_timestamp;
  const float* dt;
  ImGuiContext* imGui_context;
  Image* fallback;
  Image* wall;
  Image* ground;
  Image* player;
  Image* box;
  Memory::Arena* arena_levels;
  Memory::Arena* arena_images;
  Memory::Arena* arena_entities;
  Memory::Arena* arena_commands;
  CommandBuffer* commandBuffer;
  LevelData* levels;
  int currentLevelIndex;
  int levelCount;
  int currentLevel;
  bool* keys_previous;
  LevelData* GetCurrentLevel(){
    return &levels[currentLevelIndex];
  }

  Position* input_buffer;
  int input_buffer_capacity;
  int input_buffer_write_count;
  int input_buffer_read_count;
    
};



