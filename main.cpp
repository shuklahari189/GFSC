#include <windows.h>
#include <stdint.h>
#include <xinput.h>
#include <dsound.h>
#include <stdio.h>
#include <math.h>

#define Pi32 3.14159265359

#define internal static
#define local_persit static
#define global_variable static

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
    int toneHz;
    int toneVolume;
    uint32 runningSampleIndex;
    int wavePeriod;
    int bytesPerSample;
    int secondaryBufferSize;
    real32 tSine;
    int latencySampleCount;
};

global_variable int globalRunning;
global_variable win32OffScreenBuffer globalBackBuffer;
global_variable LPDIRECTSOUNDBUFFER globalSecondaryBuffer;
global_variable bool32 globalwDown;
global_variable bool32 globalsDown;

#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
    return ERROR_DEVICE_NOT_CONNECTED;
}
global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
    return ERROR_DEVICE_NOT_CONNECTED;
}
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPGUID lpGuid, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);

internal void
win32fillSoundBuffer(win32SoundOutput *soundOutput, DWORD byteToLock, DWORD bytesToWrite)
{
    void *region1;
    DWORD region1Size;
    void *region2;
    DWORD region2Size;

    if (SUCCEEDED(globalSecondaryBuffer->Lock(byteToLock, bytesToWrite, &region1, &region1Size, &region2, &region2Size, 0)))
    {
        int16 *sampleOut = (int16 *)region1;
        DWORD region1SampleCount = region1Size / soundOutput->bytesPerSample;
        for (int sampleIndex = 0; sampleIndex < region1SampleCount; sampleIndex++)
        {
            real32 sinValue = sinf(soundOutput->tSine);
            int16 sampleValue = (int16)(sinValue * soundOutput->toneVolume);
            *sampleOut++ = sampleValue;
            *sampleOut++ = sampleValue;

            soundOutput->tSine += ((real32)1.0f / (real32)soundOutput->wavePeriod) * 2.0f * Pi32;
            soundOutput->runningSampleIndex++;
        }

        sampleOut = (int16 *)region2;
        DWORD region2SampleCount = region2Size / soundOutput->bytesPerSample;
        for (int sampleIndex = 0; sampleIndex < region2SampleCount; sampleIndex++)
        {
            real32 sinValue = sinf(soundOutput->tSine);
            int16 sampleValue = (int16)(sinValue * soundOutput->toneVolume);
            *sampleOut++ = sampleValue;
            *sampleOut++ = sampleValue;

            soundOutput->tSine += ((real32)1.0f / (real32)soundOutput->wavePeriod) * 2.0f * Pi32;
            soundOutput->runningSampleIndex++;
        }
        globalSecondaryBuffer->Unlock(region1, region1Size, region2, region2Size);
    }
}

internal void
win32loadXInput()
{
    HMODULE xInputLibrary = LoadLibraryA("xinput1_4.dll");
    if (!xInputLibrary)
    {
        xInputLibrary = LoadLibraryA("xinput1_3.dll");
    }
    if (!xInputLibrary)
    {
        xInputLibrary = LoadLibraryA("XInput9_1_0.dll");
    }

    if (xInputLibrary)
    {
        XInputGetState = (x_input_get_state *)GetProcAddress(xInputLibrary, "XInputGetState");
        if (!XInputGetState)
        {
            XInputGetState = XInputGetStateStub;
        }
        XInputSetState = (x_input_set_state *)GetProcAddress(xInputLibrary, "XInputSetState");
        if (!XInputSetState)
        {
            XInputSetState = XInputSetStateStub;
        }
    }
    else
    {
        // TODO: error printing
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
                    if (SUCCEEDED(primaryBuffer->SetFormat(&waveFormat)))
                    {
                        OutputDebugStringA("succeded setting primary buffer format.\n");
                    }
                    else
                    {
                        // TODO: error logging
                    }
                }
            }
            else
            {
                // TODO: error logging
            }

            DSBUFFERDESC bufferDescription = {};
            bufferDescription.dwSize = sizeof(bufferDescription);
            bufferDescription.dwBufferBytes = bufferSize;
            bufferDescription.lpwfxFormat = &waveFormat;

            if (SUCCEEDED(directSound->CreateSoundBuffer(&bufferDescription, &globalSecondaryBuffer, 0)))
            {
                OutputDebugStringA("succeded creating secondary buffer.\n");
            }
        }
    }
}

internal win32WindowDimensions
win32GetWindowDimension(HWND window)
{
    win32WindowDimensions result;
    RECT clientRect;
    GetClientRect(window, &clientRect);
    result.width = clientRect.right - clientRect.left;
    result.height = clientRect.bottom - clientRect.top;
    return result;
}

