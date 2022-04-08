#include "platform.h"
#include "math.h"
#include <xmmintrin.h>
#include <emmintrin.h>

// TODO(manuto): delete, this is just for debugin on windows
////////////////////////////////////////////////////////////
#include <windows.h>                                      //
#include <stdio.h>                                        //
////////////////////////////////////////////////////////////

#include "world.cpp"

struct Rect2
{
    Vec2 position;
    Vec2 dimensions;
};

struct GameState
{
    Arena bitmapArena;
    Arena entitiesArena;
    Arena worldArena;

    Bitmap cursorBitmap;
    Bitmap tileBitmap;
    Bitmap smallTileBitmap;
    Bitmap entityBitmap;
    Bitmap treeBitmap;
    Bitmap greenBitmap;
    Bitmap fontBitmap;

    // NOTE(manuto): World test...
    World world;

    // NOTE(manuto): camera test...
    Vec2 cameraP;
    EntityChunkP cameraChunkP;
    
    // NOTE(manuto): player test...
    EntityChunkP playerChunkP;
    Vec2 playerP;
    Vec2 playerDP;
    f32 playerW;
    f32 playerH;

    // NOTE(manuto): tilemap data
    f32 tileSizeInMeters;
    i32 tileSizeInPixels;
    f32 metersToPixels;
    f32 pixelsToMeters;
    
    u32 entityCount;
    Entity *entities;

    // Debug cycle counters ONLY fro
    u64 counters[CycleCounter_Count];
};

global_variable u64 *DEBUG_pointer;

global_variable i32 tilemapTest[] = {
    0, 0, 0, 1, 1, 0, 0, 0, 
    0, 1, 1, 1, 1, 1, 1, 0, 
    0, 1, 0, 1, 1, 0, 1, 0, 
    1, 1, 1, 1, 1, 1, 1, 1, 
    1, 1, 1, 1, 1, 1, 1, 1, 
    0, 1, 0, 1, 1, 0, 1, 0, 
    0, 1, 1, 1, 1, 1, 1, 0, 
    0, 0, 0, 1, 1, 0, 0, 0, 
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

internal
void RenderTextureQuad(GameBackBuffer *backBuffer, Bitmap *bitmap, f32 posX, f32 poxY, f32 width, f32 height, f32 angle = 0.0f)
{

    START_CYCLE_COUNTER(RenderTextureQuad);

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
        for(i32 x = (i32)minX; x < (maxX - 3); x += 4)
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
            
            if(_mm_movemask_epi8(texel))
            {
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
            }
            dst += 4;
        }
        row += backBuffer->width * BYTES_PER_PIXEL;
    }

    END_CYCLE_COUNTER(RenderTextureQuad);
}

internal
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

