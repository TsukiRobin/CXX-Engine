	#include <windows.h>
#include <libloaderapi.h>
#include <processthreadsapi.h>
#include <fileapi.h>
#include <fstream>
#include <cstdio>
#include "SDL3/SDL_events.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_keycode.h"
#include "SDL3/SDL_render.h"
#include "SDL3/SDL_timer.h"
#include "command.h"
#include "common.h"
#include "arena.h"
#include "gameState.h"

// SDL  komponent pekare
SDL_Window* window;
SDL_Renderer* renderer;

// Variabler för Delta Time
Uint64 NOW = 0;
Uint64 PREV = 0;

// Projekt namn som används
const char* projectName = "Project1";

constexpr const char* NAME_OF_DLL = "project1_game.dll";
constexpr const char* NAME_OF_TEMP_DLL =  "project1_temp.dll";

// 
// Om jag i framtiden hämtar en minnesadress till en funktion
// och jag säger att det är av typen Function_Initialize
// då ska kompilatorn lita på vad jag definerar att den tar och returnerar
typedef void (*Function_Initialize) (GameData* data,SDL_Window* window, SDL_Renderer* renderer);
typedef bool (*Function_HandleEvents) (GameData* data, SDL_Event event);
typedef void (*Function_Update) (GameData* data, float dt);
typedef void (*Function_Draw) (GameData* data, SDL_Renderer* renderer);
typedef void (*Function_OnQuit) (SDL_Renderer* renderer);


// Skapa textsträngar med dom EXAKTA namnen så att vi inte kan stava fel senare
// Bättre att ha en fast variabel än en ren sträng varje gång vi kallar på dom.
constexpr const char* NAME_OF_FUNC_INIT = "Initialize";
constexpr const char* NAME_OF_FUNC_HANDLE_EVENT = "HandleEvents";
constexpr const char* NAME_OF_FUNC_UPDATE = "Update";
constexpr const char* NAME_OF_FUNC_DRAW = "Draw";
constexpr const char* NAME_OF_FUNC_QUIT = "OnQuit";
// Struct för att koppla funktion pointers och dll pointers.
struct DLL_INFO {
	HMODULE dll;
	FILETIME timestamp;
	Function_Initialize initialize;
	Function_HandleEvents handleEvents;
	Function_Update update;
	Function_Draw draw;
	Function_OnQuit quit;
};


// Funktion för att hämta senaste ändring på DLL filen
FILETIME GetTimestamp(){
	WIN32_FIND_DATA data;
	HANDLE handle = FindFirstFile(NAME_OF_DLL, &data);
	FILETIME time_of_last_change = data.ftLastWriteTime;
	FindClose(handle);
	return time_of_last_change;
}
// Laddar in DLL filen och kopierar till temp om den finns,
// Depth = antal försök som funktionen försöker ladda, en gång per 50ms
// Vi kopierar till en Temp fil för att vi inte sak låsa filen och på
// så vi kan vi fortsätta redigera spelet.
bool LoadDLL(DLL_INFO* info, int depth = 0) {
	printf("loading dll");
	if (depth > 20){
		printf("failed to write temp DLL");
		return false;
	}
		bool success = CopyFile(NAME_OF_DLL, NAME_OF_TEMP_DLL, false);

	if (!success){
		Sleep(50);
		return LoadDLL(info, depth + 1);
	}
// Ladda in Temp filen
	info->dll = LoadLibrary(NAME_OF_TEMP_DLL);

	if (info->dll == nullptr){
		printf("could not load dll");
		return false;
	}
// Letar upp adresserna till funktionerna i DLL filen,
// och kopplar dom till fåran info Struct.
	info->initialize = (Function_Initialize)GetProcAddress(info->dll, NAME_OF_FUNC_INIT);
	info->handleEvents = (Function_HandleEvents)GetProcAddress(info->dll, NAME_OF_FUNC_HANDLE_EVENT);
	info->update = (Function_Update)GetProcAddress(info->dll, NAME_OF_FUNC_UPDATE);
	info->draw = (Function_Draw)GetProcAddress(info->dll, NAME_OF_FUNC_DRAW);
	info->quit = (Function_OnQuit)GetProcAddress(info->dll, NAME_OF_FUNC_QUIT);

	info->timestamp = GetTimestamp();

	return true;	
}
// Frigör vårat minne och ta bort den temporära DLL filen. 
void UnloadDLL(DLL_INFO* info){
	FreeLibrary(info->dll);
	info->dll = nullptr;
	DeleteFile(NAME_OF_TEMP_DLL);
}

