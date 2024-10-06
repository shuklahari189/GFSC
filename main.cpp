#include <windows.h>
#include <cstdint>
#include <dsound.h>

// signed types
typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

// unsigned types
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

#define LocalPersist static
#define GlobalVariable static
#define Internal static

struct win32OffscreenBuffer
{
    int width;
    int height;
    BITMAPINFO info;
    void *memory;
    int pitch;
};

struct win32WindowDimension
{
    int width;
    int height;
};

// TODO: global for now
GlobalVariable bool globalRunning;
GlobalVariable win32OffscreenBuffer globalBackBuffer;
GlobalVariable LPDIRECTSOUNDBUFFER globalSecondaryBuffer;

Internal win32WindowDimension
win32getWindowDimensions(HWND window)
{
    win32WindowDimension result;
    RECT clientRect;
    GetClientRect(window, &clientRect);
    result.width = clientRect.right - clientRect.left;
    result.height = clientRect.bottom - clientRect.top;
    return result;
}

Internal void
renderWeirdGradient(win32OffscreenBuffer *buffer, int xOffset, int yOffset)
{
    uint8 *row = (uint8 *)buffer->memory;
    for (int y = 0; y < buffer->height; y++)
    {
        uint32_t *pixels = (uint32_t *)row;
        for (int x = 0; x < buffer->width; x++)
        {
            uint8 r = (uint8)x + xOffset;
            uint8 g = (uint8)y + yOffset;
            uint8 b = 100;
            // Big endian, therefore
            *pixels++ = (r << 16) | (g << 8) | b;
        }
        row += buffer->pitch;
    }
}

Internal void
win32ResizeDIBsection(win32OffscreenBuffer *buffer, int width, int height)
{
    if (buffer->memory)
    {
        VirtualFree(buffer->memory, 0, MEM_RELEASE);
    }
    buffer->width = width;
    buffer->height = height;
    buffer->info.bmiHeader.biSize = sizeof(buffer->info);
    buffer->info.bmiHeader.biWidth = buffer->width;
    // -height for windows to treat bitmap as top down and not bottom up,
    // that is the first three byte stores pixel data for topLeftMost pixel
    buffer->info.bmiHeader.biHeight = -buffer->height;
    buffer->info.bmiHeader.biPlanes = 1;
    buffer->info.bmiHeader.biBitCount = 32; // acutally need 24 buit for dword alignment 32
    buffer->info.bmiHeader.biCompression = BI_RGB;
    int bytesPerPixels = 4;

    int bitmapMemorySize = buffer->width * buffer->height * bytesPerPixels;
    buffer->memory = VirtualAlloc(0, bitmapMemorySize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    buffer->pitch = buffer->width * bytesPerPixels;
}

Internal void
win32DisplayBufferInWindow(win32OffscreenBuffer *buffer, HDC deviceContext,
                           int windowWidth, int windowHeight)
{
    StretchDIBits(
        deviceContext,
        0, 0, windowWidth, windowHeight,
        0, 0, buffer->width, buffer->height,
        buffer->memory,
        &buffer->info,
        DIB_RGB_COLORS,
        SRCCOPY);
}

LRESULT
win32MainWindowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;
    switch (message)
    {
    case WM_SIZE:
    {
        OutputDebugStringA("sized\n");
    }
    break;
    case WM_ACTIVATEAPP:
    {
        OutputDebugStringA("active\n");
    }
    break;
    case WM_CLOSE:
    {
        // TODO: handle this with a message for user ?
        globalRunning = false;
        OutputDebugStringA("closed\n");
    }
    break;
    case WM_MOVING:
    {
        OutputDebugStringA("moving\n");
    }
    break;
    case WM_KEYDOWN:
    // commented below to not handle alt f4 for now
    // case WM_SYSKEYDOWN:
    // case WM_SYSKEYUP:
    case WM_KEYUP:
    {
        if ((lParam & (1 << 30)) == 0)
        {
            // if (wParam == 'W')
            // {
            //      w down
            // }
        }
        else
        {
            if ((lParam & (1 << 31)) != 0)
            {
                // if (wParam == 'W')
                // {
                //      w up
                // }
            }
        }
    }
    break;
    case WM_PAINT:
    {
        PAINTSTRUCT paint;
        HDC deviceContext = BeginPaint(window, &paint);
        int x = paint.rcPaint.left;
        int y = paint.rcPaint.top;
        int width = paint.rcPaint.right - paint.rcPaint.left;
        int height = paint.rcPaint.bottom - paint.rcPaint.top;
        win32WindowDimension dimensions = win32getWindowDimensions(window);
        win32DisplayBufferInWindow(&globalBackBuffer, deviceContext, dimensions.width, dimensions.height);
        EndPaint(window, &paint);
    }
    break;
    case WM_DESTROY:
    {

        // TODO: handle this with an error,then recreate window ?
        globalRunning = false;
        OutputDebugStringA("Window Destroyed!\n");
    }
    break;
    default:
    {
        result = DefWindowProcA(window, message, wParam, lParam);
    }
    break;
    }
    return result;
}

#define CREATE_DSOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef CREATE_DSOUND_CREATE(directSoundCreate);

