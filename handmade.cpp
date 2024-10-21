#include "handmade.h"

// MAKE THIS O
#if 1
#include <math.h>
#include <stdint.h>

typedef int32_t bool32;
typedef int64_t int64;
typedef int32_t int32;
typedef int16_t int16;
typedef int8_t int8;
typedef uint64_t uint64;
typedef uint32_t uint32;
typedef uint16_t uint16;
typedef uint8_t uint8;
typedef float real32;
typedef double real64;

#define pi32 3.14159265359f

#define localPersist static
#define internal static
#define globalVariable static

#endif

internal void
renderWeirdGradient(gameOffscreenBuffer *buffer, int xOffset, int yOffset)
{

    uint8 *row = (uint8 *)buffer->memory;
    for (int y = 0; y < buffer->height; y++)
    {
        uint32 *pixel = (uint32 *)row;
        for (int x = 0; x < buffer->width; x++)
        {
            uint8 r = 0;
            uint8 g = (uint8)x + xOffset;
            uint8 b = (uint8)y + yOffset;
            *pixel++ = (r << 16) | (g << 8) | b;
        }
        row += buffer->pitch;
    }
}

internal void gameUpdateAndRender(gameOffscreenBuffer *buffer, int xOffset, int yOffset)
{
    renderWeirdGradient(buffer, xOffset, yOffset);
}