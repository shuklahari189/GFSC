#include "handmade.h"

internal void
gameOutputSound(game_sound_output_buffer *soundBuffer, int toneHz)
{
    local_persist real32 tSine;
    int16 toneVolume = 3000;
    int wavePeriod = soundBuffer->samplesPerSecond / toneHz;

    int16 *sampleOut = (int16 *)soundBuffer->samples;
    for (int sampleIndex = 0; sampleIndex < soundBuffer->sampleCount; sampleIndex++)
    {
        real32 sineValue = sinf(tSine);
        int16 sampleValue = (int16)(sineValue * toneVolume);
        *sampleOut++ = sampleValue;
        *sampleOut++ = sampleValue;
        tSine += 2.0f * Pi32 * ((real32)1.0f / (real32)wavePeriod);
        if (tSine > 2.0f * Pi32)
        {
            tSine -= 2.0f * Pi32;
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

internal void
gameUpdateAndRender(game_memory *memory, game_input *input, game_offscreen_buffer *buffer)
{
    ASSERT((&input->controllers[0].terminator - &input->controllers[0].buttons[0]) == (ARRAY_COUNT(input->controllers[0].buttons)))
    ASSERT(sizeof(game_state) <= memory->permanentStorageSize);

    game_state *gameState = (game_state *)memory->permanentStorage;
    if (!memory->isInitialized)
    {
        char *fileName = __FILE__;

        debug_read_file_result file = DEBUGPlatformReadEntireFile(fileName);
        if (file.contents)
        {
            DEBUGPlatformWriteEntireFile("test.out", file.contentSize, file.contents);
            DEBUGPlatformFreeFileMemory(file.contents);
        }

        gameState->toneHz = 512;

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

internal void
gameGetSoundSamples(game_memory *memory, game_sound_output_buffer *soundBuffer)
{
    game_state *gameState = (game_state *)memory->permanentStorage;
    gameOutputSound(soundBuffer, gameState->toneHz);
}