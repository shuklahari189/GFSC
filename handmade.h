#pragma once

#define arrayCount(array) (sizeof(array) / sizeof((array)[0]))

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

struct gameButtonState
{
    int halfTransitionCount;
    bool32 endedDown;
};

struct gameControllerInput
{
    bool32 isAnalog;

    real32 startX;
    real32 startY;

    real32 minX;
    real32 minY;

    real32 maxX;
    real32 maxY;

    real32 endX;
    real32 endY;

    union
    {
        gameButtonState Buttons[6];
        struct
        {
            gameButtonState up;
            gameButtonState down;
            gameButtonState left;
            gameButtonState right;
            gameButtonState leftShoulder;
            gameButtonState rightShoulder;
        };
    };
};

struct gameInput
{
    gameControllerInput controllers[4];
};

internal void gameUpdateAndRender(gameInput *input, gameOffScreenBuffer *buffer, gameSoundOutputBuffer *soundBuffer);