// Allokerar mängden minne som vi bestämt i GAME_MEMORY_ALLOWANCE
void* AllocateGameMemory(){
	void* blob = malloc(GAME_MEMORY_ALLOWANCE);
	if(blob == nullptr){
		printf("fatal error: could not allocate memory");
		return nullptr;
	}

	printf("memory  succesfully allocated");
	return blob;
}

//Gamestate saving
// Ofstream = OutFile Stream.
// Vi skapar filen i binärt läge.
// vi castar våra bytes från arena.base till arenasize. och skriver ut dom i 1or och 0or i våran binära fil.
void StoreGameState(Memory::Arena* arena){
	std::ofstream file("temp_state.bin", std::ios::binary);
	file.write(reinterpret_cast<const char*>(arena->base), arena->size);
	file.close();
}


// ifstream = in file stream
// Vi läser det binära innehållet i memory arenan direkt med std::ios:binary
// och skriver sedan över exakt den platsen igen.
// detta betyder att om vi har flera sub arenor så kan vi spara specifika delar i mineet.
void RetrieveGameState(Memory::Arena* arena){
	std::ifstream file("temp_state.bin", std::ios::binary);
	file.read(reinterpret_cast<char*>(arena->base), arena->size);
	file.close();
}

// Simpel funktion för att starta upp vårat spel
// Skapar upp våran event inläsning
// Skapar upp fönstret samt våran renderare
// 
void SDL_SETUP(){
	SDL_Init(SDL_INIT_EVENTS);
	window = SDL_CreateWindow(projectName, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
	renderer = SDL_CreateRenderer(window, nullptr);
}

// Räknar ut hur lång tid som gått sedan den förra framen. 
void CalculateDeltaTime(float& dt){
	NOW = SDL_GetTicksNS();
	dt = NOW - PREV;
	dt = SDL_NS_TO_SECONDS(dt);
	PREV = NOW;
}


// Kollar status på om dll filen förändrats och replacear den i så fall.
void DLL_CheckStatus(DLL_INFO* dll){
	FILETIME timestamp = GetTimestamp();
	bool is_timestamp_changed = CompareFileTime(&dll->timestamp, &timestamp) != 0;
	if (is_timestamp_changed){
		UnloadDLL(dll);
		LoadDLL(dll);
	}
}

void CalculateRemainingFrameTime_MS(double* milliseconds){
		Uint64 frame_end_time_ns = SDL_GetTicksNS();

		double frame_time_spent_ns = frame_end_time_ns - PREV;
		double frame_time_spent_ms = frame_time_spent_ns / 1e6;
		*milliseconds = FRAME_BUDGET_MS - frame_time_spent_ms;
}

int main(){
// Allokera minne
	void* game_memory = AllocateGameMemory();
	if(game_memory == nullptr){
		return 1;
	}

// Initiera memory arena och allokera gameData inuti den
	printf("Allocating Game Memory\n");
	Memory::Arena* arena_main = new Memory::Arena();
	Memory::Initialize(arena_main, game_memory, GAME_MEMORY_ALLOWANCE);
	GameData* gameData = ALLOC(arena_main, GameData);
	size_t IMAGE_ARENA_SIZE = sizeof(Image) * 1024;
// >Skapa en subarena för images och initiera dom.
	gameData->arena_images = Memory::CreateSubArena(arena_main, IMAGE_ARENA_SIZE);
// Skapa en subarena för våra levels, entities och initiera dom.
	gameData->arena_levels = Memory::CreateSubArena(arena_main, MEGABYTES(3));
	gameData->arena_entities = Memory::CreateSubArena(gameData->arena_levels, MEGABYTES(1));
	gameData->arena_commands = Memory::CreateSubArena(gameData->arena_levels, MEGABYTES(1));
	gameData->commandBuffer = ALLOC(arena_main, CommandBuffer);
	gameData->commandBuffer->capacity = 2000;
	size_t COMMAND_SIZE = sizeof(AnyCommand) * gameData->commandBuffer->capacity;
	gameData->commandBuffer->allCommands = ALLOC_ARRAY(gameData->arena_commands, AnyCommand, COMMAND_SIZE);
	
	gameData->input_buffer_capacity = 50;
	size_t RING_BUFFER_SIZE = sizeof(Position) * gameData->input_buffer_capacity;
	// gameData->input_buffer = ALLOC_ARRAY(gameData->arena_levels, Position, RING_BUFFER_SIZE);
	gameData->input_buffer = (Position*)Memory::Allocate(gameData->arena_levels, RING_BUFFER_SIZE);

	gameData->levelCount = 5;	
	gameData->levels = (LevelData*)Memory::Allocate(gameData->arena_levels, sizeof(LevelData) * gameData->levelCount);

// Initiera en del i våran arena_levels för att spara våra knapptryck.
	gameData->keys_previous = (bool*)Memory::Allocate(gameData->arena_levels, sizeof(bool) * SDL_SCANCODE_COUNT);
	
	SDL_SETUP();
// Assigna våran fallback.png till gameData fallback 
	gameData->fallback = AssetManagement::LoadSprite(gameData->arena_images, renderer, "fallback.png");
	
	printf("Successfully added game memory\n");

	MMRESULT result = timeBeginPeriod(1);
	if (result == TIMERR_NOCANDO){
		printf("could not increase timer resolution");
		Sleep(2000);
		return 3;
	}

// skapa dll struct och försök ladda in DLL filen.
	DLL_INFO dll;
	bool dll_succesfully_loaded = LoadDLL(&dll);


	if (!dll_succesfully_loaded){
		printf("Failing to load DLL");
		return 2;
	}
 // Sätter upp vårat fönster och initierar våran dll fil.
 	printf("Initializing dll\n");
	dll.initialize(gameData,window, renderer);


// sätter en variable för våran game loop.
	printf("Setting bool running to true\n");
	bool running = true;
	float dt;
	gameData->dt = &dt;
	while(running){

		DLL_CheckStatus(&dll);

		CalculateDeltaTime(dt);

		
		
		SDL_Event event;
		    while(SDL_PollEvent(&event)){
		    		printf("Calling handleEvents with event type: %u\n", event.type);
		    		fflush(stdout);
		        running = dll.handleEvents(gameData, event);
		        if (!running){
								printf("Running set to false, closing\n");
								fflush(stdout);
		            break; // Bryter ur hela while-loopen (stänger spelet)
		        }

		        if(event.type == SDL_EVENT_KEY_DOWN){
            
		            if(event.key.key == SDLK_F9){
		                StoreGameState(arena_main);
		            }
		            else if(event.key.key == SDLK_F10){
		                RetrieveGameState(arena_main);
		            }
		        }
		    } 


// ut det på skärmen med (draw)
		dll.update(gameData, dt);

		memcpy((void*)gameData->keys_previous, SDL_GetKeyboardState(nullptr), SDL_SCANCODE_COUNT * sizeof(bool));
		dll.draw(gameData, renderer);


		double time_to_sleep_ms;
		CalculateRemainingFrameTime_MS(&time_to_sleep_ms);
		if(time_to_sleep_ms > 0){
			if(time_to_sleep_ms > 1){
				SDL_Delay(time_to_sleep_ms - 1);
			}
			while (time_to_sleep_ms > 0) {
				CalculateRemainingFrameTime_MS(&time_to_sleep_ms);
			}
		}
		else{
			printf("missed frame \n");
		}				
	}
// När vi kommit ut ur loopen så stänger vi av renderaren och SDL
// Detta så vi inte låser minne
	dll.quit(renderer);
	SDL_Quit();
	return 0;
}
