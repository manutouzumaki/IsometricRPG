 
void AddEntityToMousePosition(GameState *gameState,
                              World *world, InputState *input,
                              Arena *chunkArena, Arena *entitiesArena,
                              Vec2 cameraP,
                              GameBackBuffer *backBuffer, Bitmap *bitmap)
{
    Vec2 cameraInIsometricSpace = MapEntityToIsometric(cameraP.x, cameraP.y);
    Vec2 mouseWP = cameraInIsometricSpace + Vec2{(f32)((input->mouseX - 64) - WINDOW_WIDTH*0.5f), (f32)((input->mouseY - 64) - WINDOW_HEIGHT*0.5f)};
    Vec2 mouseToIsometricP = MapIsometricToTile(mouseWP.x, mouseWP.y);

    Vec2 chunkP = mouseToIsometricP / CHUNK_SIZE;
    
    i32 chunkX = (i32)floorf(chunkP.x);
    i32 chunkY = (i32)floorf(chunkP.y);
    i32 chunkZ = 0;

    f32 tileRelativeToChunkX = floorf(mouseToIsometricP.x) - (chunkX*CHUNK_SIZE);
    f32 tileRelativeToChunkY = floorf(mouseToIsometricP.y) - (chunkY*CHUNK_SIZE);

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
                Entity *entity = &chunk->entities.data[chunk->entities.count++];
                entity->position.x = tileRelativeToChunkX;
                entity->position.y = tileRelativeToChunkY;
                entity->dimensions.x = gameState->tileSizeInMeters;
                entity->dimensions.y = gameState->tileSizeInMeters;
            }
        }
    }
    Vec2 position = {tileRelativeToChunkX, tileRelativeToChunkY};
    Vec2 pos = (Vec2{(f32)chunkX*CHUNK_SIZE, (f32)chunkY*CHUNK_SIZE} + position) - gameState->cameraP;
    Vec2 tileIsometricPosition = MapTileToIsometric(pos.x, pos.y);
    DrawBitmapVeryVeryFast(backBuffer, bitmap, tileIsometricPosition.x, tileIsometricPosition.y, 128, 128);

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
                    Vec2 pos = (Vec2{(f32)chunk->x*CHUNK_SIZE, (f32)chunk->y*CHUNK_SIZE} + entity.position) - gameState->cameraP;
                    Vec2 tileIsometricPosition = MapTileToIsometric(pos.x, pos.y);//*gameState->pixelsToMeters - gameState->cameraP;
                    DrawBitmapVeryVeryFast(backBuffer, bitmap, tileIsometricPosition.x, tileIsometricPosition.y, 128, 128);
                }

                

                Vec2 chunkPos = {(f32)(chunk->x*CHUNK_SIZE), (f32)(chunk->y*CHUNK_SIZE)};
                Vec2 pos = (chunkPos * gameState->tileSizeInMeters - gameState->cameraP) + Vec2{16, 16} * gameState->tileSizeInMeters;
                DrawRectangle(backBuffer,
                              pos.x*gameState->metersToPixels,
                              pos.y*gameState->metersToPixels,
                              (pos.x + CHUNK_SIZE * gameState->tileSizeInMeters) * gameState->metersToPixels,
                              (pos.y + CHUNK_SIZE * gameState->tileSizeInMeters) * gameState->metersToPixels,
                              0xFF335533);

                for(i32 i = 0; i < chunk->entities.count; ++i)
                {

                    Entity entity = chunk->entities.data[i];
                    Vec2 entityPos = Vec2{(f32)chunk->x*CHUNK_SIZE, (f32)chunk->y*CHUNK_SIZE} + entity.position;
                   

                    Vec2 pos = (entityPos * gameState->tileSizeInMeters - gameState->cameraP) + Vec2{16, 16} * gameState->tileSizeInMeters;
                    DrawRectangle(backBuffer, pos.x*gameState->metersToPixels, pos.y*gameState->metersToPixels,
                                  (pos.x + gameState->tileSizeInMeters)*gameState->metersToPixels,
                                  (pos.y + gameState->tileSizeInMeters)*gameState->metersToPixels,
                                  0xFF00FF00);
                } 
            }
        }
    }

    
    Vec2 playerMiniMap = Vec2{16, 16} * gameState->tileSizeInMeters;
    DrawRectangle(backBuffer,
                  (playerMiniMap.x*gameState->metersToPixels) - (gameState->playerW*0.5f)*gameState->metersToPixels,
                  (playerMiniMap.y*gameState->metersToPixels) - (gameState->playerH*0.5f)*gameState->metersToPixels,
                  (playerMiniMap.x*gameState->metersToPixels) + (gameState->playerW*0.5f)*gameState->metersToPixels,
                  (playerMiniMap.y*gameState->metersToPixels) + (gameState->playerH*0.5f)*gameState->metersToPixels,
                  0xFFFF0000);
}