Internal void win32InitDSound(HWND window, int32 bufferSize, int32 SamplePerSecond)
{
    HMODULE dSoundLibrary = LoadLibraryA("dsound.dll");
    if (dSoundLibrary)
    {
        directSoundCreate *DirectSoundCreate = (directSoundCreate *)GetProcAddress(dSoundLibrary, "DirectSoundCreate");

        LPDIRECTSOUND directSound;
        if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &directSound, 0)))
        {
            WAVEFORMATEX waveFormat;
            waveFormat.wFormatTag = WAVE_FORMAT_PCM;
            waveFormat.nChannels = 2;
            waveFormat.nSamplesPerSec = SamplePerSecond;
            waveFormat.wBitsPerSample = 16;
            waveFormat.nBlockAlign = (waveFormat.nChannels * waveFormat.wBitsPerSample) / 8;
            waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
            waveFormat.cbSize = 0;
            if (SUCCEEDED(directSound->SetCooperativeLevel(window, DSSCL_PRIORITY)))
            {
                DSBUFFERDESC bufferDescription = {};
                bufferDescription.dwSize = sizeof(bufferDescription);
                bufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

                LPDIRECTSOUNDBUFFER primaryBuffer;
                if (SUCCEEDED(directSound->CreateSoundBuffer(&bufferDescription, &primaryBuffer, 0)))
                {

                    if (SUCCEEDED(primaryBuffer->SetFormat(&waveFormat)))
                    {
                        OutputDebugStringA("Primary buffer format was set.\n");
                    }
                    else
                    {
                        // TODO: logging system
                    }
                }
                else
                {
                    // TODO: logging system
                }
            }
            else
            {
                // TODO: logging system
            }

            DSBUFFERDESC bufferDescription = {};
            bufferDescription.dwSize = sizeof(bufferDescription);
            bufferDescription.dwFlags = 0;
            bufferDescription.dwBufferBytes = bufferSize;
            bufferDescription.lpwfxFormat = &waveFormat;
            if (SUCCEEDED(directSound->CreateSoundBuffer(&bufferDescription, &globalSecondaryBuffer, 0)))
            {
                OutputDebugStringA("Secondary buffer created succesfully.\n");
            }
        }
        else
        {
            // TODO: logging system
        }
    }
}

int WINAPI
WinMain(HINSTANCE instance, HINSTANCE prevInstance, PSTR commandLine, int showCode)
{
    win32ResizeDIBsection(&globalBackBuffer, 1280, 720);
    WNDCLASSA windowClass = {};
    windowClass.style = CS_VREDRAW | CS_HREDRAW | CS_OWNDC;
    windowClass.lpfnWndProc = win32MainWindowCallback;
    windowClass.hInstance = instance;
    windowClass.lpszClassName = "HandmadeHeroClass";

    RegisterClassA(&windowClass);

    HWND window = CreateWindowExA(
        0,
        windowClass.lpszClassName,
        "GFSC",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT,
        CW_USEDEFAULT, CW_USEDEFAULT,
        NULL,
        NULL,
        instance,
        NULL);

    if (window)
    {
        int samplesPerSecond = 48000;
        int toneVolume = 1000;
        int toneHz = 256;
        uint32 runningSampleIndex = 0;
        int squareWavePeriod = samplesPerSecond / toneHz;
        int bytesPerSample = sizeof(int16) * 2;
        int halfSquareWavePeriod = squareWavePeriod / 2;
        int secondaryBufferSize = samplesPerSecond * bytesPerSample;
        win32InitDSound(window, samplesPerSecond, secondaryBufferSize);
        bool soundIsPlaying = false;

        HDC deviceContext = GetDC(window);

        int xOffset = 0;
        int yOffset = 0;

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

            DWORD playCursor;
            DWORD writeCursor;
            if (SUCCEEDED(globalSecondaryBuffer->GetCurrentPosition(&playCursor, &writeCursor)))
            {
                DWORD bytesToLock = runningSampleIndex * bytesPerSample % secondaryBufferSize;
                DWORD bytesToWrite;
                if (bytesToLock == playCursor)
                {
                    bytesToWrite = secondaryBufferSize;
                }
                else if (bytesToLock > playCursor)
                {
                    bytesToWrite = secondaryBufferSize - bytesToLock;
                    bytesToWrite += playCursor;
                }
                else
                {
                    bytesToWrite = playCursor - bytesToLock;
                }

                void *region1;
                DWORD region1Size;
                void *region2;
                DWORD region2Size;
                if (SUCCEEDED(globalSecondaryBuffer->Lock(bytesToLock, bytesToWrite,
                                                          &region1, &region1Size,
                                                          &region2, &region2Size,
                                                          0)))
                {
                    DWORD Region1SampleCount = region1Size / bytesPerSample;
                    int16 *sampleOut = (int16 *)region1;
                    for (int sampleIndex = 0; sampleIndex < Region1SampleCount; sampleIndex++)
                    {
                        int16 sampleValue = ((runningSampleIndex++ / halfSquareWavePeriod) % 2) ? toneVolume : -toneVolume;
                        *sampleOut++ = sampleValue;
                        *sampleOut++ = sampleValue;
                    }

                    DWORD Region2SampleCount = region2Size / bytesPerSample;
                    sampleOut = (int16 *)region2;
                    for (int sampleIndex = 0; sampleIndex < Region2SampleCount; sampleIndex++)
                    {
                        int16 sampleValue = ((runningSampleIndex++ / halfSquareWavePeriod) % 2) ? toneVolume : -toneVolume;
                        *sampleOut++ = sampleValue;
                        *sampleOut++ = sampleValue;
                    }
                    globalSecondaryBuffer->Unlock(region1, region1Size, region2, region2Size);
                }
            }

            if (!soundIsPlaying)
            {
                globalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);
                soundIsPlaying = true;
            }

            win32WindowDimension dimensions = win32getWindowDimensions(window);
            win32DisplayBufferInWindow(&globalBackBuffer, deviceContext, dimensions.width, dimensions.height);
        }
    }
    else
    {
        // TODO: logging system ?
    }

    return 0;
}