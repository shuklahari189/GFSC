#pragma once

struct gameOffScreenBuffer
{
    void *memory;
    int width;
    int height;
    int pitch;
};

internal void gameUpdateAndRender(gameOffScreenBuffer *buffer, int xOffset, int yOffset);