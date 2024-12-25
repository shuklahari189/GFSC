#pragma once

struct gameOffScreenBuffer
{
    void *memory;
    int width;
    int height;
    int pitch;
};

struct gameSoundOutputBuffer
{
    int16 *samples;
    int samplesPerSecond;
    int sampleCount;
};

internal void gameUpdateAndRender(gameOffScreenBuffer *buffer, gameSoundOutputBuffer *soundBuffer, int xOffset, int yOffset, int toneHz);