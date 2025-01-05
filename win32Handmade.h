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
    DWORD playCursor;
    DWORD writeCursor;
};

struct win32_sound_output
{
    int samplesPerSeconds;
    uint32 runningSampleIndex;
    int bytesPerSample;
    DWORD secondaryBufferSize;
    real32 tSine;
    int latencySampleCount;
};