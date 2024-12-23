#include <windows.h>
#include <stdint.h>
#include <xinput.h>
#include <dsound.h>

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

global_variable int globalRunning;
global_variable win32OffScreenBuffer globalBackBuffer;

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
win32loadXInput()
{
    HMODULE xInputLibrary = LoadLibraryA("xinput1_4.dll");
    if (!xInputLibrary)
    {
        xInputLibrary = LoadLibraryA("xinput1_3.dll");
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

            LPDIRECTSOUNDBUFFER secondaryBuffer;
            if (SUCCEEDED(directSound->CreateSoundBuffer(&bufferDescription, &secondaryBuffer, 0)))
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
        bool wasDown = ((lParam & (1 << 30)) != 0);
        bool isDown = ((lParam & (1 << 31)) == 0);
        if (wasDown != isDown)
        {
            if (vkCode == 'W')
            {
            }
            if (vkCode == 'S')
            {
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
                OutputDebugStringA("escape: ");
                if (isDown)
                {
                    OutputDebugStringA("isDown ");
                }
                if (wasDown)
                {
                    OutputDebugStringA("wasDown ");
                }
                OutputDebugStringA("\n");
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
            win32initDSound(window, 48000, 48000 * sizeof(int16) * 2);
            int xOffset = 0;
            int yOffset = 0;
            globalRunning = true;
            while (globalRunning)
            {
                HDC deviceContext = GetDC(window);
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
                        int up = pad->wButtons & XINPUT_GAMEPAD_DPAD_UP;
                        int down = pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN;
                        int right = pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT;
                        int left = pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT;
                        int start = pad->wButtons & XINPUT_GAMEPAD_START;
                        int back = pad->wButtons & XINPUT_GAMEPAD_BACK;
                        int leftShoulder = pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER;
                        int rightShoulder = pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER;
                        int aButton = pad->wButtons & XINPUT_GAMEPAD_A;
                        int bButton = pad->wButtons & XINPUT_GAMEPAD_B;
                        int xButton = pad->wButtons & XINPUT_GAMEPAD_X;
                        int yButton = pad->wButtons & XINPUT_GAMEPAD_Y;

                        int16 stickX = pad->sThumbLX;
                        int16 stickY = pad->sThumbLY;

                        xOffset += stickX >> 12;
                        yOffset += stickY >> 12;
                    }
                    else
                    {
                        // controllerIndex'th controller is not plugged in
                    }
                }

                renderWeirdGradient(&globalBackBuffer, xOffset, yOffset);

                win32WindowDimensions dimension = win32GetWindowDimension(window);
                win32DisplayBufferInWindow(&globalBackBuffer, deviceContext, dimension.width, dimension.height);
                ++xOffset;
            }
        }
    }

    return 0;
}