internal
void DrawBitmapVeryVeryFast(GameBackBuffer *backBuffer, Bitmap *bitmap, f32 x, f32 y, f32 width, f32 height)
{

    START_CYCLE_COUNTER(DrawBitmapVeryVeryFast);

    i32 minX = (i32)x;
    i32 minY = (i32)y;
    i32 maxX = (i32)x + (i32)width;
    i32 maxY = (i32)y + (i32)height;
    
    i32 offsetX = 0;
    i32 offsetY = 0;
    if(minX < 0)
    {
        offsetX = -minX;
        minX = 0;
    }
    if(maxX > backBuffer->width)
    {
        maxX = backBuffer->width;
    }
    if(minY < 0)
    {
        offsetY = -minY;
        minY = 0;
    }
    if(maxY > backBuffer->height)
    {
        maxY = backBuffer->height;
    }

    f32 ratioU = (f32)bitmap->width / width;
    f32 ratioV = (f32)bitmap->height / height;

    u32 *colorBuffer = (u32 *)backBuffer->memory;
    u32 *srcBuffer = (u32 *)bitmap->data + bitmap->width * (bitmap->height - 1);

    __m128i u255 = _mm_set1_epi32(0xFF);
    __m128 f255 = _mm_set1_ps(255.0f);
    __m128 one = _mm_set1_ps(1.0f);
    
    i32 counterY = offsetY;
    for(i32 y = minY; y < maxY; ++y)
    {
        i32 counterX = offsetX;
        for(i32 x = minX; x < (maxX - 3); x += 4)
        {
            u32 *dst = colorBuffer + (y * backBuffer->width + x);
            __m128i oldTexel = _mm_loadu_si128((__m128i *)dst);
            __m128i texel;
            
            i32 texY = (i32)(counterY * ratioV);
            
            i32 texX = (i32)((f32)(counterX + 0) * ratioU);
            Mu(texel, 0) = *(srcBuffer + (-texY * (i32)bitmap->width + texX));

            texX = (i32)((f32)(counterX + 1) * ratioU);
            Mu(texel, 1) = *(srcBuffer + (-texY * (i32)bitmap->width + texX));
            
            texX = (i32)((f32)(counterX + 2) * ratioU);
            Mu(texel, 2) = *(srcBuffer + (-texY * (i32)bitmap->width + texX));

            texX = (i32)((f32)(counterX + 3) * ratioU);
            Mu(texel, 3) = *(srcBuffer + (-texY * (i32)bitmap->width + texX));

            if(_mm_movemask_epi8(texel))
            {
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

                _mm_storeu_si128((__m128i *)dst, color);
            }
            counterX += 4; 
        }
        ++counterY;
    }

    END_CYCLE_COUNTER(DrawBitmapVeryVeryFast);
}

internal
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

internal
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

internal
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

internal 
Vec2 MapIsometricToTile(f32 c, f32 d)
{
    const f32 HW = 64;
    const f32 HH = 32;
    const f32 a = (WINDOW_WIDTH / 2) - HW;
    const f32 b = (WINDOW_HEIGHT / 2);
    
    f32 y = (d - b - ((c/HW)*HH) + ((a/HW)*HH)) / (2*HH);
    f32 x = (c/HW) - (a/HW) + y;  

    Vec2 result = {x, y};

    return result;
 
}

internal
u8 *GetFirstElement_(void *array, i32 count, i32 elementSize)
{
    u8 *result = (u8 *)array;
    result -= ((count - 1) * elementSize);
    return result;
}

#define GetFirstElement(array, count, type) (type *)GetFirstElement_((void *)(array), count, sizeof(type))



void RemapEntityChunkPosition(EntityChunkP *entity)
{
    if(entity->relP.x >= (f32)CHUNK_SIZE)
    {
        entity->relP.x -= (f32)CHUNK_SIZE;
        entity->chunkP.x += 1.0f;
    }
    if(entity->relP.y >= (f32)CHUNK_SIZE)
    {
        entity->relP.y -= (f32)CHUNK_SIZE;
        entity->chunkP.y += 1.0f;
    }
    if(entity->relP.x < 0.0f)
    {
        entity->relP.x += (f32)CHUNK_SIZE;
        entity->chunkP.x -= 1.0f;
    }
    if(entity->relP.y < 0.0f)
    {
        entity->relP.y += (f32)CHUNK_SIZE;
        entity->chunkP.y -= 1.0f;
    }
}

#include "worldEditor.cpp"

internal
void DrawPlayer(GameBackBuffer *backBuffer, EntityChunkP player, EntityChunkP camera, Bitmap *bitmap)
{
    Vec2 chunkDiff = player.chunkP - camera.chunkP;
    Vec2 pos = (chunkDiff*CHUNK_SIZE) - camera.relP + player.relP;
    Vec2 tileIsometricPosition = MapEntityToIsometric(pos.x, pos.y);
    DrawBitmapVeryVeryFast(backBuffer, bitmap, tileIsometricPosition.x - (32+16+8), tileIsometricPosition.y - (96+16), 128, 128);
}

