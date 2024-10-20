#include <stdint.h>
#include <math.h>

#define Pi32 3.14159265359f

#define localPersists static;
#define globalVariable static;
#define internal static;

typedef int64_t int64;
typedef int32_t int32;
typedef int16_t int16;
typedef int8_t int8;
typedef int32_t bool32;
typedef uint64_t uint64;
typedef uint32_t uint32;
typedef uint16_t uint16;
typedef uint8_t uint8;
typedef int32_t bool32;
typedef float real32;
typedef double real64;

#include "handmade.cpp"

// ********************************************************

#include <windows.h>
#include <dsound.h>

struct DisplayBuffer
{
    uint16 width;
    uint16 height;
    BITMAPINFO info;
    void *memory;
    uint32 pitch;
};

struct Dimensions
{
    uint16 width;
    uint16 height;
};

globalVariable DisplayBuffer displayBuffer;
globalVariable bool globalRunning;

internal void win32_createDisplayBuffer(DisplayBuffer *buffer, int width, int height)
{
    if (buffer->memory)
    {
        VirtualFree(buffer->memory, 0, MEM_RELEASE);
    }

    buffer->width = width;
    buffer->height = height;
    buffer->info.bmiHeader.biSize = sizeof(buffer->info.bmiHeader);
    buffer->info.bmiHeader.biWidth = width;
    // Negative height for top down bitmap
    buffer->info.bmiHeader.biHeight = -height;
    buffer->info.bmiHeader.biPlanes = 1;
    buffer->info.bmiHeader.biBitCount = 32;
    buffer->info.bmiHeader.biCompression = BI_RGB;
    uint8 bytesPerPixels = 4;
    uint32 bitmapMemroySize = width * height * bytesPerPixels;
    buffer->memory = VirtualAlloc(0, bitmapMemroySize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    buffer->pitch = width * bytesPerPixels;
}

internal void win32_displayBufferOnWindow(HDC deviceContext, DisplayBuffer *buffer, int windowWidth, int windowHeight)
{
    StretchDIBits(deviceContext,
                  0, 0, windowWidth, windowHeight,
                  0, 0, buffer->width, buffer->height,
                  buffer->memory, &buffer->info, DIB_RGB_COLORS, SRCCOPY);
}

internal Dimensions win32_getWindowDimensions(HWND window)
{
    RECT clientrect;
    GetClientRect(window, &clientrect);
    Dimensions dimension;
    dimension.height = clientrect.bottom;
    dimension.width = clientrect.right;
    return dimension;
}

struct SoundOutput
{
    int samplesPerSecond;
    int toneHz;
    int16 toneVolume;
    uint32 runningSampleIndex;
    int wavePeriod;
    int bytesPerSample;
    int secondaryBufferSize;
    real32 tsine;
    int latencySampleCount;
};

internal LPDIRECTSOUNDBUFFER globalSecondaryBuffer;

#define CREATE_DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter);
typedef CREATE_DIRECT_SOUND_CREATE(direct_sound_create);

internal void win32_initDSound(HWND window, int32 samplesPerSecond, int32 bufferSize)
{
    HMODULE dSound = LoadLibraryA("dsound.dll");
    if (dSound)
    {
        direct_sound_create *DirectSoundCreate = (direct_sound_create *)GetProcAddress(dSound, "DirectSoundCreate");
        LPDIRECTSOUND directSound;
        if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &directSound, 0)))
        {
            WAVEFORMATEX waveFormat = {};
            waveFormat.wFormatTag = WAVE_FORMAT_PCM;
            waveFormat.nChannels = 2;
            waveFormat.nSamplesPerSec = samplesPerSecond;
            waveFormat.wBitsPerSample = 16;
            waveFormat.nBlockAlign = (waveFormat.nChannels * waveFormat.wBitsPerSample) / 8;
            waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
            if (SUCCEEDED(directSound->SetCooperativeLevel(window, DSSCL_PRIORITY)))
            {
                DSBUFFERDESC bufferDescription = {};
                bufferDescription.dwSize = sizeof(bufferDescription);
                bufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

                LPDIRECTSOUNDBUFFER primaryBuffer;
                if (SUCCEEDED(directSound->CreateSoundBuffer(&bufferDescription, &primaryBuffer, 0)))
                {
                    if (SUCCEEDED(primaryBuffer->SetFormat(&waveFormat))) // *** THIS HERE **** [no reason, not so important just defined this way]
                    {
                        OutputDebugStringA("Succeeded setting primary buffer format.\n");
                    }
                }
            }

            DSBUFFERDESC bufferDescription = {};
            bufferDescription.dwSize = sizeof(bufferDescription);
            bufferDescription.dwFlags = 0;
            bufferDescription.dwBufferBytes = bufferSize;
            bufferDescription.lpwfxFormat = &waveFormat; // *** WHY THIS HERE **** [no reason, not so important just defined this way]
            if (SUCCEEDED(directSound->CreateSoundBuffer(&bufferDescription, &globalSecondaryBuffer, 0)))
            {
                OutputDebugStringA("Succeeded creating  secondary buffer.\n");
            }
        }
    }
}