#if 0

/*

                Vec2 pos =  (Vec2{(f32)chunk->x*CHUNK_SIZE, (f32)chunk->y*CHUNK_SIZE} + Vec2{16, 16} * gameState->tileSizeInMeters) * gameState->metersToPixels - gameState->cameraP;
                DrawRectangle(backBuffer,
                              pos.x,
                              pos.y,
                              pos.x + (CHUNK_SIZE * gameState->tileSizeInMeters * gameState->metersToPixels),
                              pos.y + (CHUNK_SIZE * gameState->tileSizeInMeters * gameState->metersToPixels),
                              0xFF335533);

                for(i32 i = 0; i < chunk->entities.count; ++i)
                {

                    Entity entity = chunk->entities.data[i];
                    Vec2 entityPos = Vec2{(f32)chunk->x*CHUNK_SIZE, (f32)chunk->y*CHUNK_SIZE} + entity.position;
                    
                    f32 distance = Vec2LengthSq(entityPos - gameState->cameraP * gameState->pixelsToMeters);
                    if(distance < DISTANCE_TO_RENDER)
                    {
                        Vec2 pos = ((Vec2{entityPos.x * (f32)gameState->tileSizeInMeters, entityPos.y * (f32)gameState->tileSizeInMeters} + Vec2{16, 16} * gameState->tileSizeInMeters) * gameState->metersToPixels) - gameState->cameraP;
                        DrawRectangle(backBuffer,
                                      pos.x,
                                      pos.y,
                                      pos.x + (gameState->tileSizeInMeters*gameState->metersToPixels),
                                      pos.y + (gameState->tileSizeInMeters*gameState->metersToPixels),
                                      0xFF00FF00);
                    }

                    Vec2 playerMiniMap = Vec2{16, 16} * gameState->tileSizeInMeters;
                    DrawRectangle(backBuffer,
                                  (playerMiniMap.x*gameState->metersToPixels) - (gameState->playerW*0.5f)*gameState->metersToPixels,
                                  (playerMiniMap.y*gameState->metersToPixels) - (gameState->playerH*0.5f)*gameState->metersToPixels,
                                  (playerMiniMap.x*gameState->metersToPixels) + (gameState->playerW*0.5f)*gameState->metersToPixels,
                                  (playerMiniMap.y*gameState->metersToPixels) + (gameState->playerH*0.5f)*gameState->metersToPixels,
                                  0xFFFF0000);

                    Entity entity = chunk->entities.data[i];
                    Vec2 pos = entity.position - gameState->cameraP;
                    DrawRectangle(backBuffer,
                                  pos.x * gameState->metersToPixels + WINDOW_WIDTH/2,
                                  pos.y * gameState->metersToPixels + WINDOW_HEIGHT/2,
                                  (pos.x + gameState->tileSizeInMeters) * gameState->metersToPixels + WINDOW_WIDTH/2,
                                  (pos.y + gameState->tileSizeInMeters) * gameState->metersToPixels + WINDOW_HEIGHT/2,
                                  0xFFFF00FF);
                                  
                }


*/







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
