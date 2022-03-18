#include "platform.h"
#include "math.h"

struct Rect2
{
    Vec2 position;
    Vec2 dimensions;
};

struct GameState
{
    Arena bitmapArena;

    Bitmap tileBitmap;
    Bitmap smallTileBitmap;
    Bitmap entityBitmap;
    Bitmap treeBitmap;

    // NOTE(manuto): camera test...
    Vec2 cameraP;
    
    // NOTE(manuto): player test...    
    i32 playerTileX;
    i32 playerTileY;
    Vec2 playerP;
    Vec2 playerDP;
    f32 playerW;
    f32 playerH;

    // NOTE(manuto): tilemap data
    f32 tileSizeInMeters;
    i32 tileSizeInPixels;
    f32 metersToPixels;
    f32 pixelsToMeters;
};


global_variable i32 tilemapTest[] = {

    0, 0, 0, 0, 0, 0, 0, 0, 
    0, 1, 1, 1, 1, 1, 1, 0, 
    0, 1, 1, 1, 0, 1, 1, 0, 
    0, 1, 1, 1, 1, 1, 1, 0, 
    0, 1, 1, 0, 1, 0, 1, 0, 
    0, 1, 1, 1, 1, 1, 1, 0, 
    0, 1, 1, 1, 1, 1, 1, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 

};

void SwapF32(f32 *a, f32 *b)
{
    f32 temp = *a;
    *a = *b;
    *b = temp;
}

inline
u32 BitScanForward(u32 mask)
{
    unsigned long shift = 0;
    _BitScanForward(&shift, mask);
    return (u32)shift;
}

internal
Bitmap DEBUG_LoadBitmap(char * fileName, debug_read_file *DEBUG_ReadFile, Arena *arena)
{
    Bitmap result = {};
    void *file = DEBUG_ReadFile(fileName, arena);
    if(file)
    {
        BitmapHeader *bmpHeader = (BitmapHeader *)file; 
        result.width = bmpHeader->width;
        result.height = bmpHeader->height;
        result.data = (u8 *)file + bmpHeader->bitmapOffset;

        u32 redShift = BitScanForward(bmpHeader->redMask);
        u32 greenShift = BitScanForward(bmpHeader->greenMask);
        u32 blueShift = BitScanForward(bmpHeader->blueMask);
        u32 alphaShift = BitScanForward(bmpHeader->alphaMask);
        
        u32 *colorData = (u32 *)result.data;
        for(u32 i = 0; i < result.width*result.height; ++i)
        {
            u32 red = (colorData[i] & bmpHeader->redMask) >> redShift;       
            u32 green = (colorData[i] & bmpHeader->greenMask) >> greenShift;       
            u32 blue = (colorData[i] & bmpHeader->blueMask) >> blueShift;       
            u32 alpha = (colorData[i] & bmpHeader->alphaMask) >> alphaShift;       
            colorData[i] = (alpha << 24) | (red << 16) | (green << 8) | (blue << 0);
        }
    }
    return result;
}

internal
void DrawRectangle(GameBackBuffer *backBuffer, f32 minX, f32 minY, f32 maxX, f32 maxY, u32 color)
{
    i32 minx = (i32)minX;
    i32 miny = (i32)minY;
    i32 maxx = (i32)maxX;
    i32 maxy = (i32)maxY;
    
    if(minx < 0)
    {
        minx = 0;
    }
    if(maxx > backBuffer->width)
    {
        maxx = backBuffer->width;
    }
    if(miny < 0)
    {
        miny = 0;
    }
    if(maxy > backBuffer->height)
    {
        maxy = backBuffer->height;
    }

    u32 offset = (miny * backBuffer->width + minx) * BYTES_PER_PIXEL;
    u8 *row = (u8 *)backBuffer->memory + offset;
    for(i32 y = miny; y < maxy; ++y)
    {
        u32 *col = (u32 *)row;
        for(i32 x = minx; x < maxx; ++x)
        {
            *col++ = color;
        }
        row += backBuffer->width * BYTES_PER_PIXEL;
    }
}

