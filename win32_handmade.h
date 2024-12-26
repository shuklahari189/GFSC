#pragma once

struct win32OffScreenBuffer
{
    BITMAPINFO info;
    void *memory;
    int width;
    int height;
    int pitch;
};

struct win32WindowDimensions
{
    int width;
    int height;
};

struct win32SoundOutput
{
    int samplesPerSecond;
    uint32 runningSampleIndex;
    int bytesPerSample;
    int secondaryBufferSize;
    real32 tSine;
    int latencySampleCount;
};