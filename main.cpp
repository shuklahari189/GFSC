#include <windows.h>
#include <dsound.h>
#include <math.h>
#include <stdint.h>

typedef int32_t bool32;
typedef int64_t int64;
typedef int32_t int32;
typedef int16_t int16;
typedef int8_t int8;
typedef uint64_t uint64;
typedef uint32_t uint32;
typedef uint16_t uint16;
typedef uint8_t uint8;
typedef float real32;
typedef double real64;

#define pi32 3.14159265359f

#define localPersist static
#define internal static
#define globalVariable static

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter);
typedef DIRECT_SOUND_CREATE(direct_sound_create);

struct win32offscreenbuffer
{
    BITMAPINFO info;
    void *memory;
    int height;
    int width;
    int pitch;
};

struct win32windowDimension
{
    int width;
    int height;
};

struct win32soundOutput
{
    int samplesPerSecond;
    int toneHz;
    int16 toneVolume;
    uint32 runningSampleIndex;
    int wavePeriod;
    int bytesPerSample;
    int secondaryBufferSize;
    real32 tsin;
    int latencySampleCount;
};

globalVariable bool32 globalRunning;
globalVariable win32offscreenbuffer globalBackBuffer;
globalVariable LPDIRECTSOUNDBUFFER globalSecondaryBuffer;
globalVariable bool32 isWDown;
globalVariable bool32 isSDown;

internal void
win32fillSoundBuffer(win32soundOutput *soundOutput, DWORD byteToLock, DWORD bytesToWrite)
{
    void *region1;
    DWORD region1Size;
    void *region2;
    DWORD region2Size;
    if (SUCCEEDED(globalSecondaryBuffer->Lock(byteToLock, bytesToWrite,
                                              &region1, &region1Size,
                                              &region2, &region2Size, 0)))
    {
        DWORD region1sampleCount = region1Size / soundOutput->bytesPerSample;
        int16 *sampleOut = (int16 *)region1;
        for (DWORD sampleIndex = 0; sampleIndex < region1sampleCount; sampleIndex++)
        {
            real32 sinValue = sinf(soundOutput->tsin);
            int16 sampleValue = (int16)(sinValue * soundOutput->toneVolume);
            *sampleOut++ = sampleValue;
            *sampleOut++ = sampleValue;
            soundOutput->tsin += 2.0f * pi32 * 1.0f / (real32)soundOutput->wavePeriod;
            soundOutput->runningSampleIndex++;
        }

        DWORD region2sampleCount = region2Size / soundOutput->bytesPerSample;
        sampleOut = (int16 *)region2;
        for (DWORD sampleIndex = 0; sampleIndex < region2sampleCount; sampleIndex++)
        {
            real32 sinValue = sinf(soundOutput->tsin);
            int16 sampleValue = (int16)(sinValue * soundOutput->toneVolume);
            *sampleOut++ = sampleValue;
            *sampleOut++ = sampleValue;
            soundOutput->tsin += 2.0f * pi32 * 1.0f / (real32)soundOutput->wavePeriod;
            soundOutput->runningSampleIndex++;
        }
        globalSecondaryBuffer->Unlock(region1, region1Size, region2, region2Size);
    }
}

internal void
win32initDSound(HWND window, int32 samplesPerSecond, int32 bufferSize)
{
    HMODULE dSoundLibrary = LoadLibraryA("dsound.dll");

    if (dSoundLibrary)
    {
        direct_sound_create *DirectSoundCreate = (direct_sound_create *)GetProcAddress(dSoundLibrary, "DirectSoundCreate");

        LPDIRECTSOUND directSound;
        if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &directSound, 0)))
        {
            WAVEFORMATEX waveFormat = {};
            waveFormat.wFormatTag = WAVE_FORMAT_PCM;
            waveFormat.nChannels = 2;
            waveFormat.wBitsPerSample = 16;
            waveFormat.nSamplesPerSec = samplesPerSecond;
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
                    primaryBuffer->SetFormat(&waveFormat);
                    OutputDebugStringA("Succeeded setting primary buffer's format.\n");
                }
            }

            DSBUFFERDESC bufferDescription = {};
            bufferDescription.dwSize = sizeof(bufferDescription);
            bufferDescription.dwBufferBytes = bufferSize;
            bufferDescription.lpwfxFormat = &waveFormat;
            if (SUCCEEDED(directSound->CreateSoundBuffer(&bufferDescription, &globalSecondaryBuffer, 0)))
            {
                OutputDebugStringA("Succeeded creating secondary buffer.\n");
            }
        }
    }
}

internal win32windowDimension
win32getWindowDimensions(HWND window)
{
    win32windowDimension windowDimesnions;
    RECT clientRect;
    GetClientRect(window, &clientRect);
    windowDimesnions.width = clientRect.right;
    windowDimesnions.height = clientRect.bottom;
    return windowDimesnions;
}

internal void
renderWeirdGradient(win32offscreenbuffer *buffer, int xOffset, int yOffset)
{

    uint8 *row = (uint8 *)buffer->memory;
    for (int y = 0; y < buffer->height; y++)
    {
        uint32 *pixel = (uint32 *)row;
        for (int x = 0; x < buffer->width; x++)
        {
            uint8 r = 0;
            uint8 g = (uint8)x + xOffset;
            uint8 b = (uint8)y + yOffset;
            *pixel++ = (r << 16) | (g << 8) | b;
        }
        row += buffer->pitch;
    }
}

