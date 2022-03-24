#include "platform.h"
#include "math.h"
#include <xmmintrin.h>
#include <emmintrin.h>

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

    Vec2 vertices[4] = {
        position,
        position + right*width,
        position + right*width + down*height,
        position + down*height,
    };

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


    u32 offset = ((u32)minY * backBuffer->width + (u32)minX) * BYTES_PER_PIXEL;
    u8 *row = (u8 *)backBuffer->memory + offset;
    
    __m128 positionX = _mm_set1_ps(position.x); 
    __m128 positionY = _mm_set1_ps(position.y);
    __m128 rightX = _mm_set1_ps(right.x); 
    __m128 rightY = _mm_set1_ps(right.y); 
    __m128 downX = _mm_set1_ps(down.x);
    __m128 downY = _mm_set1_ps(down.y);
    __m128 invWidth = _mm_set1_ps(1.0f / width);
    __m128 invHeight = _mm_set1_ps(1.0f / height);
    __m128 one = _mm_set1_ps(1.0f);
    __m128 zero = _mm_set1_ps(0.0f);
    __m128 f255 = _mm_set1_ps(255.0f);
    __m128i u255 = _mm_set1_epi32(0xFF);
    __m128 bitmapWidth = _mm_set1_ps((f32)bitmap->width);
    __m128 bitmapHeight = _mm_set1_ps((f32)bitmap->height);


    for(i32 y = (i32)minY; y < maxY; ++y)
    {
        u32 *dst = (u32 *)row;
        for(i32 x = (i32)minX; x < maxX; x += 4)
        {

            __m128i oldTexel = _mm_loadu_si128((__m128i *)dst);

            __m128 pixelPosX = _mm_set_ps((f32)(x + 3), (f32)(x + 2), (f32)(x + 1), (f32)x);
            __m128 pixelPosY = _mm_set1_ps((f32)y);

            __m128 dx = _mm_sub_ps(pixelPosX, positionX); 
            __m128 dy = _mm_sub_ps(pixelPosY, positionY); 

            __m128 u = _mm_mul_ps(_mm_add_ps(_mm_mul_ps(dx, rightX), _mm_mul_ps(dy, rightY)),  invWidth);
            __m128 v = _mm_mul_ps(_mm_add_ps(_mm_mul_ps(dx, downX), _mm_mul_ps(dy, downY)),  invHeight);
            
            __m128i mask = _mm_castps_si128(_mm_and_ps
                    (
                        _mm_and_ps(_mm_cmpge_ps(u, zero), _mm_cmple_ps(u, one)),
                        _mm_and_ps(_mm_cmpge_ps(v, zero), _mm_cmple_ps(v, one))
                    ));
            
            u = _mm_min_ps(_mm_max_ps(u, zero), one);
            v = _mm_min_ps(_mm_max_ps(v, zero), one);

            __m128 texX = _mm_mul_ps(u, bitmapWidth);
            __m128 texY = _mm_mul_ps(v, bitmapHeight);

            __m128i pixelX = _mm_cvttps_epi32(texX);
            __m128i pixelY = _mm_cvttps_epi32(texY);

            __m128i texel;

#define M(a, i) ((f32 *)&(a))[i]
#define Mi(a, i) ((i32 *)&(a))[i]
#define Mu(a, i) ((u32 *)&(a))[i]

            i32 offset = (-Mi(pixelY, 0) * bitmap->width * BYTES_PER_PIXEL) + Mi(pixelX, 0)*sizeof(u32);
            u8 *texelPtr = ((u8 *)bitmap->data) + ((bitmap->width *(bitmap->height - 1)) * BYTES_PER_PIXEL);
            texelPtr += offset;
            Mu(texel, 0) = *((u32 *)texelPtr);

            offset = (-Mi(pixelY, 1) * bitmap->width * BYTES_PER_PIXEL) + Mi(pixelX, 1)*sizeof(u32);
            texelPtr = ((u8 *)bitmap->data) + ((bitmap->width *(bitmap->height - 1)) * BYTES_PER_PIXEL);
            texelPtr += offset;
            Mu(texel, 1) = *((u32 *)texelPtr);

            offset = (-Mi(pixelY, 2) * bitmap->width * BYTES_PER_PIXEL) + Mi(pixelX, 2)*sizeof(u32);
            texelPtr = ((u8 *)bitmap->data) + ((bitmap->width *(bitmap->height - 1)) * BYTES_PER_PIXEL);
            texelPtr += offset;
            Mu(texel, 2) = *((u32 *)texelPtr);

            offset = (-Mi(pixelY, 3) * bitmap->width * BYTES_PER_PIXEL) + Mi(pixelX, 3)*sizeof(u32);
            texelPtr = ((u8 *)bitmap->data) + ((bitmap->width *(bitmap->height - 1)) * BYTES_PER_PIXEL);
            texelPtr += offset;
            Mu(texel, 3) = *((u32 *)texelPtr);
            

            __m128 a = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(texel, 24), u255));
            __m128 invA =_mm_div_ps(a, f255); 

            __m128 srcR = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(texel, 16), u255));
            __m128 srcG = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(texel, 8), u255));
            __m128 srcB = _mm_cvtepi32_ps(_mm_and_si128(texel, u255));

            __m128 dstR = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(oldTexel, 16), u255));
            __m128 dstG = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(oldTexel, 8), u255));
            __m128 dstB = _mm_cvtepi32_ps(_mm_and_si128(oldTexel, u255));

            __m128 r = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(one, invA), dstR), _mm_mul_ps(invA, srcR));
            __m128 g = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(one, invA), dstG), _mm_mul_ps(invA, srcG));
            __m128 b = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(one, invA), dstB), _mm_mul_ps(invA, srcB));
            
            __m128i color = _mm_or_si128(
                        _mm_or_si128(_mm_slli_epi32(_mm_cvtps_epi32(a), 24), _mm_slli_epi32(_mm_cvtps_epi32(r), 16)),
                        _mm_or_si128(_mm_slli_epi32(_mm_cvtps_epi32(g),  8), _mm_cvtps_epi32(b))
                    );

            __m128i maskedColor = _mm_or_si128(_mm_and_si128(mask, color), _mm_andnot_si128(mask, oldTexel));
            
            _mm_storeu_si128((__m128i *)dst, maskedColor);
            dst += 4;
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