internal void
renderWeirdGradient(win32OffScreenBuffer *buffer, int xOffset, int yOffset)
{
    uint8 *row = (uint8 *)buffer->memory;
    for (int y = 0;
         y < buffer->height;
         y++)
    {
        uint32 *pixel = (uint32 *)row;
        for (int x = 0;
             x < buffer->width;
             x++)
        {
            //            red              green                      blue
            *pixel++ = (0) << 16 | (uint8(x + xOffset) << 8) | uint8(y + yOffset);
        }
        row += buffer->pitch;
    }
}

internal void
win32ResizeDIBSection(win32OffScreenBuffer *buffer, int width, int height)
{
    if (buffer->memory)
    {
        VirtualFree(buffer->memory, 0, MEM_RELEASE);
    }
    buffer->width = width;
    buffer->height = height;
    int bytesPerPixel = 4;

    buffer->info.bmiHeader.biSize = sizeof(buffer->info.bmiHeader);
    buffer->info.bmiHeader.biWidth = buffer->width;
    // top down bitmap
    buffer->info.bmiHeader.biHeight = -buffer->height;
    buffer->info.bmiHeader.biPlanes = 1;
    buffer->info.bmiHeader.biBitCount = 32;
    buffer->info.bmiHeader.biCompression = BI_RGB;

    int bitmapMemorySize = (buffer->width * buffer->height) * bytesPerPixel;
    buffer->memory = VirtualAlloc(0, bitmapMemorySize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    buffer->pitch = width * bytesPerPixel;
}

internal void
win32DisplayBufferInWindow(win32OffScreenBuffer *buffer, HDC deviceContext,
                           int windowWidth, int windowHeight)
{
    StretchDIBits(deviceContext,
                  0, 0, windowWidth, windowHeight,
                  0, 0, buffer->width, buffer->height,
                  buffer->memory, &buffer->info,
                  DIB_RGB_COLORS, SRCCOPY);
}

internal LRESULT CALLBACK
win32MainWindowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CLOSE:
    {
        globalRunning = false;
    }
        return 0;
    case WM_ACTIVATEAPP:
    {
    }
        return 0;
    case WM_DESTROY:
    {
        globalRunning = false;
    }
        return 0;
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_KEYDOWN:
    case WM_KEYUP:
    {
        uint32 vkCode = wParam;
        bool32 wasDown = ((lParam & (1 << 30)) != 0);
        bool32 isDown = ((lParam & (1 << 31)) == 0);
        if (wasDown != isDown)
        {
            if (vkCode == 'W')
            {
                if (isDown)
                {
                    globalwDown = true;
                }
                if (wasDown)
                {
                    globalwDown = false;
                }
            }
            if (vkCode == 'S')
            {
                if (isDown)
                {
                    globalsDown = true;
                }
                if (wasDown)
                {
                    globalsDown = false;
                }
            }
            if (vkCode == 'A')
            {
            }
            if (vkCode == 'D')
            {
            }
            if (vkCode == 'E')
            {
            }
            if (vkCode == 'Q')
            {
            }
            if (vkCode == VK_UP)
            {
            }
            if (vkCode == VK_DOWN)
            {
            }
            if (vkCode == VK_RIGHT)
            {
            }
            if (vkCode == VK_LEFT)
            {
            }
            if (vkCode == VK_SPACE)
            {
            }
            if (vkCode == VK_ESCAPE)
            {
                // OutputDebugStringA("escape: ");
                // if (isDown)
                // {
                //     OutputDebugStringA("isDown ");
                // }
                // if (wasDown)
                // {
                //     OutputDebugStringA("wasDown ");
                // }
                // OutputDebugStringA("\n");
            }
        }
        bool32 altKeyWasDown = (lParam & (1 << 29));
        if ((vkCode == VK_F4) && altKeyWasDown)
        {
            globalRunning = false;
        }
    }
        return 0;
    case WM_PAINT:
    {
        PAINTSTRUCT paint;
        HDC deviceContext = BeginPaint(window, &paint);
        win32WindowDimensions dimension = win32GetWindowDimension(window);
        win32DisplayBufferInWindow(&globalBackBuffer, deviceContext, dimension.width, dimension.height);
        EndPaint(window, &paint);
    }
        return 0;
    }
    return DefWindowProcA(window, message, wParam, lParam);
}