internal
void DrawBitmap(GameBackBuffer *backBuffer, Bitmap *bitmap, f32 x, f32 y)
{
    i32 minx = (i32)x;
    i32 miny = (i32)y;
    i32 maxx = (i32)x + bitmap->width;
    i32 maxy = (i32)y + bitmap->height;
    
    i32 offsetX = 0;
    i32 offsetY = 0;
    if(minx < 0)
    {
        offsetX = -minx;
        minx = 0;
    }
    if(maxx > backBuffer->width)
    {
        maxx = backBuffer->width;
    }
    if(miny < 0)
    {
        offsetY = -miny;
        miny = 0;
    }
    if(maxy > backBuffer->height)
    {
        maxy = backBuffer->height;
    }

    u32 dstOffset = (miny * backBuffer->width + minx) * BYTES_PER_PIXEL;
    u8 *dstRow = (u8 *)backBuffer->memory + dstOffset;
    
    i32 srcOffset = (-offsetY * bitmap->width + offsetX) * BYTES_PER_PIXEL; 
    u8 *srcRow = (u8 *)bitmap->data + (bitmap->width * (bitmap->height - 1)) * BYTES_PER_PIXEL;
    srcRow += srcOffset; 
       
    for(i32 y = miny; y < maxy; ++y)
    {
        u32 *dst = (u32 *)dstRow;
        u32 *src = (u32 *)srcRow;
        for(i32 x = minx; x < maxx; ++x)
        {
            // TODO(manuto): alpha blending
            f32 a = (f32)((*src >> 24) & 0xFF) / 255.0f;
            
            f32 srcR = (f32)((*src >> 16) & 0xFF);
            f32 srcG = (f32)((*src >>  8) & 0xFF);
            f32 srcB = (f32)((*src >>  0) & 0xFF);
            
            f32 dstR = (f32)((*dst >> 16) & 0xFF);
            f32 dstG = (f32)((*dst >>  8) & 0xFF);
            f32 dstB = (f32)((*dst >>  0) & 0xFF);

            f32 r = (1.0f - a)*dstR + a*srcR; 
            f32 g = (1.0f - a)*dstG + a*srcG; 
            f32 b = (1.0f - a)*dstB + a*srcB; 

            *dst = ((u32)(r + 0.5f) << 16) | ((u32)(g + 0.5f) << 8) | ((u32)(b + 0.5f) << 0);
            
            dst++;
            src++;
        }
        dstRow += backBuffer->width * BYTES_PER_PIXEL;
        srcRow -= bitmap->width * BYTES_PER_PIXEL;
    }
}

