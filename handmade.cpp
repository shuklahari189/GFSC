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
gameUpdateAndRender(gameMemory *memory, gameInput *input, gameOffScreenBuffer *buffer, gameSoundOutputBuffer *soundBuffer)
{
    ASSERT(sizeof(gameState) <= memory->permanentStorageSize);
    gameState *gameSt = (gameState *)memory->permanentStorage;
    if (!memory->isInitialized)
    {
        gameSt->toneHz = 256;

        // TODO: more app.. to be done in platform layer
        memory->isInitialized = true;
    }

    gameControllerInput *input0 = &input->controllers[0];
    if (input0->isAnalog)
    {
        gameSt->xOffset += (int)(4.0f * input0->endX);
        gameSt->toneHz = 256 + (int)(120.0f * input0->endY);
    }
    else
    {
    }

    // input.aButtonEndedDown;
    // input.aButtonHalfTransitionCount;
    if (input0->down.endedDown)
    {
        gameSt->xOffset += 1;
    }

    win32gameOutputSound(soundBuffer, gameSt->toneHz);
    renderWeirdGradient(buffer, gameSt->xOffset, gameSt->yOffset);
}