int CALLBACK WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR commandLine, int showCode)
{
    LARGE_INTEGER perfCountFrequencyResult;
    QueryPerformanceFrequency(&perfCountFrequencyResult);
    int64 perfCountFrequency = perfCountFrequencyResult.QuadPart;

    win32loadXInput();
    win32ResizeDIBSection(&globalBackBuffer, 1280, 720);

    WNDCLASSA windowClass = {};
    windowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    windowClass.lpfnWndProc = win32MainWindowCallback;
    windowClass.hInstance = instance;
    windowClass.lpszClassName = "HANDMADE HERO WINDOW CLASS";

    if (RegisterClassA(&windowClass))
    {
        HWND window = CreateWindowExA(0,
                                      windowClass.lpszClassName,
                                      "HandMade Hero",
                                      WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                      CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                      0, 0, instance, 0);
        if (window)
        {
            HDC deviceContext = GetDC(window);

            int xOffset = 0;
            int yOffset = 0;

            win32SoundOutput soundOutput = {};
            soundOutput.samplesPerSecond = 48000;
            soundOutput.toneHz = 256;
            soundOutput.toneVolume = 5000;
            soundOutput.runningSampleIndex = 0;
            soundOutput.wavePeriod = soundOutput.samplesPerSecond / soundOutput.toneHz;
            soundOutput.bytesPerSample = sizeof(int16) * 2;
            soundOutput.secondaryBufferSize = soundOutput.samplesPerSecond * soundOutput.bytesPerSample;
            soundOutput.latencySampleCount = soundOutput.samplesPerSecond / 15;
            win32initDSound(window, soundOutput.samplesPerSecond, soundOutput.secondaryBufferSize);
            win32fillSoundBuffer(&soundOutput, 0, soundOutput.latencySampleCount * soundOutput.bytesPerSample);
            globalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

            globalRunning = true;
            LARGE_INTEGER lastCounter;
            QueryPerformanceCounter(&lastCounter);
            uint64 lastCycleCount = __rdtsc();
            while (globalRunning)
            {
                if (globalwDown)
                {
                    ++yOffset;
                    soundOutput.toneHz += 1;
                    soundOutput.wavePeriod = soundOutput.samplesPerSecond / soundOutput.toneHz;
                }
                else if (globalsDown)
                {
                    --yOffset;
                    soundOutput.toneHz -= 1;
                    soundOutput.wavePeriod = soundOutput.samplesPerSecond / soundOutput.toneHz;
                }
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

                for (DWORD controllerIndex = 0; controllerIndex < XUSER_MAX_COUNT; controllerIndex++)
                {
                    XINPUT_STATE controllerState;
                    if (XInputGetState(controllerIndex, &controllerState) == ERROR_SUCCESS)
                    {
                        // controllerIndex'th controller is plugged in
                        XINPUT_GAMEPAD *pad = &controllerState.Gamepad;
                        bool32 up = pad->wButtons & XINPUT_GAMEPAD_DPAD_UP;
                        bool32 down = pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN;
                        bool32 right = pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT;
                        bool32 left = pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT;
                        bool32 start = pad->wButtons & XINPUT_GAMEPAD_START;
                        bool32 back = pad->wButtons & XINPUT_GAMEPAD_BACK;
                        bool32 leftShoulder = pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER;
                        bool32 rightShoulder = pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER;
                        bool32 aButton = pad->wButtons & XINPUT_GAMEPAD_A;
                        bool32 bButton = pad->wButtons & XINPUT_GAMEPAD_B;
                        bool32 xButton = pad->wButtons & XINPUT_GAMEPAD_X;
                        bool32 yButton = pad->wButtons & XINPUT_GAMEPAD_Y;

                        int16 stickX = pad->sThumbLX;
                        int16 stickY = pad->sThumbLY;

                        // #define XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE 7849
                        // #define XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE 8689
                        xOffset += stickX / 4096;
                        yOffset += stickY / 4096;
                    }
                    else
                    {
                        // controllerIndex'th controller is not plugged in
                    }
                }

                renderWeirdGradient(&globalBackBuffer, xOffset, yOffset);

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

                win32WindowDimensions dimension = win32GetWindowDimension(window);
                win32DisplayBufferInWindow(&globalBackBuffer, deviceContext, dimension.width, dimension.height);

                uint64 endCycleCount = __rdtsc();
                LARGE_INTEGER endCounter;
                QueryPerformanceCounter(&endCounter);
                uint64 cyclesElapsed = endCycleCount - lastCycleCount;
                int64 counterElapsed = endCounter.QuadPart - lastCounter.QuadPart;
                real64 msPerFrame = (((real64)counterElapsed * 1000.0f) / (real64)perfCountFrequency);
                real64 FPS = (real64)perfCountFrequency / (real64)counterElapsed;
                real64 MCPF = (real64)cyclesElapsed / (1000.0f * 1000.0f);

                char buffer[256];
                sprintf(buffer, "%.02fms/f, %.02ff/s, %.02fmc/f\n", msPerFrame, FPS, MCPF);
                OutputDebugStringA(buffer);

                lastCounter = endCounter;
                lastCycleCount = endCycleCount;
            }
        }
    }

    return 0;
}