void RenderTextureQuad(GameBackBuffer *backBuffer, Bitmap *bitmap, f32 posX, f32 poxY, f32 width, f32 height, f32 angle = 0.0f)
{
    Vec2 position = {posX, poxY};
    Vec2 right = Vec2Rotate({1, 0}, angle);
    Vec2 down = Vec2Perp(right);
    
    right = right * width;
    down = down * height;

#if 0
    Vec2 vertices[4] = {
        position - right*0.5f - down*0.5f,
        position + right*0.5f - down*0.5f,
        position + right*0.5f + down*0.5f,
        position - right*0.5f + down*0.5f,
    };
#else
    Vec2 vertices[4] = {
        position,
        position + right,
        position + right + down,
        position + down,
    };
#endif

    // TODO: Create the bounding box arrown the quad
    // start with the x
    f32 minX = vertices[0].x;
    f32 maxX = vertices[0].x;
    f32 minY = vertices[0].y;
    f32 maxY = vertices[0].y;
    for(i32 i = 1; i < 4; ++i)
    {
        if(minX > vertices[i].x)
        {
            minX = vertices[i].x;
        }
        if(maxX < vertices[i].x)
        {
            maxX = vertices[i].x;
        }
        if(minY > vertices[i].y)
        {
            minY = vertices[i].y;
        }
        if(maxY < vertices[i].y)
        {
            maxY = vertices[i].y;
        }
    }
    
    Vec2 a = vertices[0];
    Vec2 b = vertices[1];
    Vec2 c = vertices[2];
    Vec2 d = vertices[3];

    Vec2 ab = b - a;
    Vec2 bc = c - b;
    Vec2 cd = d - c;
    Vec2 da = a - d;
    
    Vec2 aNormal = Vec2Norm(Vec2Perp(ab));
    Vec2 bNormal = Vec2Norm(Vec2Perp(bc));
    Vec2 cNormal = Vec2Norm(Vec2Perp(cd));
    Vec2 dNormal = Vec2Norm(Vec2Perp(da));

    if(minX < 0)
    {
        minX = 0;
    }
    if(maxX > backBuffer->width)
    {
        maxX = (f32)backBuffer->width;
    }
    if(minY < 0)
    {
        minY = 0;
    }
    if(maxY > backBuffer->height)
    {
        maxY = (f32)backBuffer->height;
    }

    f32 invWidth = 1.0f / Vec2LengthSq(right);
    f32 invHeight = 1.0f / Vec2LengthSq(down);

    u32 offset = ((u32)minY * backBuffer->width + (u32)minX) * BYTES_PER_PIXEL;
    u8 *row = (u8 *)backBuffer->memory + offset;
    for(i32 y = (i32)minY; y < maxY; ++y)
    {
        u32 *dst = (u32 *)row;
        for(i32 x = (i32)minX; x < maxX; ++x)
        {
            // TODO(manuto): check with all the normals to see 
            // if the pixel is inside the quad...
            Vec2 pixelPos = {(f32)x, (f32)y};
            if(Vec2Dot(aNormal, a - pixelPos) <= 0.0f &&
               Vec2Dot(bNormal, b - pixelPos) <= 0.0f &&
               Vec2Dot(cNormal, c - pixelPos) <= 0.0f &&
               Vec2Dot(dNormal, d - pixelPos) <= 0.0f)
            {
                Vec2 d = pixelPos - a;

                f32 u = (Vec2Dot(d, right)*invWidth);
                f32 v = (Vec2Dot(d, down)*invHeight);

                //Assert((u >= 0.0f) && (u <= 1.0f));
                //Assert((v >= 0.0f) && (v <= 1.0f));

                f32 texX = u * (f32)(bitmap->width);
                f32 texY = v * (f32)(bitmap->height);
                 
                i32 finalTexX = (i32)texX;
                i32 finalTexY = (i32)texY;

                f32 fx = texX - (f32)finalTexX;
                f32 fy = texY - (f32)finalTexY;

                //Assert((finalTexX >= 0) && (finalTexX < (i32)bitmap->width));
                //Assert((finalTexY >= 0) && (finalTexY < (i32)bitmap->height));

#if 1
                i32 offset = (-finalTexY * bitmap->width * BYTES_PER_PIXEL) + finalTexX*sizeof(u32);
                u8 *texel = ((u8 *)bitmap->data) + ((bitmap->width *(bitmap->height - 1)) * BYTES_PER_PIXEL);
                texel += offset;
#else
                u8 *texel = ((u8 *)bitmap->data) + finalTexY * bitmap->width * BYTES_PER_PIXEL + finalTexX*sizeof(u32);
#endif
#if 0              
                // NOTE(manuto): active this for bilinear filtering
                u32 *texelA = (u32 *)texel;
                u32 *texelB = (u32 *)(texel + sizeof(u32));
                u32 *texelC = (u32 *)(texel - (bitmap->width * BYTES_PER_PIXEL));
                u32 *texelD = (u32 *)(texel - (bitmap->width * BYTES_PER_PIXEL) + sizeof(u32));
                Vec4 texA = {
                    {(f32)((*texelA >> 16) & 0xFF)}, {(f32)((*texelA >> 8) & 0xFF)},
                    {(f32)((*texelA >> 0) & 0xFF)}, {(f32)((*texelA >> 24) & 0xFF)},
                };
                Vec4 texB = {
                    {(f32)((*texelB >> 16) & 0xFF)}, {(f32)((*texelB >> 8) & 0xFF)},
                    {(f32)((*texelB >> 0) & 0xFF)}, {(f32)((*texelB >> 24) & 0xFF)},
                };
                Vec4 texC = {
                    {(f32)((*texelC >> 16) & 0xFF)}, {(f32)((*texelC >> 8) & 0xFF)},
                    {(f32)((*texelC >> 0) & 0xFF)}, {(f32)((*texelC >> 24) & 0xFF)},
                };
                Vec4 texD = {
                    {(f32)((*texelD >> 16) & 0xFF)}, {(f32)((*texelD >> 8) & 0xFF)},
                    {(f32)((*texelD >> 0) & 0xFF)}, {(f32)((*texelD >> 24) & 0xFF)},
                };
                Vec4 finalColor = Vec4Lerp(Vec4Lerp(texA, texB, fx), Vec4Lerp(texC, texD, fx), fy);
                f32 a = finalColor.w / 255.0f;
                f32 srcR = finalColor.x;
                f32 srcG = finalColor.y;
                f32 srcB = finalColor.z;
#else
                f32 a = (f32)((*(u32 *)texel >> 24) & 0xFF) / 255.0f;
                f32 srcR = (f32)((*(u32 *)texel >> 16) & 0xFF);
                f32 srcG = (f32)((*(u32 *)texel >> 8) & 0xFF);
                f32 srcB = (f32)((*(u32 *)texel >> 0) & 0xFF);
#endif           
                f32 dstR = (f32)((*dst >> 16) & 0xFF);
                f32 dstG = (f32)((*dst >>  8) & 0xFF);
                f32 dstB = (f32)((*dst >>  0) & 0xFF);

                f32 r = (1.0f - a)*dstR + a*srcR; 
                f32 g = (1.0f - a)*dstG + a*srcG; 
                f32 b = (1.0f - a)*dstB + a*srcB; 

                *dst = ((u32)(r + 0.5f) << 16) | ((u32)(g + 0.5f) << 8) | ((u32)(b + 0.5f) << 0);
            }
            ++dst;
        }
        row += backBuffer->width * BYTES_PER_PIXEL;
    }
}