internal void win32_fillSoundBuffer(SoundOutput *soundOutput, DWORD byteToLock, DWORD bytesToWrite)
{
    void *region1;
    DWORD region1Size;
    void *region2;
    DWORD region2Size;
    if (SUCCEEDED(globalSecondaryBuffer->Lock(byteToLock, bytesToWrite, &region1, &region1Size, &region2, &region2Size, 0)))
    {
        DWORD region1SampleCount = region1Size / soundOutput->bytesPerSample;
        int16 *sampleOut = (int16 *)region1;
        for (DWORD sampleIndex = 0; sampleIndex < region1SampleCount; sampleIndex++)
        {
            real32 sinValue = sinf(soundOutput->tsine);
            int16 sampleValue = (int16)(sinValue * soundOutput->toneVolume);
            *sampleOut++ = sampleValue;
            *sampleOut++ = sampleValue;

            soundOutput->tsine += 2.0f * Pi32 / (real32)soundOutput->wavePeriod;
            ++soundOutput->runningSampleIndex;
        }

        DWORD region2SampleCount = region2Size / soundOutput->bytesPerSample;
        sampleOut = (int16 *)region2;
        for (DWORD sampleIndex = 0; sampleIndex < region2SampleCount; sampleIndex++)
        {
            real32 sinValue = sinf(soundOutput->tsine);
            int16 sampleValue = (int16)(sinValue * soundOutput->toneVolume);
            *sampleOut++ = sampleValue;
            *sampleOut++ = sampleValue;

            soundOutput->tsine += 2.0f * Pi32 / (real32)soundOutput->wavePeriod;
            ++soundOutput->runningSampleIndex;
        }

        globalSecondaryBuffer->Unlock(region1, region1Size, region2, region2Size);
    }
}

LRESULT CALLBACK
win32_MainWindowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CLOSE:
    {
        globalRunning = false;
    }
        return 0;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC deviceContext = BeginPaint(window, &ps);
        EndPaint(window, &ps);
    }
        return 0;
    case WM_KEYDOWN:
    case WM_KEYUP:
    {
        if ((lParam & (1 << 30)) == 0)
        {
            if (wParam == 'W')
            {
                OutputDebugStringA("W pressed.\n");
            }
        }
        else
        {
            if ((lParam & (1 << 31)) != 0)
            {
                if (wParam == 'W')
                {
                    OutputDebugStringA("W released.\n");
                }
            }
        }
    }
    }
    return DefWindowProcA(window, message, wParam, lParam);
}

int WINAPI
WinMain(HINSTANCE instance, HINSTANCE prevInstance, PSTR arguments, int showCode)
{
    WNDCLASSA windowClass = {};
    windowClass.hInstance = instance;
    windowClass.lpfnWndProc = win32_MainWindowCallback;
    windowClass.lpszClassName = "HandmadeHero class";
    windowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;

    RegisterClassA(&windowClass);

    HWND window = CreateWindowExA(
        0, windowClass.lpszClassName, "Game",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        0, 0, instance, 0);

    if (window)
    {
        globalRunning = true;

        HDC deviceContext = GetDC(window);
        win32_createDisplayBuffer(&displayBuffer, 1920, 1080);

        SoundOutput soundOutput = {};
        soundOutput.samplesPerSecond = 48000;                                                        // 48 khz = 48000 samples per second
        soundOutput.toneHz = 256;                                                                    // no of samples between (Two peeks) theoratical
        soundOutput.toneVolume = 3000;                                                               // max value [lower than (2 ^ 16 - 1) =  65535]
        soundOutput.runningSampleIndex = 0;                                                          // incremented every [left right] = 1 sample filled
        soundOutput.wavePeriod = soundOutput.samplesPerSecond / soundOutput.toneHz;                  // 48000 / 256 = 187 no of samples between (Two peeks) in actual buffer
        soundOutput.bytesPerSample = sizeof(int16) * 2;                                              // [left(2byte) right(2byte)] = 4bytes
        soundOutput.secondaryBufferSize = soundOutput.samplesPerSecond * soundOutput.bytesPerSample; // 48000 * 4bytes
        soundOutput.latencySampleCount = soundOutput.samplesPerSecond / 15;
        win32_initDSound(window, soundOutput.samplesPerSecond, soundOutput.secondaryBufferSize);
        win32_fillSoundBuffer(&soundOutput, 0, (soundOutput.latencySampleCount * soundOutput.bytesPerSample));
        globalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

        while (globalRunning)
        {
            MSG message;
            while (PeekMessageA(&message, 0, 0, 0, PM_REMOVE))
            {
                if (message.message == WM_QUIT)
                {
                    globalRunning = false;
                }
                TranslateMessage(&message);
                DispatchMessageA(&message);
            }

            game_DisplayBuffer buffer = {};
            buffer.memory = displayBuffer.memory;
            buffer.width = displayBuffer.width;
            buffer.height = displayBuffer.height;
            buffer.pitch = displayBuffer.pitch;
            gameUpdateAndRender(&buffer);

            Dimensions windowDimensions = win32_getWindowDimensions(window);
            win32_displayBufferOnWindow(deviceContext, &displayBuffer, windowDimensions.width, windowDimensions.height);

            // offset, in bytes, from the start of the buffer , the current play position
            DWORD playCursor;
            DWORD writeCursor;
            if (SUCCEEDED(globalSecondaryBuffer->GetCurrentPosition(&playCursor, &writeCursor)))
            {
                // Offset, in bytes, from the start of the buffer to where the lock begins.
                DWORD byteToLock = (soundOutput.runningSampleIndex * soundOutput.bytesPerSample) % soundOutput.secondaryBufferSize;
                DWORD targetCursor = (playCursor + (soundOutput.latencySampleCount * soundOutput.bytesPerSample)) % soundOutput.secondaryBufferSize;
                DWORD bytesToWrite; // number of bytes to be filled
                if (byteToLock > targetCursor)
                {
                    bytesToWrite = (soundOutput.secondaryBufferSize - byteToLock);
                    bytesToWrite += targetCursor;
                }
                else
                {
                    bytesToWrite = targetCursor - byteToLock;
                }
                win32_fillSoundBuffer(&soundOutput, byteToLock, bytesToWrite);
            }
        }
        ReleaseDC(window, deviceContext);
    }
}