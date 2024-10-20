#include "handmade.h"

static void win32_fillDisplayBuffer(game_DisplayBuffer *buffer)
{
    uint8 *row = (uint8 *)buffer->memory;
    for (int y = 0; y < buffer->height; y++)
    {
        uint32 *pixel = (uint32 *)row;
        for (int x = 0; x < buffer->width; x++)
        {
            uint8 r = 255;
            uint8 g = 0;
            uint8 b = 0;
            *pixel++ = (r << 16) | (g << 8) | b;
        }
        row += buffer->pitch;
    }
}

static void
gameUpdateAndRender(game_DisplayBuffer *buffer)
{
    
    win32_fillDisplayBuffer(buffer);
}