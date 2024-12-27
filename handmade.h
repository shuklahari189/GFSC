#pragma once

/*
    HANDMADE_INTERNAL:
        0 - build for public release
        1 - build for developer only
    HANDMADE_SLOW:
        0 - not slow code allowed
        1 - slow code allowed
*/

#if HANDMADE_SLOW
#define ASSERT(expression) \
    if (!(expression))     \
    {                      \
        *(int *)0 = 0;     \
    }
#else
#define ASSERT(expression)
#endif

#define ARRAY_COUNT(array) (sizeof(array) / sizeof((array)[0]))
#define KILOBYTES(value) ((value) * 1024LL)
#define MEGABYTES(value) (KILOBYTES(value) * 1024LL)
#define GIGABYTES(value) (MEGABYTES(value) * 1024LL)
#define TERABYTES(value) (GIGABYTES(value) * 1024LL)

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

struct gameMemory
{
    bool32 isInitialized;
    uint64 permanentStorageSize;
    void *permanentStorage; // required to be zeroed at startup
    uint64 transientStorageSize;
    void *transientStorage;
};

internal void gameUpdateAndRender(gameMemory *memory, gameInput *input, gameOffScreenBuffer *buffer, gameSoundOutputBuffer *soundBuffer);

struct gameState
{
    int toneHz;
    int xOffset;
    int yOffset;
};