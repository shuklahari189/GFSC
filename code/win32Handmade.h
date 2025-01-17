#pragma once

struct win32_offscreen_buffer
{
    BITMAPINFO info;
    void *memory;
    int width;
    int height;
    int pitch;
    int bytesPerPixel;
};

struct win32_window_dimensions
{
    int width;
    int height;
};

struct win32_debug_time_marker
{
    DWORD outputPlayCursor;
    DWORD outputWriteCursor;
    DWORD outputLocation;
    DWORD outputByteCount;
    DWORD expectedFlipPlayCursor;

    DWORD flipPlayCursor;
    DWORD flipWriteCursor;
};

struct win32_sound_output
{
    int samplesPerSeconds;
    uint32 runningSampleIndex;
    int bytesPerSample;
    DWORD secondaryBufferSize;
    DWORD safetyBytes;
    real32 tSine;
    int latencySampleCount;
};

struct win32_game_code
{
    HMODULE gameCodeDll;
    FILETIME DLLLastWriteTime;

    game_update_and_render *UpdateAndRender;
    game_get_sound_samples *GetSoundSamples;

    bool32 isValid;
};

#define WIN32_STATE_FILE_NAME_COUNT MAX_PATH
struct win32_state
{
    uint64 totalSize;
    void *gameMemoryBlock;

    HANDLE recordingHandle;
    int inputRecordingIndex;
    HANDLE playBackHandle;
    int inputPlayingIndex;

    char exeFileName[WIN32_STATE_FILE_NAME_COUNT];
    char *onePastLastExeFileNameSlash;
};
