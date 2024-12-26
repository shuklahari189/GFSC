#include "handmade.h"

internal void
win32gameOutputSound(gameSoundOutputBuffer *soundBuffer, int toneHz)
{
    local_persit real32 tSine;
    int16 toneVolume = 3000;
    int wavePeriod = soundBuffer->samplesPerSecond / toneHz;

    int16 *sampleOut = soundBuffer->samples;
    for (int sampleIndex = 0; sampleIndex < soundBuffer->sampleCount; sampleIndex++)
    {
        real32 sinValue = sinf(tSine);
        int16 sampleValue = (int16)(sinValue * toneVolume);
        *sampleOut++ = sampleValue;
        *sampleOut++ = sampleValue;
        tSine += ((real32)1.0f / (real32)wavePeriod) * 2.0f * Pi32;
    }
}

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
            //          red         green         blue
            *pixel++ = (0 << 16) | (uint8(y + yOffset) << 8) | uint8(x + xOffset);
        }
        row += buffer->pitch;
    }
}

internal void
gameUpdateAndRender(gameInput *input, gameOffScreenBuffer *buffer, gameSoundOutputBuffer *soundBuffer)
{
    local_persit int toneHz = 256;
    local_persit int xOffset = 0;
    local_persit int yOffset = 0;

    gameControllerInput *input0 = &input->controllers[0];
    if (input0->isAnalog)
    {
        xOffset += (int)(4.0f * input0->endX);
        toneHz = 256 + (int)(120.0f * input0->endY);
    }
    else
    {
    }

    // input.aButtonEndedDown;
    // input.aButtonHalfTransitionCount;
    if (input0->down.endedDown)
    {
        xOffset += 1;
    }

    win32gameOutputSound(soundBuffer, toneHz);
    renderWeirdGradient(buffer, xOffset, yOffset);
}