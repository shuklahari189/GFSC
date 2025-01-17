#pragma once

#include <stdint.h>
#include <math.h>

#define global_variable static
#define local_persist static
#define internal static

#define Pi32 3.14159265359f

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef int32 bool32;

typedef float real32;
typedef double real64;

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

inline uint32
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

#define DEBUG_PLATFORM_FREE_FILE_MEMORY(name) void name(void *memory)
typedef DEBUG_PLATFORM_FREE_FILE_MEMORY(debug_platform_free_file_memory);

#define DEBUG_PLATFORM_READ_ENTIRE_FILE(name) debug_read_file_result name(char *fileName)
typedef DEBUG_PLATFORM_READ_ENTIRE_FILE(debug_platfrom_read_entire_file);

#define DEBUG_PLATFORM_WRITE_ENTIRE_FILE(name) bool32 name(char *fileName, uint32 memorySize, void *memory)
typedef DEBUG_PLATFORM_WRITE_ENTIRE_FILE(debug_platform_write_entire_file);
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
    int bytesPerPixel;
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

    debug_platform_free_file_memory *DEBUGPlatformFreeFileMemory;
    debug_platfrom_read_entire_file *DEBUGPlatformReadEntireFile;
    debug_platform_write_entire_file *DEBUGPlatformWriteEntireFile;
};

#define GAME_UPDATE_AND_RENDER(name) void name(game_memory *memory, game_input *input, game_offscreen_buffer *buffer)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);
GAME_UPDATE_AND_RENDER(gameUpdateAndRenderStub)
{
}

#define GAME_GET_SOUND_SAMPLES(name) void name(game_memory *memory, game_sound_output_buffer *soundBuffer)
typedef GAME_GET_SOUND_SAMPLES(game_get_sound_samples);
GAME_GET_SOUND_SAMPLES(gameGetSoundSamplesStub)
{
}

struct game_state
{
    int toneHz;
    int xOffset;
    int yOffset;
    real32 tSine;

    int playerX;
    int playerY;
    real32 tJump;
};
