#include "defines.h"
#include <intrin.h>

struct Arena
{
    u8 *base;
    size_t used;
    size_t size;
};

#define DEBUG_READFILE(name) void *name(char * fileName, Arena *arena)
typedef DEBUG_READFILE(debug_read_file);

struct GameMemory
{
    void *data;
    size_t used;
    size_t size;
    b8 initialized;

    debug_read_file *DEBUG_ReadFile;
};

void *PushStruct_(Arena *arena, size_t size)
{
    Assert(arena->used + size <= arena->size);
    void *result = (void *)(arena->base + arena->used);
    arena->used += size;
    return result;
}

void InitArena(GameMemory *memory, size_t size, Arena *arena)
{
    Assert(memory->used + size <= memory->size);
    arena->base = (u8 *)memory->data + memory->used;
    arena->size = size;
    arena->used = 0;
    memory->used += size;
    
    // NOTE: clear the arena to zero
    for(size_t i = 0; i < size; ++i)
    {
        arena->base[i] = 0;
    }
}

#define PushStruct(arena, type) (type *)PushStruct_(arena, sizeof(type))
#define PushArray(arena, type, count) (type *)PushStruct_(arena, sizeof(type)*(count))

struct GameBackBuffer
{
    void *memory;
    i32 width;
    i32 height;
};

#pragma pack(push, 1)
struct BitmapHeader
{
    u16 fileType;
    u32 fileSize;
    u16 reserved1;
    u16 reserved2;
    u32 bitmapOffset;
	u32 size;             
	i32 width;            
    i32 height;           
	u16 planes;           
	u16 bitsPerPixel;    
	u32 compression;      
	u32 sizeOfBitmap;     
	i32 horzResolution;  
	i32 vertResolution;  
	u32 colorsUsed;       
	u32 colorsImportant;  
	u32 redMask;          
	u32 greenMask;        
	u32 blueMask;         
	u32 alphaMask;        
};
#pragma pack(pop)

struct Bitmap
{
    void *data;
    u32 width;
    u32 height;
};


struct ButtonState
{
    b8 isDown;
    b8 wasDown;
};

struct InputState
{
    f32 deltaTime;
    f32 leftStickX;
    f32 leftStickY;
    f32 rightStickX;
    f32 rightStickY;
    union
    {
        struct
        {
            ButtonState up;   
            ButtonState down;
            ButtonState left;
            ButtonState right;
            ButtonState start;
            ButtonState back;
            ButtonState a;
            ButtonState b;
            ButtonState x;
            ButtonState y;
        }; 
        ButtonState buttons[10];
    };
};

#define GAME_UPDATE_AND_RENDER(name) void name(GameMemory *memory, GameBackBuffer *backBuffer, InputState *input)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);

enum CycleCounter
{
    CycleCounter_GameUpdateAndRender,
    CycleCounter_RenderTextureQuad,
    CycleCounter_CollisionCounter,
    CycleCounter_DrawBitmapVeryVeryFast,
    CycleCounter_Count 
};

extern u64 *DEBUG_pointer;

#define START_CYCLE_COUNTER(id) u64 StartCycleCounter_##id = __rdtsc()
#define END_CYCLE_COUNTER(id) DEBUG_pointer[CycleCounter_##id] += (__rdtsc() - StartCycleCounter_##id) 