EXPORT_TO_PLATORM 
GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    START_CYCLE_COUNTER(GameUpdateAndRender);

    GameState *gameState = (GameState *)memory->data;
    if(!memory->initialized)
    {
        memory->used = sizeof(GameState);
        
        InitArena(memory, Kilobyte(64), &gameState->worldArena);
        InitArena(memory, Kilobyte(96), &gameState->bitmapArena);
        InitArena(memory, Megabyte(128), &gameState->entitiesArena);
        
        DEBUG_pointer = gameState->counters;

        gameState->tileBitmap = DEBUG_LoadBitmap("../assets/tile.bmp", memory->DEBUG_ReadFile, &gameState->bitmapArena);
        gameState->entityBitmap = DEBUG_LoadBitmap("../assets/entity.bmp", memory->DEBUG_ReadFile, &gameState->bitmapArena);
        gameState->smallTileBitmap = DEBUG_LoadBitmap("../assets/small_tile.bmp", memory->DEBUG_ReadFile, &gameState->bitmapArena);
        gameState->treeBitmap = DEBUG_LoadBitmap("../assets/tree.bmp", memory->DEBUG_ReadFile, &gameState->bitmapArena);
        gameState->cursorBitmap = DEBUG_LoadBitmap("../assets/cursor.bmp", memory->DEBUG_ReadFile, &gameState->bitmapArena);
        gameState->greenBitmap = DEBUG_LoadBitmap("../assets/green.bmp", memory->DEBUG_ReadFile, &gameState->bitmapArena);
        gameState->fontBitmap = DEBUG_LoadBitmap("../assets/font.bmp", memory->DEBUG_ReadFile, &gameState->bitmapArena);
   
        gameState->tileSizeInMeters = 1.0f;
        gameState->tileSizeInPixels = 8;
        gameState->metersToPixels = (f32)gameState->tileSizeInPixels / gameState->tileSizeInMeters;
        gameState->pixelsToMeters = gameState->tileSizeInMeters / (f32)gameState->tileSizeInPixels;
        
        gameState->cameraChunkP.chunkP.x = 0; 
        gameState->cameraChunkP.chunkP.y = 0; 
        gameState->cameraChunkP.relP.x = 0; 
        gameState->cameraChunkP.relP.y = 0; 
    
        gameState->cameraP.x = 0.0f;
        gameState->cameraP.y = 0.0f;

        gameState->playerP.x = 0.0f;
        gameState->playerP.y = 0.0f;
        gameState->playerW = gameState->tileSizeInMeters*0.2f;
        gameState->playerH = gameState->tileSizeInMeters*0.2f;

        gameState->entityCount = 0;

        // TODO(manuto): create entities for the tiles
        for(i32 y = 0; y < MAP_SIZE; ++y)
        {
            for(i32 x = 0; x < MAP_SIZE; ++x)
            {
                if(tilemapTest[(y%8)*8+(x%8)] == 0)
                {
                    Entity *entity = PushStruct(&gameState->entitiesArena, Entity);
                    entity->position.x = (f32)x;
                    entity->position.y = (f32)y;
                    entity->dimensions.x = gameState->tileSizeInMeters;
                    entity->dimensions.y = gameState->tileSizeInMeters;
                    entity->collides = 1;//(tilemapTest[(y%8)*8+(x%8)] == 0);
                    gameState->entities = entity;
                    ++gameState->entityCount;
                }
            }
        }

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

///////////////////////////////////////////////////
// TODO(manuto): Collision System
///////////////////////////////////////////////////

    START_CYCLE_COUNTER(CollisionCounter);
    Vec2 rayDirection = gameState->playerDP * input->deltaTime;

    i32 cameraChunkX = (i32)gameState->cameraChunkP.chunkP.x;
    i32 cameraChunkY = (i32)gameState->cameraChunkP.chunkP.y;
    i32 minX = cameraChunkX - 1;
    i32 maxX = cameraChunkX + 1;
    i32 minY = cameraChunkY - 1;
    i32 maxY = cameraChunkY + 1;
    
    for(i32 y = minY; y <= maxY; ++y)
    {
        for(i32 x = minX; x <= maxX; ++x)
        {
            Chunk *chunk = GetChunkFromPosition(&gameState->world, x, y, 0);
            i32 playerChunkX = (i32)gameState->playerChunkP.chunkP.x;
            i32 playerChunkY = (i32)gameState->playerChunkP.chunkP.y;
            if(chunk /*&&
               chunk->x == playerChunkX &&
               chunk->y == playerChunkY*/)
            {
                for(i32 j = 0; j < CHUNK_SIZE; ++j)
                {
                    for(i32 i = 0; i < CHUNK_SIZE; ++i)
                    {
                        if(chunk->tilemap.floors.data[j * CHUNK_SIZE + i] == 0)
                        {
                            EntityChunkP entityChunkP = {};
                            entityChunkP.chunkP.x = (f32)chunk->x;
                            entityChunkP.chunkP.y = (f32)chunk->y;
                            entityChunkP.relP.x = (f32)i;
                            entityChunkP.relP.y = (f32)j;

                            Vec2 entityPos = entityChunkP.chunkP*CHUNK_SIZE + entityChunkP.relP;

#if 1
                            Rect2 rect = {};
                            rect.position = {floorf(entityPos.x) * gameState->tileSizeInMeters - (gameState->playerW*0.5f), floorf(entityPos.y) * gameState->tileSizeInMeters - (gameState->playerH*0.5f)};
                            rect.dimensions = {gameState->tileSizeInMeters + gameState->playerW, gameState->tileSizeInMeters + gameState->playerH};
                            Vec2 contactNormal = {};
                    
                            f32 invDirX = 1.0f / rayDirection.x;
                            f32 invDirY = 1.0f / rayDirection.y;

                            f32 minX = (rect.position.x - gameState->playerP.x) * invDirX; 
                            f32 maxX = ((rect.position.x + rect.dimensions.x) - gameState->playerP.x) * invDirX;
                            
                            f32 minY = (rect.position.y - gameState->playerP.y) * invDirY; 
                            f32 maxY = ((rect.position.y + rect.dimensions.y) - gameState->playerP.y) * invDirY;
#else

                            Rect2 rect = {};
                            rect.position = {floorf(entityChunkP.relP.x) * gameState->tileSizeInMeters - (gameState->playerW*0.5f), floorf(entityChunkP.relP.y) * gameState->tileSizeInMeters - (gameState->playerH*0.5f)};
                            rect.dimensions = {gameState->tileSizeInMeters + gameState->playerW, gameState->tileSizeInMeters + gameState->playerH};
                            Vec2 contactNormal = {};
                    
                            f32 invDirX = 1.0f / rayDirection.x;
                            f32 invDirY = 1.0f / rayDirection.y;

                            f32 minX = (rect.position.x - gameState->playerChunkP.relP.x) * invDirX; 
                            f32 maxX = ((rect.position.x + rect.dimensions.x) - gameState->playerChunkP.relP.x) * invDirX;
                            
                            f32 minY = (rect.position.y - gameState->playerChunkP.relP.y) * invDirY; 
                            f32 maxY = ((rect.position.y + rect.dimensions.y) - gameState->playerChunkP.relP.y) * invDirY;
#endif
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
                                Vec2 delta = Vec2ElementMul(contactNormal, Vec2{fabsf(gameState->playerDP.x), fabsf(gameState->playerDP.y)}) * (1.0f - tMin);
                                gameState->playerDP = gameState->playerDP + delta;
                            }
                        }
                    }
                }
            }
        }
    }
    END_CYCLE_COUNTER(CollisionCounter);
    
    gameState->playerP = gameState->playerDP * input->deltaTime + gameState->playerP; 

    gameState->playerChunkP.relP = gameState->playerDP * input->deltaTime + gameState->playerChunkP.relP; 
    
    RemapEntityChunkPosition(&gameState->playerChunkP);

    gameState->cameraChunkP = gameState->playerChunkP;
        
    
    ClearScreen(backBuffer, 0xFF003333);
    
    DrawMap(backBuffer, gameState, &gameState->world, &gameState->tileBitmap, input);

    DrawPlayer(backBuffer, gameState->playerChunkP, gameState->cameraChunkP, &gameState->entityBitmap);

    AddEntityToMousePosition(gameState, &gameState->world, input,
                             &gameState->worldArena, &gameState->entitiesArena,
                             backBuffer, &gameState->greenBitmap);




