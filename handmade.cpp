#include "handmade.h"

internal void
gameOutputSound(game_state *gameState, game_sound_output_buffer *soundBuffer, int toneHz)
{
    int16 toneVolume = 3000;
    int wavePeriod = soundBuffer->samplesPerSecond / toneHz;

    int16 *sampleOut = (int16 *)soundBuffer->samples;
    for (int sampleIndex = 0; sampleIndex < soundBuffer->sampleCount; sampleIndex++)
    {
        real32 sineValue = sinf(gameState->tSine);
        int16 sampleValue = (int16)(sineValue * toneVolume);
        *sampleOut++ = sampleValue;
        *sampleOut++ = sampleValue;
        gameState->tSine += 2.0f * Pi32 * ((real32)1.0f / (real32)wavePeriod);
        if (gameState->tSine > 2.0f * Pi32)
        {
            gameState->tSine -= 2.0f * Pi32;
        }
    }
}

internal void
renderWieredGradiant(game_offscreen_buffer *buffer, int xOffset, int yOffset)
{
    uint8 *row = (uint8 *)buffer->memory;
    for (int y = 0; y < buffer->height; y++)
    {
        uint32 *pixel = (uint32 *)row;
        for (int x = 0; x < buffer->width; x++)
        {
            uint8 R = 0;
            uint8 G = (uint8)(y + yOffset);
            uint8 B = (uint8)(x + xOffset);
            *pixel++ = uint32(R << 16) | uint32(G << 8) | uint32(B);
        }
        row += buffer->pitch;
    }
}

extern "C" GAME_UPDATE_AND_RENDER(gameUpdateAndRender)
{
    ASSERT((&input->controllers[0].terminator - &input->controllers[0].buttons[0]) == (ARRAY_COUNT(input->controllers[0].buttons)))
    ASSERT(sizeof(game_state) <= memory->permanentStorageSize);

    game_state *gameState = (game_state *)memory->permanentStorage;
    if (!memory->isInitialized)
    {
        char *fileName = __FILE__;

        debug_read_file_result file = memory->DEBUGPlatformReadEntireFile(fileName);
        if (file.contents)
        {
            memory->DEBUGPlatformWriteEntireFile("test.out", file.contentSize, file.contents);
            memory->DEBUGPlatformFreeFileMemory(file.contents);
        }

        gameState->toneHz = 512;
        gameState->tSine = 0;

        memory->isInitialized = true;
    }
    for (int controllerIndex = 0; controllerIndex < ARRAY_COUNT(input->controllers); controllerIndex++)
    {
        game_controller_input *controller = getController(input, controllerIndex);

        if (controller->isAnalog)
        {
            gameState->xOffset += (int)(4.0f * controller->stickAverageX);
            gameState->toneHz = 512 + (int)(128.0f * controller->stickAverageY);
        }
        else
        {
            if (controller->moveLeft.endedDown)
            {
                gameState->xOffset -= 1;
            }
            if (controller->moveRight.endedDown)
            {
                gameState->xOffset += 1;
            }
        }

        if (controller->actionDown.endedDown)
        {
            gameState->toneHz -= 1;
        }
        if (controller->actionUp.endedDown)
        {
            gameState->toneHz += 1;
        }
    }
    renderWieredGradiant(buffer, gameState->xOffset, gameState->yOffset);
}

extern "C" GAME_GET_SOUND_SAMPLES(gameGetSoundSamples)
{
    game_state *gameState = (game_state *)memory->permanentStorage;
    gameOutputSound(gameState, soundBuffer, gameState->toneHz);
}

#if HANDMADE_WIN32
#include <windows.h>
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
    return TRUE;
}
#endif