 
void AddEntityToMousePosition(GameState *gameState,
                              World *world, InputState *input,
                              Arena *chunkArena, Arena *entitiesArena,
                              GameBackBuffer *backBuffer, Bitmap *bitmap)
{
    Vec2 mouseP = Vec2{(f32)((input->mouseX - 64)), (f32)((input->mouseY - 64))};
    Vec2 mouseRToIsometricP = MapIsometricToTile(mouseP.x, mouseP.y);

    EntityChunkP mouseChunkP = gameState->cameraChunkP;
    mouseChunkP.relP = Vec2Floor(mouseChunkP.relP + mouseRToIsometricP);
    RemapEntityChunkPosition(&mouseChunkP);
    
    i32 chunkX = (i32)mouseChunkP.chunkP.x;
    i32 chunkY = (i32)mouseChunkP.chunkP.y;
    i32 chunkZ = 0;

    f32 tileRelativeToChunkX = mouseChunkP.relP.x;
    f32 tileRelativeToChunkY = mouseChunkP.relP.y;

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
            chunk->tilemap.floors.data[(i32)tileRelativeToChunkY * CHUNK_SIZE + (i32)tileRelativeToChunkX] = 1;
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

    Vec2 chunkDiff = mouseChunkP.chunkP - gameState->cameraChunkP.chunkP;
    Vec2 pos = (chunkDiff*CHUNK_SIZE) - gameState->cameraChunkP.relP + mouseChunkP.relP;
    Vec2 tileIsometricPosition = MapTileToIsometric(pos.x, pos.y);
    DrawBitmapVeryVeryFast(backBuffer, bitmap, tileIsometricPosition.x, tileIsometricPosition.y, 128, 128);
}


void DrawMap(GameBackBuffer *backBuffer, GameState *gameState, World *world, Bitmap *bitmap,
             InputState *input)
{
    i32 cameraChunkX = (i32)gameState->cameraChunkP.chunkP.x;
    i32 cameraChunkY = (i32)gameState->cameraChunkP.chunkP.y;
    
    i32 minX = cameraChunkX - DISTANCE_TO_RENDER;
    i32 maxX = cameraChunkX + DISTANCE_TO_RENDER;
    i32 minY = cameraChunkY - DISTANCE_TO_RENDER;
    i32 maxY = cameraChunkY + DISTANCE_TO_RENDER;

    for(i32 y = minY; y <= maxY; ++y)
    {
        for(i32 x = minX; x <= maxX; ++x)
        {
            Chunk *chunk = GetChunkFromPosition(world, x, y, 0);
            if(chunk)
            {
                for(i32 j = 0; j < CHUNK_SIZE; ++j)
                {
                    for(i32 i = 0; i < CHUNK_SIZE; ++i)
                    {
                        if(chunk->tilemap.floors.data[j * CHUNK_SIZE + i] == 1)
                        {
                            EntityChunkP entityChunkP = {};
                            entityChunkP.chunkP.x = (f32)chunk->x;
                            entityChunkP.chunkP.y = (f32)chunk->y;
                            entityChunkP.relP.x = (f32)i;
                            entityChunkP.relP.y = (f32)j;
                        
                            Vec2 chunkDiff = entityChunkP.chunkP - gameState->cameraChunkP.chunkP;
                            Vec2 pos = (chunkDiff*CHUNK_SIZE) - gameState->cameraChunkP.relP + entityChunkP.relP;
                            Vec2 tileIsometricPosition = MapTileToIsometric(pos.x, pos.y);
                            DrawBitmapVeryVeryFast(backBuffer, bitmap, tileIsometricPosition.x, tileIsometricPosition.y, 128, 128);
                        }
                    }
                }

                /*
                for(i32 i = 0; i < chunk->entities.count; ++i)
                {
                    Entity entity = chunk->entities.data[i];

                    EntityChunkP entityChunkP = {};
                    entityChunkP.chunkP.x = (f32)chunk->x;
                    entityChunkP.chunkP.y = (f32)chunk->y;
                    entityChunkP.relP = entity.position;
                    
                    Vec2 chunkDiff = entityChunkP.chunkP - gameState->cameraChunkP.chunkP;
                    Vec2 pos = (chunkDiff*CHUNK_SIZE) - gameState->cameraChunkP.relP + entityChunkP.relP;
                    Vec2 tileIsometricPosition = MapTileToIsometric(pos.x, pos.y);
                    DrawBitmapVeryVeryFast(backBuffer, bitmap, tileIsometricPosition.x, tileIsometricPosition.y, 128, 128);
                }
                */

                Vec2 chunkDiff = Vec2{(f32)chunk->x, (f32)chunk->y} - gameState->cameraChunkP.chunkP;
                Vec2 posDiff = (chunkDiff*CHUNK_SIZE) - gameState->cameraChunkP.relP;
                Vec2 pos = (posDiff * gameState->tileSizeInMeters) + Vec2{16, 16} * gameState->tileSizeInMeters;
                DrawRectangle(backBuffer,
                              pos.x*gameState->metersToPixels,
                              pos.y*gameState->metersToPixels,
                              (pos.x + CHUNK_SIZE * gameState->tileSizeInMeters) * gameState->metersToPixels,
                              (pos.y + CHUNK_SIZE * gameState->tileSizeInMeters) * gameState->metersToPixels,
                              0xFF335533);
                /*
                for(i32 i = 0; i < chunk->entities.count; ++i)
                {

                    Entity entity = chunk->entities.data[i];

                    EntityChunkP entityChunkP = {};
                    entityChunkP.chunkP.x = (f32)chunk->x;
                    entityChunkP.chunkP.y = (f32)chunk->y;
                    entityChunkP.relP = entity.position;
                    
                    Vec2 chunkDiff = entityChunkP.chunkP - gameState->cameraChunkP.chunkP;
                    Vec2 posDiff = (chunkDiff*CHUNK_SIZE) - gameState->cameraChunkP.relP + entityChunkP.relP;
                    Vec2 pos = (posDiff * gameState->tileSizeInMeters) + Vec2{16, 16}  * gameState->tileSizeInMeters;
                    DrawRectangle(backBuffer, pos.x*gameState->metersToPixels, pos.y*gameState->metersToPixels,
                                  (pos.x + gameState->tileSizeInMeters)*gameState->metersToPixels,
                                  (pos.y + gameState->tileSizeInMeters)*gameState->metersToPixels,
                                  0xFF00FF00);
                } 
                */
            }
        }
    }

    
    Vec2 playerMiniMap = Vec2{16, 16}  * gameState->tileSizeInMeters;
    DrawRectangle(backBuffer,
                  (playerMiniMap.x*gameState->metersToPixels) - (gameState->playerW*0.5f)*gameState->metersToPixels,
                  (playerMiniMap.y*gameState->metersToPixels) - (gameState->playerH*0.5f)*gameState->metersToPixels,
                  (playerMiniMap.x*gameState->metersToPixels) + (gameState->playerW*0.5f)*gameState->metersToPixels,
                  (playerMiniMap.y*gameState->metersToPixels) + (gameState->playerH*0.5f)*gameState->metersToPixels,
                  0xFFFF0000);
}
