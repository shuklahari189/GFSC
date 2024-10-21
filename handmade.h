#pragma once
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

struct gameOffscreenBuffer
{
    void *memory;
    int height;
    int width;
    int pitch;
};

internal void gameUpdateAndRender(gameOffscreenBuffer *buffer, int xOffset, int yOffset);