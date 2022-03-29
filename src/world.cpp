
struct ChunkEntities
{
    i32 count;
    Entity data[CHUNK_SIZE*CHUNK_SIZE];
    ChunkEntities *next;
};

struct Chunk
{
    i32 x;
    i32 y;
    i32 z;
    ChunkEntities entities;
    Chunk *next;
};


struct World
{
    u32 chunkCount;
    Chunk *chunksHashTable[4096];
};

u32 ChunkHashFunction(i32 x, i32 y, i32 z, u32 hashmapSize)
{
    // TODO(manuto): BETTER HASH FUNCTION... ;)
    u32 hashValueUnbound = abs(9*x + 5*y + 3*z);
    u32 hashValue = hashValueUnbound & (hashmapSize - 1);
    return hashValue;
  
}

void AddChunkToHashTable(World *world, Arena *arena, i32 x, i32 y, i32 z)
{
    u32 hashValue = ChunkHashFunction(x, y, z, ArrayCount(world->chunksHashTable)); 
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
            chunk = &(*chunk)->next;
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

Chunk *GetChunkFromPosition(World *world, i32 x, i32 y, i32 z)
{
    u32 hashValue = ChunkHashFunction(x, y, z, ArrayCount(world->chunksHashTable));
    Chunk *chunk = world->chunksHashTable[hashValue];
    while(chunk)
    {
        if(chunk->x == x && chunk->y == y && chunk->z == z)
        {
            break;
        }
        else
        {
            chunk = chunk->next;
        }  
    }
    return chunk;
}
