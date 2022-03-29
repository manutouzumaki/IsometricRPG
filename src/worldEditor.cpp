 
void AddEntityToMousePosition(GameState *gameState,
                              World *world, InputState *input,
                              Arena *chunkArena, Arena *entitiesArena,
                              Vec2 cameraP)
{

    Vec2 mouseP = Vec2{(f32)input->mouseX - WINDOW_WIDTH/2, (f32)input->mouseY - WINDOW_HEIGHT/2};
    Vec2 mouseWorldP = cameraP + (mouseP * gameState->pixelsToMeters);    
    
    Vec2 chunkP = {
        floorf(mouseWorldP.x / (CHUNK_SIZE*gameState->tileSizeInMeters)), 
        floorf(mouseWorldP.y / (CHUNK_SIZE*gameState->tileSizeInMeters)), 
    };
    Vec2 mouseRelativeToChunk = mouseWorldP - (chunkP*CHUNK_SIZE);

    i32 chunkX = (i32)chunkP.x;
    i32 chunkY = (i32)chunkP.y;
    i32 chunkZ = 0;

    i32 tileX = (i32)floorf(mouseRelativeToChunk.x); 
    i32 tileY = (i32)floorf(mouseRelativeToChunk.y); 

    if(MouseOnClick(input->mouseLeft))
    {
        Chunk *chunk = GetChunkFromPosition(world, chunkX, chunkY, chunkZ);
        if(!chunk)
        {
            AddChunkToHashTable(world, chunkArena, chunkX, chunkY, chunkZ);
            chunk = GetChunkFromPosition(world, chunkX, chunkY, chunkZ);
        }

        if(chunk)
        {
            if(chunk->entities.count < ArrayCount(chunk->entities.data))
            {
                // Add entities...
                Entity *entity = &chunk->entities.data[chunk->entities.count++];
                entity->position.x = (f32)(chunkX*CHUNK_SIZE + tileX);
                entity->position.y = (f32)(chunkY*CHUNK_SIZE + tileY);
                entity->dimensions.x = gameState->tileSizeInMeters;
                entity->dimensions.y = gameState->tileSizeInMeters;
            }
        }
    }

}


void DrawMap(GameBackBuffer *backBuffer, GameState *gameState, World *world, Bitmap *bitmap,
             InputState *input)
{
    i32 cameraChunkX = (i32)floorf(gameState->cameraP.x / (CHUNK_SIZE*gameState->tileSizeInMeters));
    i32 cameraChunkY = (i32)floorf(gameState->cameraP.y / (CHUNK_SIZE*gameState->tileSizeInMeters));
    
    i32 minX = cameraChunkX - DISTANCE_TO_RENDER;
    i32 maxX = cameraChunkX + DISTANCE_TO_RENDER;
    i32 minY = cameraChunkY - DISTANCE_TO_RENDER;
    i32 maxY = cameraChunkY + DISTANCE_TO_RENDER;

    for(i32 y = minY; y < maxY; ++y)
    {
        for(i32 x = minX; x < maxX; ++x)
        {
            Chunk *chunk = GetChunkFromPosition(world, x, y, 0);
            if(chunk)
            {
                for(i32 i = 0; i < chunk->entities.count; ++i)
                {
                    Entity entity = chunk->entities.data[i];
                    Vec2 pos = entity.position - gameState->cameraP;
                    Vec2 tileIsometricPosition = MapTileToIsometric(pos.x, pos.y);
                    DrawBitmapVeryVeryFast(backBuffer, bitmap, tileIsometricPosition.x, tileIsometricPosition.y, 128, 128);

                }
            }
        }
    }


    Vec2 pos = Vec2{(f32)input->mouseX, (f32)input->mouseY};
    pos = MapIsometricToTile(pos.x, pos.y);

    Vec2 tileIsometricPosition = MapTileToIsometric(pos.x, pos.y);
    DrawBitmapVeryVeryFast(backBuffer, bitmap, tileIsometricPosition.x, tileIsometricPosition.y, 128, 128);



}


#if 0
void DrawMap(GameBackBuffer *backBuffer, GameState *gameState, World *world)
{
    i32 cameraChunkX = (i32)floorf(gameState->cameraP.x / (CHUNK_SIZE*gameState->tileSizeInMeters));
    i32 cameraChunkY = (i32)floorf(gameState->cameraP.y / (CHUNK_SIZE*gameState->tileSizeInMeters));
    
    i32 minX = cameraChunkX - DISTANCE_TO_RENDER;
    i32 maxX = cameraChunkX + DISTANCE_TO_RENDER;
    i32 minY = cameraChunkY - DISTANCE_TO_RENDER;
    i32 maxY = cameraChunkY + DISTANCE_TO_RENDER;

    for(i32 y = minY; y < maxY; ++y)
    {
        for(i32 x = minX; x < maxX; ++x)
        {
            Chunk *chunk = GetChunkFromPosition(world, x, y, 0);
            if(chunk)
            {
                Vec2 pos = ((Vec2{(f32)x, (f32)y} * CHUNK_SIZE * gameState->tileSizeInMeters) - gameState->cameraP) * gameState->metersToPixels;
                DrawRectangle(backBuffer,
                              pos.x + WINDOW_WIDTH/2,
                              pos.y + WINDOW_HEIGHT/2,
                              pos.x + ((CHUNK_SIZE * gameState->tileSizeInMeters) * gameState->metersToPixels) + WINDOW_WIDTH/2,
                              pos.y + ((CHUNK_SIZE * gameState->tileSizeInMeters) * gameState->metersToPixels) + WINDOW_HEIGHT/2,
                              0xFF335533);

                for(i32 i = 0; i < chunk->entities.count; ++i)
                {
                    Entity entity = chunk->entities.data[i];
                    Vec2 pos = entity.position - gameState->cameraP;
                    DrawRectangle(backBuffer,
                                  pos.x * gameState->metersToPixels + WINDOW_WIDTH/2,
                                  pos.y * gameState->metersToPixels + WINDOW_HEIGHT/2,
                                  (pos.x + gameState->tileSizeInMeters) * gameState->metersToPixels + WINDOW_WIDTH/2,
                                  (pos.y + gameState->tileSizeInMeters) * gameState->metersToPixels + WINDOW_HEIGHT/2,
                                  0xFFFF00FF);
                }
            }
        }
    } 
}
#endif