/*
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
        //playerDDP = Vec2Rotate(playerDDP, DegToRad(-45.0f));  
        Vec2Normalize(&playerDDP);
    }
    playerDDP = playerDDP * acceleration;

    playerDDP = gameState->playerDP * -8.0f + playerDDP;

    gameState->playerDP = playerDDP * input->deltaTime + gameState->playerDP;

    Vec2 playerDelta = {};


///////////////////////////////////////////////////
// TODO(manuto): Collision System
///////////////////////////////////////////////////

    START_CYCLE_COUNTER(CollisionCounter);
    Vec2 rayDirection = gameState->playerDP * input->deltaTime;
    Entity *firstEntity = GetFirstElement(gameState->entities, gameState->entityCount, Entity);
    for(u32 index = 0; index < gameState->entityCount; ++index)
    {
        Entity *entity = firstEntity + index;
        f32 distance = Vec2LengthSq(entity->position - gameState->playerP);
        if(entity->collides && distance < 4.0f)
        {
            Rect2 rect = {};
            rect.position = {floorf(entity->position.x) * gameState->tileSizeInMeters - (gameState->playerW*0.5f), floorf(entity->position.y) * gameState->tileSizeInMeters - (gameState->playerH*0.5f)};
            rect.dimensions = {gameState->tileSizeInMeters + gameState->playerW, gameState->tileSizeInMeters + gameState->playerH};
            Vec2 contactNormal = {};
    
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
                Vec2 delta = Vec2ElementMul(contactNormal, Vec2{fabsf(gameState->playerDP.x), fabsf(gameState->playerDP.y)}) * (1.0f - tMin);
                gameState->playerDP = gameState->playerDP + delta;
            }  
        }
    }

    END_CYCLE_COUNTER(CollisionCounter);
    
    gameState->playerP = gameState->playerDP * input->deltaTime + gameState->playerP; 
    
    gameState->cameraP = gameState->playerP; 

    Vec2 cameraOffset = {(WINDOW_WIDTH*0.1f)*gameState->pixelsToMeters, (WINDOW_HEIGHT*0.1f)*gameState->pixelsToMeters};
    Vec2 playerInCameraSpace = (gameState->playerP - gameState->cameraP);

    Vec2 playerIsometricPosition = MapEntityToIsometric(playerInCameraSpace.x, playerInCameraSpace.y);

    ClearScreen(backBuffer, 0xFF003333);

    i32 cameraTileX = (i32)floorf(gameState->cameraP.x);
    i32 cameraTileY = (i32)floorf(gameState->cameraP.y);
    
    i32 minX = cameraTileX - DISTANCE_TO_RENDER;
    if(minX < 0)
    {
        minX = 0;
    }
    i32 maxX = cameraTileX + DISTANCE_TO_RENDER;
    if(maxX > MAP_SIZE)
    {
        maxX = MAP_SIZE;
    }
    i32 minY = cameraTileY - DISTANCE_TO_RENDER;
    if(minY < 0)
    {
        minY = 0;
    }
    i32 maxY = cameraTileY + DISTANCE_TO_RENDER;
    if(maxY > MAP_SIZE)
    {
        maxY = MAP_SIZE;
    }

    for(i32 y = minY; y < maxY; ++y)
    {
        for(i32 x = minX; x < maxX; ++x)
        {
            if(tilemapTest[(y%8)*8+(x%8)] == 1)
            {
                Vec2 entityPos = Vec2{(f32)x, (f32)y};
                Vec2 pos = entityPos - gameState->cameraP;
                Vec2 tileIsometricPosition = MapTileToIsometric(pos.x, pos.y);
                DrawBitmapVeryVeryFast(backBuffer, &gameState->tileBitmap, tileIsometricPosition.x, tileIsometricPosition.y, 128, 128);
            }
        }
    }
    DrawBitmapVeryVeryFast(backBuffer, &gameState->entityBitmap, playerIsometricPosition.x - (32+16+8), playerIsometricPosition.y - (96+16), 128, 128);

#if 0  
    for(u32 index = 0; index < gameState->entityCount; ++index)
    {
        Entity *entity = firstEntity + index;
        if(!entity->collides)
        {
            f32 distance = Vec2LengthSq(entity->position - gameState->playerP);
            if(distance < DISTANCE_TO_RENDER)
            {
                Vec2 pos = (Vec2{entity->position.x * (f32)gameState->tileSizeInMeters, entity->position.y * (f32)gameState->tileSizeInMeters} - gameState->cameraP) + Vec2{16, 16} * gameState->tileSizeInMeters;
                DrawRectangle(backBuffer, pos.x*gameState->metersToPixels, pos.y*gameState->metersToPixels,
                              (pos.x + gameState->tileSizeInMeters)*gameState->metersToPixels,
                              (pos.y + gameState->tileSizeInMeters)*gameState->metersToPixels,
                              0xFF00FF00);
            }   
        }
    }
#else
        for(i32 y = minY; y < maxY; ++y)
        {
            for(i32 x = minX; x < maxX; ++x)
            {

                if(tilemapTest[(y%8)*8+(x%8)] == 1)
                {
                    Vec2 entityPos = Vec2{(f32)x, (f32)y};
                    f32 distance = Vec2LengthSq(entityPos - gameState->playerP);
                    if(distance < RADIO_TO_CHECK)
                    {
                        Vec2 pos = (Vec2{entityPos.x * (f32)gameState->tileSizeInMeters, entityPos.y * (f32)gameState->tileSizeInMeters} - gameState->cameraP) + Vec2{16, 16} * gameState->tileSizeInMeters;
                        DrawRectangle(backBuffer, pos.x*gameState->metersToPixels, pos.y*gameState->metersToPixels,
                                      (pos.x + gameState->tileSizeInMeters)*gameState->metersToPixels,
                                      (pos.y + gameState->tileSizeInMeters)*gameState->metersToPixels,
                                      0xFF00FF00);
                    }
                }
            }
        }
#endif


    Vec2 playerMiniMap = playerInCameraSpace + Vec2{16, 16} * gameState->tileSizeInMeters;
    DrawRectangle(backBuffer,
                 (playerMiniMap.x*gameState->metersToPixels) - (gameState->playerW*0.5f)*gameState->metersToPixels,
                 (playerMiniMap.y*gameState->metersToPixels) - (gameState->playerH*0.5f)*gameState->metersToPixels,
                 (playerMiniMap.x*gameState->metersToPixels) + (gameState->playerW*0.5f)*gameState->metersToPixels,
                 (playerMiniMap.y*gameState->metersToPixels) + (gameState->playerH*0.5f)*gameState->metersToPixels,
                 0xFFFF0000);

    
    END_CYCLE_COUNTER(GameUpdateAndRender);

#if 0  
    for(i32 i = 0; i < CycleCounter_Count; ++i)
    {
        char buffer[100];
        sprintf_s(buffer, "%d) cycle per frame: %I64u\n", i, gameState->counters[i]);
        OutputDebugString(buffer);
        gameState->counters[i] = 0;
    }
#endif
*/

}