void ClearScreen(GameBackBuffer *backBuffer, u32 color)
{
    u32 *colorBuffer = (u32 *)backBuffer->memory;
    __m128i pixelColor = _mm_set1_epi32(color);
    for(i32 i = 0; i < backBuffer->width*backBuffer->height; i += 4)
    {
        _mm_storeu_si128((__m128i *)colorBuffer, pixelColor);
        colorBuffer += 4;
    }
}

Vec2 MapTileToIsometric(f32 x, f32 y)
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

Vec2 MapEntityToIsometric(f32 x, f32 y)
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

    Vec2 playerIsometricPosition = MapEntityToIsometric(playerInCameraSpace.x, playerInCameraSpace.y);

    ClearScreen(backBuffer, 0xFF003333);

    for(i32 y = 0; y < 8; ++y)
    {
        for(i32 x = 0; x < 8; ++x)
        {
            if(tilemapTest[y * 8 + x] == 1)
            {
                Vec2 pos = (Vec2{(f32)x, (f32)y} - gameState->cameraP);
                Vec2 tileIsometricPosition = MapTileToIsometric(pos.x, pos.y);
                RenderTextureQuad(backBuffer, &gameState->tileBitmap, tileIsometricPosition.x, tileIsometricPosition.y, 128, 128);
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
                Vec2 pos = (Vec2{x * (f32)gameState->tileSizeInMeters, y * (f32)gameState->tileSizeInMeters} - gameState->cameraP) + Vec2{4, 4} * gameState->tileSizeInMeters;
                DrawRectangle(backBuffer, pos.x*gameState->metersToPixels, pos.y*gameState->metersToPixels,
                              (pos.x + gameState->tileSizeInMeters)*gameState->metersToPixels,
                              (pos.y + gameState->tileSizeInMeters)*gameState->metersToPixels,
                              0xFF00FF00);
            }
        }
    }
    Vec2 playerMiniMap = playerInCameraSpace + Vec2{4, 4} * gameState->tileSizeInMeters;
    DrawRectangle(backBuffer,
                 (playerMiniMap.x*gameState->metersToPixels) - (gameState->playerW*0.5f)*gameState->metersToPixels,
                 (playerMiniMap.y*gameState->metersToPixels) - (gameState->playerH*0.5f)*gameState->metersToPixels,
                 (playerMiniMap.x*gameState->metersToPixels) + (gameState->playerW*0.5f)*gameState->metersToPixels,
                 (playerMiniMap.y*gameState->metersToPixels) + (gameState->playerH*0.5f)*gameState->metersToPixels,
                 0xFFFF0000);
    
}