void DrawPixel(GameBackBuffer *backBuffer, f32 x, f32 y, u32 color)
{
    i32 px = (i32)x;
    i32 py = (i32)y;
    
    if(px < 0)
    {
        px = 0;
    }
    if(px >= backBuffer->width)
    {
        px = backBuffer->width - 1;
    }
    if(py < 0)
    {
        py = 0;
    }
    if(py >= backBuffer->height)
    {
        py = backBuffer->height - 1;
    }
    
    u32 *colorBuffer = (u32 *)backBuffer->memory + py * backBuffer->width + px;
    *colorBuffer = color;
}

Vec2 MapToIsometricTilemapTile(f32 x, f32 y)
{
    const f32 HALF_TILE_WIDTH = 64;
    const f32 HALF_TILE_HEIGHT = 32;
    const f32 startX = (WINDOW_WIDTH / 2) - HALF_TILE_WIDTH;
    const f32 startY = (WINDOW_HEIGHT / 2);

    f32 finalX = startX + (x * HALF_TILE_WIDTH) - (y * HALF_TILE_WIDTH);
    f32 finalY = startY + (x * HALF_TILE_HEIGHT) + (y * HALF_TILE_HEIGHT); 
    Vec2 result = { finalX, finalY };
    return result;
}

Vec2 MapToIsometricTilemap(f32 x, f32 y)
{
    const f32 HALF_TILE_WIDTH = 64;
    const f32 HALF_TILE_HEIGHT = 32;
    const f32 startX = (WINDOW_WIDTH / 2);
    const f32 startY = (WINDOW_HEIGHT / 2);

    f32 finalX = startX + (x * HALF_TILE_WIDTH) - (y * HALF_TILE_WIDTH);
    f32 finalY = startY + (x * HALF_TILE_HEIGHT) + (y * HALF_TILE_HEIGHT); 
    Vec2 result = { finalX, finalY };
    return result;
}

// TODO(manuto): delete, this is just for debugin on windows
////////////////////////////////////////////////////////////
#include <windows.h>                                      //
#include <stdio.h>                                        //
////////////////////////////////////////////////////////////

