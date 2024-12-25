#include "handmade.h"

internal void
renderWeirdGradient(gameOffScreenBuffer *buffer, int xOffset, int yOffset)
{
    uint8 *row = (uint8 *)buffer->memory;
    for (int y = 0;
         y < buffer->height;
         y++)
    {
        uint32 *pixel = (uint32 *)row;
        for (int x = 0;
             x < buffer->width;
             x++)
        {
            //            red              green                      blue
            *pixel++ = (0) << 16 | (uint8(x + xOffset) << 8) | uint8(y + yOffset);
        }
        row += buffer->pitch;
    }
}

internal void
gameUpdateAndRender(gameOffScreenBuffer *buffer, int xOffset, int yOffset)
{
    renderWeirdGradient(buffer, xOffset, yOffset);
}