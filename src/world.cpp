
struct Chunk
{
    i32 x, y, z;
    Chunk *next;
};


struct World
{
    u32 chunkCount;
    Chunk *chunksHashTable[4096];
};



void AddChunkToHashTable(World *world, Arena *arena, i32 x, i32 y, i32 z)
{
    // TODO(manuto): BETTER HASH FUNCTION... ;)
    u32 hashValueUnbound = 9*x + 5*y + 3*z;
    u32 hashValue = hashValueUnbound & (ArrayCount(world->chunksHashTable) - 1);
    
    Chunk **chunk = &world->chunksHashTable[hashValue];
    if(!*chunk)
    {
        *chunk = PushStruct(arena, Chunk);
        (*chunk)->x = x;
        (*chunk)->y = y;
        (*chunk)->z = z;
        (*chunk)->next = 0;
        ++world->chunkCount;
        OutputDebugString("Chunck Added NO collision\n");
    }
    else
    {
        while(*chunk &&
              ((*chunk)->x != x || (*chunk)->y != y || (*chunk)->z != z))
        {
            OutputDebugString("Collision!\n");
            *chunk = (*chunk)->next;
        }
        if(!*chunk)
        {
            *chunk = PushStruct(arena, Chunk);
            (*chunk)->x = x;
            (*chunk)->y = y;
            (*chunk)->z = z; 
            (*chunk)->next = 0;
            ++world->chunkCount;
            OutputDebugString("Chunck Added YES collision\n");
        }
        else
        {
            // chunk already on the hashmap
            OutputDebugString("Chunk Already on the hashMap!\n");
            return;
        } 
    }
}