///////////////////////////////////////////////////////////////////////////
// NOTE(manuto): Collision detection with SSE2                           //
///////////////////////////////////////////////////////////////////////////
#if 0
    __m128 one = _mm_set1_ps(1.0f);
    __m128 negativeOne = _mm_set1_ps(-1.0f);
    __m128 zero = _mm_set1_ps(0.0f);
    __m128 rayDirectionX = _mm_set1_ps(gameState->playerDP.x * input->deltaTime);
    __m128 rayDirectionY = _mm_set1_ps(gameState->playerDP.y * input->deltaTime);
    __m128 invDirX = _mm_div_ps(one, rayDirectionX);
    __m128 invDirY = _mm_div_ps(one, rayDirectionY);
    __m128 dimensionX = _mm_set1_ps(gameState->tileSizeInMeters + gameState->playerW);
    __m128 dimensionY = _mm_set1_ps(gameState->tileSizeInMeters + gameState->playerH);
    __m128 playerPX = _mm_set1_ps(gameState->playerP.x);
    __m128 playerPY = _mm_set1_ps(gameState->playerP.y);

    __m128 absPlayerDPX = _mm_set1_ps(fabsf(gameState->playerDP.x));
    __m128 absPlayerDPY = _mm_set1_ps(fabsf(gameState->playerDP.y));



    Entity *firstEntity = GetFirstElement(gameState->entities, gameState->entityCount, Entity);
    for(u32 index = 0; index < gameState->entityCount; index += 4)
    {
        Entity *entity0 = firstEntity + index;
        Entity *entity1 = firstEntity + index + 1;
        Entity *entity2 = firstEntity + index + 2;
        Entity *entity3 = firstEntity + index + 3;
        
        __m128 positionX = _mm_set_ps(
                    entity0->position.x * gameState->tileSizeInMeters - (gameState->playerW*0.5f),
                    entity1->position.x * gameState->tileSizeInMeters - (gameState->playerW*0.5f),
                    entity2->position.x * gameState->tileSizeInMeters - (gameState->playerW*0.5f),
                    entity3->position.x * gameState->tileSizeInMeters - (gameState->playerW*0.5f)
                ); 
        __m128 positionY = _mm_set_ps(
                    entity0->position.y * gameState->tileSizeInMeters - (gameState->playerH*0.5f),
                    entity1->position.y * gameState->tileSizeInMeters - (gameState->playerH*0.5f),
                    entity2->position.y * gameState->tileSizeInMeters - (gameState->playerH*0.5f),
                    entity3->position.y * gameState->tileSizeInMeters - (gameState->playerH*0.5f)
                ); 

        __m128 minX = _mm_mul_ps(_mm_sub_ps(positionX, playerPX), invDirX); 
        __m128 maxX = _mm_mul_ps(_mm_sub_ps(_mm_add_ps(positionX, dimensionX), playerPX), invDirX);

        __m128 minY = _mm_mul_ps(_mm_sub_ps(positionY, playerPY), invDirY); 
        __m128 maxY = _mm_mul_ps(_mm_sub_ps(_mm_add_ps(positionY, dimensionY), playerPY), invDirY);
        
        __m128i maskNan = _mm_castps_si128(_mm_and_ps(_mm_cmpord_ps(minY, minX), _mm_cmpord_ps(maxY, maxX)));

        __m128 temp = minX;
        minX = _mm_min_ps(minX, maxX);
        maxX = _mm_max_ps(temp, maxX);
        
        temp = minY;
        minY = _mm_min_ps(minY, maxY);
        maxY = _mm_max_ps(temp, maxY);
        
        __m128i maskMinMax = _mm_castps_si128(_mm_and_ps(_mm_cmple_ps(minX, maxY), _mm_cmple_ps(minY, maxX)));
        
        __m128 tMin = _mm_max_ps(minX, minY); 
        __m128 tMax = _mm_min_ps(maxX, maxY);
        
        __m128i tMaxMask = _mm_castps_si128(_mm_cmpge_ps(tMax, zero));

        __m128i mask = _mm_and_si128(maskNan, _mm_and_si128(maskMinMax, tMaxMask));
        if(_mm_movemask_epi8(mask))
        {     
            __m128 contactNormalX = _mm_set1_ps(0.0f);
            __m128 contactNormalY = _mm_set1_ps(0.0f);

            __m128 normalAX = one;
            __m128 normalAY = zero;

            __m128 normalBX = negativeOne;
            __m128 normalBY = zero;

            __m128 normalCX = zero;
            __m128 normalCY = one;

            __m128 normalDX = zero;
            __m128 normalDY = negativeOne;

            __m128i minXGraterMinY = _mm_castps_si128(_mm_cmpge_ps(minX, minY));
            __m128i minXLessMinY = _mm_castps_si128(_mm_cmple_ps(minX, minY));
            __m128i invDirXLessZero = _mm_castps_si128(_mm_cmple_ps(invDirX, zero));
            __m128i invDirXGraterZero = _mm_castps_si128(_mm_cmpge_ps(invDirX, zero));
            __m128i invDirYLessZero = _mm_castps_si128(_mm_cmple_ps(invDirY, zero));
            __m128i invDirYGraterZero = _mm_castps_si128(_mm_cmpge_ps(invDirY, zero));

            __m128i optionA = _mm_and_si128(minXGraterMinY, invDirXLessZero);
            __m128i optionB = _mm_and_si128(minXGraterMinY, invDirXGraterZero);
            __m128i optionC = _mm_and_si128(minXLessMinY, invDirYLessZero);
            __m128i optionD = _mm_and_si128(minXLessMinY, invDirYGraterZero);

            contactNormalX = _mm_add_ps(_mm_and_ps(normalAX, _mm_cvtepi32_ps(optionA)), contactNormalX);
            contactNormalY = _mm_add_ps(_mm_and_ps(normalAY, _mm_cvtepi32_ps(optionA)), contactNormalY);
            
            contactNormalX = _mm_add_ps(_mm_and_ps(normalBX, _mm_cvtepi32_ps(optionB)), contactNormalX);
            contactNormalY = _mm_add_ps(_mm_and_ps(normalBY, _mm_cvtepi32_ps(optionB)), contactNormalY);

            contactNormalX = _mm_add_ps(_mm_and_ps(normalCX, _mm_cvtepi32_ps(optionC)), contactNormalX);
            contactNormalY = _mm_add_ps(_mm_and_ps(normalCY, _mm_cvtepi32_ps(optionC)), contactNormalY);

            contactNormalX = _mm_add_ps(_mm_and_ps(normalDX, _mm_cvtepi32_ps(optionD)), contactNormalX);
            contactNormalY = _mm_add_ps(_mm_and_ps(normalDY, _mm_cvtepi32_ps(optionD)), contactNormalY);
            
            __m128i tMinMask = _mm_castps_si128(_mm_and_ps(_mm_cmpge_ps(tMin, zero), _mm_cmple_ps(tMin, one)));
            if(_mm_movemask_epi8(tMinMask))
            {
                __m128 playerDPX = _mm_and_ps(_mm_and_ps(_mm_mul_ps(_mm_mul_ps(contactNormalX, absPlayerDPX), _mm_sub_ps(one, tMin)), _mm_and_ps(_mm_cmpge_ps(tMin, zero), _mm_cmple_ps(tMin, one))), _mm_castsi128_ps(mask) ); 
                __m128 playerDPY = _mm_and_ps(_mm_and_ps(_mm_mul_ps(_mm_mul_ps(contactNormalY, absPlayerDPY), _mm_sub_ps(one, tMin)), _mm_and_ps(_mm_cmpge_ps(tMin, zero), _mm_cmple_ps(tMin, one))), _mm_castsi128_ps(mask) );
                
                i32 stopHere = 0;

                for(i32 i = 0; i < 4; ++i)
                {
                    if(M(playerDPX, i) != 0.0f || M(playerDPY, i) != 0.0f)
                    {
                        gameState->playerDP.x += M(playerDPX, i);
                        gameState->playerDP.y += M(playerDPY, i);
                    }
                }

            }


        }    
    }
#endif
