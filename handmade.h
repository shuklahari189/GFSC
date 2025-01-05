#pragma once

#if HANDMADE_SLOW
#define ASSERT(expression) \
    if (!(expression))     \
    {                      \
        *((int *)0) = 0;   \
    }
#else
#define ASSERT(expression)
#endif

#define KILOBYTES(value) ((value) * 1024LL)
#define MEGABYTES(value) (KILOBYTES(value) * 1024LL)
#define GIGABYTES(value) (MEGABYTES(value) * 1024LL)
#define TERABYTES(value) (GIGABYTES(value) * 1024LL)

#define ARRAY_COUNT(array) (sizeof(array) / sizeof((array)[0]))

internal inline uint32
safeTruncateUInt64(uint64 value)
{
    ASSERT(value < 0xffffffff);
    uint32 result = (uint32)value;
    return result;
}

#if HANDMADE_INTERNAL
struct debug_read_file_result
{
    uint32 contentSize;
    void *contents;
};
internal debug_read_file_result DEBUGPlatformReadEntireFile(char *fileName);
internal void DEBUGPlatformFreeFileMemory(void *memory);
internal bool32 DEBUGPlatformWriteEntireFile(char *fileName, uint32 memorySize, void *memory);
#endif

struct game_sound_output_buffer
{
    int16 *samples;
    int sampleCount;
    int samplesPerSecond;
};

struct game_offscreen_buffer
{
    void *memory;
    int width;
    int height;
    int pitch;
};

struct game_button_state
{
    int halfTransitionState;
    bool32 endedDown;
};

struct game_controller_input
{
    bool32 isConnected;
    bool32 isAnalog;
    real32 stickAverageX;
    real32 stickAverageY;

    union
    {
        game_button_state buttons[12];
        struct
        {
            game_button_state moveUp;
            game_button_state moveDown;
            game_button_state moveLeft;
            game_button_state moveRight;

            game_button_state actionUp;
            game_button_state actionDown;
            game_button_state actionLeft;
            game_button_state actionRight;

            game_button_state leftShoulder;
            game_button_state rightShoulder;

            game_button_state back;
            game_button_state start;

            game_button_state terminator;
        };
    };
};

struct game_input
{
    game_controller_input controllers[5];
};

inline game_controller_input *getController(game_input *input, int unsigned controllerIndex)
{
    ASSERT(controllerIndex < ARRAY_COUNT(input->controllers));
    game_controller_input *result = &input->controllers[controllerIndex];
    return result;
}

struct game_memory
{
    bool32 isInitialized;
    uint64 transientStorageSize;
    void *transientStorage; // Required to be zeroed at startup
    uint64 permanentStorageSize;
    void *permanentStorage; // Required to be zeroed at startup
};

internal void
gameUpdateAndRender(game_memory *memory, game_input *input, game_offscreen_buffer *buffer, game_sound_output_buffer *soundBuffer);

struct game_state
{
    int toneHz;
    int xOffset;
    int yOffset;
};