internal void
win32resizeDIBsection(win32offscreenbuffer *buffer, int width, int height)
{
    if (buffer->memory)
    {
        VirtualFree(buffer->memory, 0, MEM_RELEASE);
    }

    buffer->width = width;
    buffer->height = height;
    int bytesPerPixels = 4;

    buffer->info.bmiHeader.biSize = sizeof(buffer->info.bmiHeader);
    buffer->info.bmiHeader.biWidth = width;
    buffer->info.bmiHeader.biHeight = -height;
    buffer->info.bmiHeader.biPlanes = 1;
    buffer->info.bmiHeader.biBitCount = 32;
    buffer->info.bmiHeader.biCompression = BI_RGB;

    int bitmapMemorySize = width * height * bytesPerPixels;
    buffer->memory = VirtualAlloc(0, bitmapMemorySize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    buffer->pitch = width * bytesPerPixels;
}

internal void
win32displayBufferInWindow(win32offscreenbuffer *buffer, HDC deviceContext, int windowWidth, int windowHeight)
{
    // TODO: Aspect ratio correction
    StretchDIBits(deviceContext,
                  0, 0, windowWidth, windowHeight,
                  0, 0, buffer->width, buffer->height,
                  buffer->memory,
                  &buffer->info, DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK
win32mainWindowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CLOSE:
    {
        globalRunning = false;
        OutputDebugStringA("WM_CLOSE.\n");
    }
        return 0;
    case WM_DESTROY:
    {
        globalRunning = false;
        OutputDebugStringA("WM_DESTROY.\n");
    }
        return 0;
    case WM_KEYDOWN:
    case WM_KEYUP:
    {
        if ((lParam & (1 << 30)) == 0)
        {
            if (wParam == 'W')
            {
                isWDown = true;
            }
            if (wParam == 'S')
            {
                isSDown = true;
            }
        }
        else
        {
            if ((lParam & (1 << 31)) != 0)
            {
                if (wParam == 'W')
                {
                    isWDown = false;
                }
                if (wParam == 'S')
                {
                    isSDown = false;
                }
            }
        }
    }
        return 0;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC deviceContext = BeginPaint(window, &ps);

        renderWeirdGradient(&globalBackBuffer, 0, 0);
        win32windowDimension windowDimension = win32getWindowDimensions(window);
        win32displayBufferInWindow(&globalBackBuffer, deviceContext, windowDimension.width, windowDimension.height);

        EndPaint(window, &ps);
    }
        return 0;
    }

    return DefWindowProcA(window, message, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE prevInstance, PSTR commandLine, int showCode)
{
    win32resizeDIBsection(&globalBackBuffer, 1200, 720);

    WNDCLASSA windowClass = {};
    windowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    windowClass.lpszClassName = "handmadeHeroWindowClass";
    windowClass.hInstance = instance;
    windowClass.lpfnWndProc = win32mainWindowCallback;

    RegisterClassA(&windowClass);

    HWND window = CreateWindowExA(
        0, windowClass.lpszClassName, "game", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        0, 0,
        instance, 0);

    if (window)
    {
        HDC deviceContext = GetDC(window);

        int xOffset = 0;
        int yOffset = 0;

        win32soundOutput soundOutput = {};
        soundOutput.samplesPerSecond = 48000;
        soundOutput.toneHz = 256;
        soundOutput.toneVolume = 3000;
        soundOutput.runningSampleIndex = 0;
        soundOutput.wavePeriod = soundOutput.samplesPerSecond / soundOutput.toneHz;
        soundOutput.bytesPerSample = sizeof(int16) * 2;
        soundOutput.secondaryBufferSize = soundOutput.samplesPerSecond * soundOutput.bytesPerSample;
        soundOutput.latencySampleCount = soundOutput.samplesPerSecond / 15;

        win32initDSound(window, soundOutput.samplesPerSecond, soundOutput.secondaryBufferSize);
        win32fillSoundBuffer(&soundOutput, 0, soundOutput.latencySampleCount * soundOutput.bytesPerSample);

        globalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

        globalRunning = true;
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

            renderWeirdGradient(&globalBackBuffer, xOffset, yOffset);

            if (isWDown)
            {
                soundOutput.toneHz += 1;
                yOffset++;
                soundOutput.wavePeriod = soundOutput.samplesPerSecond / soundOutput.toneHz;
            }
            if (isSDown)
            {
                soundOutput.toneHz -= 1;
                yOffset--;
                soundOutput.wavePeriod = soundOutput.samplesPerSecond / soundOutput.toneHz;
            }

            DWORD playCursor;
            DWORD writeCursor;
            if (SUCCEEDED(globalSecondaryBuffer->GetCurrentPosition(&playCursor, &writeCursor)))
            {
                DWORD byteToLock = (soundOutput.runningSampleIndex * soundOutput.bytesPerSample) % soundOutput.secondaryBufferSize;
                DWORD targetCursor = (playCursor + (soundOutput.latencySampleCount * soundOutput.bytesPerSample)) % soundOutput.secondaryBufferSize;
                DWORD bytesToWrite;
                if (byteToLock > targetCursor)
                {
                    bytesToWrite = soundOutput.secondaryBufferSize - byteToLock;
                    bytesToWrite += targetCursor;
                }
                else
                {
                    bytesToWrite = targetCursor - byteToLock;
                }

                win32fillSoundBuffer(&soundOutput, byteToLock, bytesToWrite);
            }

            win32windowDimension windowDimension = win32getWindowDimensions(window);
            win32displayBufferInWindow(&globalBackBuffer, deviceContext, windowDimension.width, windowDimension.height);
        }
        ReleaseDC(window, deviceContext);
    }

    return 0;
}