EXPORT_TO_PLATORM 
GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    GameState *gameState = (GameState *)memory->data;
    if(!memory->initialized)
    {
        memory->used = sizeof(GameState);
        
        InitArena(memory, Kilobyte(50), &gameState->bitmapArena);

        gameState->tileBitmap = DEBUG_LoadBitmap("../assets/tile.bmp", memory->DEBUG_ReadFile, &gameState->bitmapArena);
        gameState->entityBitmap = DEBUG_LoadBitmap("../assets/entity.bmp", memory->DEBUG_ReadFile, &gameState->bitmapArena);
        gameState->smallTileBitmap = DEBUG_LoadBitmap("../assets/small_tile.bmp", memory->DEBUG_ReadFile, &gameState->bitmapArena);
        gameState->treeBitmap = DEBUG_LoadBitmap("../assets/tree.bmp", memory->DEBUG_ReadFile, &gameState->bitmapArena);
   
        gameState->tileSizeInMeters = 1.0f;
        gameState->tileSizeInPixels = 32;
        gameState->metersToPixels = (f32)gameState->tileSizeInPixels / gameState->tileSizeInMeters;
        gameState->pixelsToMeters = gameState->tileSizeInMeters / (f32)gameState->tileSizeInPixels;

        gameState->cameraP.x = 2.0f;
        gameState->cameraP.y = 2.0f;

        gameState->playerP.x = 2.0f;
        gameState->playerP.y = 2.0f;
        gameState->playerW = gameState->tileSizeInMeters*0.2f;
        gameState->playerH = gameState->tileSizeInMeters*0.2f;

        memory->initialized = true;
    }
   
    Vec2 playerDDP = {}; 

    playerDDP.x = input->leftStickX;
    playerDDP.y = -input->leftStickY;
    if(input->up.isDown)
    {
        playerDDP.y = -1.0f; 
    }
    if(input->left.isDown)
    {
        playerDDP.x = -1.0f; 
    }
    if(input->down.isDown)
    {
        playerDDP.y = 1.0f; 
    }
    if(input->right.isDown)
    {
        playerDDP.x = 1.0f; 
    }
    
    const float acceleration = 20.0f;
    if(Vec2Length(playerDDP) > 0.0f)
    {
        playerDDP = Vec2Rotate(playerDDP, DegToRad(-45.0f));  
        Vec2Normalize(&playerDDP);
    }
    playerDDP = playerDDP * acceleration;

    playerDDP = gameState->playerDP * -8.0f + playerDDP;

    gameState->playerDP = playerDDP * input->deltaTime + gameState->playerDP;

    Vec2 playerDelta = {};

    // TODO(manuto): New Axis aligned collision detection
    for(i32 y = 0; y < 8; ++y)
    {
        for(i32 x = 0; x < 8; ++x)
        {
            if(tilemapTest[y * 8 + x] == 0)
            {
                Rect2 rect = {};
                rect.position = {(f32)x * gameState->tileSizeInMeters - (gameState->playerW*0.5f), (f32)y * gameState->tileSizeInMeters - (gameState->playerH*0.5f)};
                rect.dimensions = {gameState->tileSizeInMeters + gameState->playerW, gameState->tileSizeInMeters + gameState->playerH};
                Vec2 contactNormal = {};
                Vec2 rayDirection = gameState->playerDP * input->deltaTime;
    

                f32 invDirX = 1.0f / rayDirection.x;
                f32 invDirY = 1.0f / rayDirection.y;

                f32 minX = (rect.position.x - gameState->playerP.x) * invDirX; 
                f32 maxX = ((rect.position.x + rect.dimensions.x) - gameState->playerP.x) * invDirX;
                
                f32 minY = (rect.position.y - gameState->playerP.y) * invDirY; 
                f32 maxY = ((rect.position.y + rect.dimensions.y) - gameState->playerP.y) * invDirY;
                
                if (_isnanf(maxY) || _isnanf(maxX))
                {
                    continue;
                }
                if (_isnanf(minY) || _isnanf(minX))
                {
                    continue;
                }

                if(minX > maxX) SwapF32(&minX, &maxX);
                if(minY > maxY) SwapF32(&minY, &maxY);

                if(minX > maxY || minY > maxX)
                {
                    continue;
                }
                
                f32 tMin = minX > minY ? minX : minY; 
                f32 tMax = maxX < maxY ? maxX : maxY;
                
                if(tMax < 0)
                {
                    continue;
                }

                if(minX > minY)
                {
                    if(invDirX < 0.0f)
                    {
                        contactNormal = Vec2{1.0f, 0.0f};
                    }
                    else
                    {
                        contactNormal = Vec2{-1.0f, 0.0f};
                    }
                }
                else if(minX < minY)
                {
                    if(invDirY < 0.0f)
                    {
                        contactNormal = Vec2{0.0f, 1.0f}; 
                    }
                    else
                    {
                        contactNormal = Vec2{0.0f, -1.0f}; 
                    }
                }
                else
                {
                    if(invDirX == invDirY)
                    {
                        if(invDirX < 0.0f && invDirY < 0.0f)
                        {
                            contactNormal = Vec2{1.0f, 1.0f};
                        }
                        else if(invDirX > 0.0f && invDirY > 0.0f)
                        {
                            contactNormal = Vec2{-1.0f, -1.0f};
                        }
                    }
                    else if(invDirX == -invDirY)
                    {
                        if(invDirX < 0.0f && invDirY > 0.0f)
                        {
                            contactNormal = Vec2{1.0f, -1.0f};
                        }
                        else if(invDirX > 0.0f && invDirY < 0.0f)
                        {
                            contactNormal = Vec2{-1.0f, 1.0f};
                        }
                    }
                }

                if(tMin >= 0.0f && tMin < 1.0f)
                {
                    gameState->playerDP = gameState->playerDP + Vec2ElementMul(contactNormal, Vec2{fabsf(gameState->playerDP.x), fabsf(gameState->playerDP.y)}) * (1.0f - tMin);
                }
            }
        }
    }
    
    gameState->playerP = gameState->playerDP * input->deltaTime + gameState->playerP; 
    
    gameState->cameraP = gameState->playerP;

    Vec2 cameraOffset = {(WINDOW_WIDTH*0.1f)*gameState->pixelsToMeters, (WINDOW_HEIGHT*0.1f)*gameState->pixelsToMeters};
    Vec2 playerInCameraSpace = (gameState->playerP - gameState->cameraP);

    Vec2 playerIsometricPosition = MapToIsometricTilemap(playerInCameraSpace.x, playerInCameraSpace.y);

    u32 *colorBuffer = (u32 *)backBuffer->memory;
    for(i32 y  = 0; y < backBuffer->height; ++y)
    {
        for(i32 x = 0; x < backBuffer->width; ++x)
        {
            colorBuffer[y * backBuffer->width + x] = 0xFF00AAAA;
        }
    }



    for(i32 y = 0; y < 8; ++y)
    {
        for(i32 x = 0; x < 8; ++x)
        {
            if(tilemapTest[y * 8 + x] == 1)
            {
                Vec2 pos = (Vec2{(f32)x, (f32)y} - gameState->cameraP);
                Vec2 tileIsometricPosition = MapToIsometricTilemapTile(pos.x, pos.y);
                RenderTextureQuad(backBuffer, &gameState->smallTileBitmap, tileIsometricPosition.x, tileIsometricPosition.y, 128, 64);
            }
        }
    }
    RenderTextureQuad(backBuffer, &gameState->entityBitmap, playerIsometricPosition.x - (32+16+8), playerIsometricPosition.y - (96+16), 128, 128);
    
    for(i32 y = 0; y < 8; ++y)
    {
        for(i32 x = 0; x < 8; ++x)
        {
            if(tilemapTest[y * 8 + x] == 1)
            {
                Vec2 pos = (Vec2{x * (f32)gameState->tileSizeInMeters, y * (f32)gameState->tileSizeInMeters} - gameState->cameraP) + Vec2{3, 3} * gameState->tileSizeInMeters;
                DrawRectangle(backBuffer, pos.x*gameState->metersToPixels, pos.y*gameState->metersToPixels,
                              (pos.x + gameState->tileSizeInMeters)*gameState->metersToPixels,
                              (pos.y + gameState->tileSizeInMeters)*gameState->metersToPixels,
                              0xFF00FF00);
            }
        }
    }
    Vec2 playerMiniMap = playerInCameraSpace + Vec2{3, 3} * gameState->tileSizeInMeters;
    DrawRectangle(backBuffer,
                 (playerMiniMap.x*gameState->metersToPixels) - (gameState->playerW*0.5f)*gameState->metersToPixels,
                 (playerMiniMap.y*gameState->metersToPixels) - (gameState->playerH*0.5f)*gameState->metersToPixels,
                 (playerMiniMap.x*gameState->metersToPixels) + (gameState->playerW*0.5f)*gameState->metersToPixels,
                 (playerMiniMap.y*gameState->metersToPixels) + (gameState->playerH*0.5f)*gameState->metersToPixels,
                 0xFFFF0000);
    
}
