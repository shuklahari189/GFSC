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

#define KILOBYTES(value) ((value) * 1024LL)
#define MEGABYTES(value) (KILOBYTES(value) * 1024LL)
#define GIGABYTES(value) (MEGABYTES(value) * 1024LL)
#define TERABYTES(value) (GIGABYTES(value) * 1024LL)
#define ARRAY_COUNT(array) (sizeof(array) / sizeof((array)[0]))

inline uint32
safeTruncateUInt32(uint64 value)
{
    ASSERT(value <= 0xffffffff);
    uint32 value32 = (uint32)value;
    return value32;
}

#if HANDMADE_INTERNAL
struct debugReadFileResult
{
    uint32 contentSize;
    void *contents;
};
internal debugReadFileResult DEBUGPlatformReadEntireFile(char *fileName);
internal void DEBUGPlatformFreeFileMemory(void *memory);
internal bool32 DEBUGPlatformWriteEntireFile(char *fileName, uint32 memorySize, void *memory);
#endif

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
    bool32 isConnected;
    bool32 isAnalog;
    real32 stickAverageX;
    real32 stickAverageY;

    union
    {
        gameButtonState Buttons[12];
        struct
        {
            gameButtonState moveUp;
            gameButtonState moveDown;
            gameButtonState moveLeft;
            gameButtonState moveRight;

            gameButtonState actionUp;
            gameButtonState actionDown;
            gameButtonState actionLeft;
            gameButtonState actionRight;

            gameButtonState leftShoulder;
            gameButtonState rightShoulder;

            gameButtonState start;
            gameButtonState back;

            // NOTE: all buttons must be added above this button
            gameButtonState terminator;
        };
    };
};

struct gameInput
{
    gameControllerInput controllers[5];
};

inline gameControllerInput *getController(gameInput *input, int unsigned controllerIndex)
{
    ASSERT(controllerIndex < ARRAY_COUNT(input->controllers));
    gameControllerInput *result = &input->controllers[controllerIndex];
    return result;
}

struct gameMemory
{
    bool32 isInitialized;
    uint64 permanentStorageSize;
    void *permanentStorage; // required to be zeroed at startup
    uint64 transientStorageSize;
    void *transientStorage; // required to be zeroed at startup
};

internal void gameUpdateAndRender(gameMemory *memory, gameInput *input, gameOffScreenBuffer *buffer, gameSoundOutputBuffer *soundBuffer);

struct gameState
{
    int toneHz;
    int xOffset;
    int yOffset;
};