#include  <cstdint>
#include <fstream>
#include <vector>
#include "levels.h"
#include "arena.h"
#include "Parsers/json.hpp"
#include "entity.h"
#include "common.h"
using namespace std;



#include <fstream>
#include <vector>

const int LEVEL_INDEX = 0;
const int ENTITIES_INDEX = 1;
void CreateLevel(Arena* arena, LevelData* level, const char* level_name) {
    if (!arena || !level || !level_name) {
        printf("arena: %p, level: %p, level_name: %p\n", (void*)arena, (void*)level, (void*)level_name);
        printf("ERROR: Invalid arguments passed to CreateLevel\n");
        fflush(stdout);
        
        return;
    }

    std::ifstream stream(level_name);
    if (!stream.is_open()) {
        printf("ERROR: Could not open level file at path: %s\n", level_name);
        fflush(stdout);
        return;
    }

    try {
        nlohmann::json jsonResult = nlohmann::json::parse(stream);

        if (!jsonResult.contains("layers") || jsonResult["layers"].size() <= LEVEL_INDEX) {
            printf("ERROR: Layer index %d not found in %s\n", LEVEL_INDEX, level_name);
            fflush(stdout);
            return;
        }

        // Tiled stores tile data as ints
        auto dataField = jsonResult["layers"][LEVEL_INDEX]["data"].get<std::vector<int>>();

        level->w = jsonResult["width"].get<int>();
        level->h = jsonResult["height"].get<int>();
        level->level_path = level_name;

        size_t size_of_cells = sizeof(uint8_t) * level->w * level->h;
        level->cells = (uint8_t*)Memory::Allocate(arena, size_of_cells);

        if (level->cells == nullptr) {
            printf("ERROR: Arena out of memory when allocating level cells (%zu bytes)\n", size_of_cells);
            fflush(stdout);
            return;
        }

        for (int i = 0; i < level->w * level->h && i < (int)dataField.size(); i++) {
            level->cells[i] = (uint8_t)dataField[i];
        }

        printf("Successfully loaded level '%s' (%dx%d)\n", level_name, level->w, level->h);
        fflush(stdout);

    } catch (const nlohmann::json::exception& e) {
        printf("JSON Exception in CreateLevel (%s): %s\n", level_name, e.what());
        fflush(stdout);
    } catch (const std::exception& e) {
        printf("Standard Exception in CreateLevel: %s\n", e.what());
        fflush(stdout);
    }
}


void CreateEntities(Arena* arena, LevelData* lvl_data){
  Reset(arena);
  lvl_data->entityCount = 0;
  fstream stream(lvl_data->level_path);
  auto result = nlohmann::json::parse(stream);
  auto entityData = result["layers"][ENTITIES_INDEX]["data"].get<vector<uint8_t>>();

  for (int i = 0; i < lvl_data->w * lvl_data->h; i++){
    unsigned char entity_id = entityData[i];
    if(entity_id != 0){
      lvl_data->entityCount++;
    }
  }

  lvl_data->entityBuffer = ALLOC_ARRAY(arena, Entity, lvl_data->entityCount);
  int index = 0;
  for (int i = 0; i < lvl_data->w * lvl_data->h; i++){
    unsigned char entity_id = entityData[i];
    if (entity_id != 0){
      int x = i % lvl_data->w;
      int y = i / lvl_data->w;
      lvl_data->entityBuffer[index].id = (ID)entity_id;
      lvl_data->entityBuffer[index].InitializeBaseBehaviour();
      lvl_data->entityBuffer[index].x = x;
      lvl_data->entityBuffer[index].y = y;
      index += 1;      
    }
  }

}
