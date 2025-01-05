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

#include "handmade.cpp"

#include <windows.h>
#include <stdio.h>
#include <malloc.h>
#include <xinput.h>
#include <dsound.h>

#include "win32Handmade.h"

global_variable bool32 globalRunning;
global_variable win32_offscreen_buffer globalBackBuffer;
global_variable LPDIRECTSOUNDBUFFER globalSecondaryBuffer;
global_variable int64 globalPerfCountFrequency;

internal debug_read_file_result
DEBUGPlatformReadEntireFile(char *fileName)
{
    debug_read_file_result result = {};
    HANDLE fileHandle = CreateFileA(fileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if (fileHandle != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER fileSize;
        if (GetFileSizeEx(fileHandle, &fileSize))
        {
            uint32 fileSize32 = safeTruncateUInt64(fileSize.QuadPart);
            result.contents = VirtualAlloc(0, fileSize32, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
            if (result.contents)
            {
                DWORD bytesRead;
                if (ReadFile(fileHandle, result.contents, fileSize32, &bytesRead, 0) && (fileSize32 == bytesRead))
                {
                    result.contentSize = fileSize32;
                }
                else
                {
                    DEBUGPlatformFreeFileMemory(result.contents);
                    result.contents = 0;
                }
            }
            else
            {
            }
        }
        else
        {
        }
        CloseHandle(fileHandle);
    }
    else
    {
    }

    return result;
}

internal void DEBUGPlatformFreeFileMemory(void *memory)
{
    if (memory)
    {
        VirtualFree(memory, 0, MEM_RELEASE);
    }
}

internal bool32
DEBUGPlatformWriteEntireFile(char *fileName, uint32 memorySize, void *memory)
{
    bool32 result = false;
    HANDLE fileHandle = CreateFileA(fileName, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
    if (fileHandle != INVALID_HANDLE_VALUE)
    {
        DWORD bytesWritten;
        if (WriteFile(fileHandle, memory, memorySize, &bytesWritten, 0))
        {
            result = (bytesWritten == memorySize);
        }
        else
        {
        }
        CloseHandle(fileHandle);
    }
    else
    {
    }

    return result;
}

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

internal void
win32LoadXInput()
{
    // xinput1_3.dll
    // xinput1_4.dll
    // XInput9_1_0.dll
    HMODULE XInputLibrary = LoadLibraryA("xinput1_4.dll");
    if (!XInputLibrary)
    {
        XInputLibrary = LoadLibraryA("XInput9_1_0.dll");
    }
    if (!XInputLibrary)
    {
        XInputLibrary = LoadLibraryA("xinput1_3.dll");
    }

    if (XInputLibrary)
    {
        XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
        if (!XInputGetState)
        {
            XInputGetState = XInputGetStateStub;
        }
        XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
        if (!XInputSetState)
        {
            XInputSetState = XInputSetStateStub;
        }
    }
}

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);

internal void
win32InitDSound(HWND window, int32 samplesPerSeconds, int32 bufferSize)
{
    HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");
    if (DSoundLibrary)
    {
        direct_sound_create *DirectSoundCreate = (direct_sound_create *)GetProcAddress(DSoundLibrary, "DirectSoundCreate");

        LPDIRECTSOUND directSound;
        if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &directSound, 0)))
        {
            WAVEFORMATEX waveFormat = {};
            waveFormat.wFormatTag = WAVE_FORMAT_PCM;
            waveFormat.nChannels = 2;
            waveFormat.nSamplesPerSec = samplesPerSeconds;
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
                        OutputDebugStringA("Suceeded setting primary buffer format.\n");
                    }
                    else
                    {
                    }
                }
            }
            else
            {
            }
            DSBUFFERDESC bufferDescription = {};
            bufferDescription.dwSize = sizeof(bufferDescription);
            bufferDescription.dwBufferBytes = bufferSize;
            bufferDescription.lpwfxFormat = &waveFormat;
            bufferDescription.dwFlags = DSBCAPS_GETCURRENTPOSITION2;
            if (SUCCEEDED(directSound->CreateSoundBuffer(&bufferDescription, &globalSecondaryBuffer, 0)))
            {
                OutputDebugStringA("succeded creating secondary buffer.\n");
            }
        }
        else
        {
        }
    }
    else
    {
    }
}

internal win32_window_dimensions win32GetWindowDimensions(HWND window)
{
    win32_window_dimensions result;

    RECT clientRect;
    GetClientRect(window, &clientRect);
    result.width = clientRect.right - clientRect.left;
    result.height = clientRect.bottom - clientRect.top;
    return result;
}

internal void
win32ResizeDIBSection(win32_offscreen_buffer *buffer, int width, int height)
{
    if (buffer->memory)
    {
        VirtualFree(buffer->memory, 0, MEM_RELEASE);
    }

    buffer->width = width;
    buffer->height = height;
    int bytesPerPixel = 4;
    buffer->bytesPerPixel = bytesPerPixel;

    buffer->info.bmiHeader.biSize = sizeof(buffer->info.bmiHeader);
    buffer->info.bmiHeader.biWidth = width;
    buffer->info.bmiHeader.biHeight = -height;
    buffer->info.bmiHeader.biPlanes = 1;
    buffer->info.bmiHeader.biBitCount = 32;
    buffer->info.bmiHeader.biCompression = BI_RGB;

    int bitmapMemorySize = (width * height) * bytesPerPixel;
    buffer->memory = VirtualAlloc(0, bitmapMemorySize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    buffer->pitch = buffer->width * bytesPerPixel;
}

internal void
win32DisplayBufferInWindow(win32_offscreen_buffer *buffer, HDC deviceContext, int windowWidth, int windowHeight)
{
    StretchDIBits(deviceContext,
                  0, 0, windowWidth, windowHeight,
                  0, 0, buffer->width, buffer->height,
                  buffer->memory, &buffer->info,
                  DIB_RGB_COLORS, SRCCOPY);
}

internal LRESULT CALLBACK
win32MainWindowindowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    {
        ASSERT(!"no !")
    }
    case WM_ACTIVATEAPP:
    {
    }
        return 0;
    case WM_DESTROY:
    {
        globalRunning = false;
    }
        return 0;
    case WM_PAINT:
    {
        PAINTSTRUCT paint;
        HDC deviceContext = BeginPaint(window, &paint);
        win32_window_dimensions dimensions = win32GetWindowDimensions(window);
        win32DisplayBufferInWindow(&globalBackBuffer, deviceContext, dimensions.width, dimensions.height);
        EndPaint(window, &paint);
    }
        return 0;
    }
    return DefWindowProcA(window, message, wParam, lParam);
}

internal void
win32ClearBuffer(win32_sound_output *soundOutput)
{
    void *region1;
    DWORD region1Size;
    void *region2;
    DWORD region2Size;
    if (SUCCEEDED(globalSecondaryBuffer->Lock(0, soundOutput->secondaryBufferSize, &region1, &region1Size, &region2, &region2Size, 0)))
    {
        uint8 *destSample = (uint8 *)region1;
        for (DWORD byteIndex = 0; byteIndex < region1Size; byteIndex++)
        {
            *destSample++ = 0;
        }
        destSample = (uint8 *)region2;
        for (DWORD byteIndex = 0; byteIndex < region2Size; byteIndex++)
        {
            *destSample++ = 0;
        }
        globalSecondaryBuffer->Unlock(region1, region1Size, region2, region2Size);
    }
}

internal void
win32FillSoundBuffer(win32_sound_output *soundOutput, DWORD byteToLock, DWORD bytesToWrite, game_sound_output_buffer *source)
{
    void *region1;
    DWORD region1Size;
    void *region2;
    DWORD region2Size;
    if (SUCCEEDED(globalSecondaryBuffer->Lock(byteToLock, bytesToWrite, &region1, &region1Size, &region2, &region2Size, 0)))
    {
        DWORD region1SampleCount = region1Size / soundOutput->bytesPerSample;
        int16 *destSample = (int16 *)region1;
        int16 *sourceSample = (int16 *)source->samples;
        for (DWORD sampleIndex = 0; sampleIndex < region1SampleCount; sampleIndex++)
        {
            *destSample++ = *sourceSample++;
            *destSample++ = *sourceSample++;
            soundOutput->runningSampleIndex++;
        }

        DWORD region2SampleCount = region2Size / soundOutput->bytesPerSample;
        destSample = (int16 *)region2;
        for (DWORD sampleIndex = 0; sampleIndex < region2SampleCount; sampleIndex++)
        {
            *destSample++ = *sourceSample++;
            *destSample++ = *sourceSample++;
            soundOutput->runningSampleIndex++;
        }
        globalSecondaryBuffer->Unlock(region1, region1Size, region2, region2Size);
    }
}

internal void
win32ProcessKeyboardMessage(game_button_state *newState, bool32 isDown)
{
    ASSERT(newState->endedDown != isDown);
    newState->endedDown = isDown;
    newState->halfTransitionState++;
}

internal void
win32processXInputDigitalButton(DWORD XInputButtonState, game_button_state *oldState, DWORD buttonBit, game_button_state *newState)
{
    newState->endedDown = ((XInputButtonState & buttonBit) == buttonBit);
    newState->halfTransitionState = (oldState->endedDown != newState->endedDown) ? 1 : 0;
}

internal void
win32ProcessPendingMessage(game_controller_input *keyboardController)
{
    MSG message;
    while (PeekMessageA(&message, 0, 0, 0, PM_REMOVE) > 0)
    {
        switch (message.message)
        {
        case WM_QUIT:
        {
            globalRunning = false;
        }
        break;
        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        {
            uint32 vkCode = (uint32)message.wParam;
            bool32 wasDown = ((message.lParam & (1 << 30)) != 0);
            bool32 isDown = ((message.lParam & (1 << 31)) == 0);
            if (wasDown != isDown)
            {
                if (vkCode == 'W')
                {
                    win32ProcessKeyboardMessage(&keyboardController->moveUp, isDown);
                }
                if (vkCode == 'A')
                {
                    win32ProcessKeyboardMessage(&keyboardController->moveLeft, isDown);
                }
                if (vkCode == 'S')
                {
                    win32ProcessKeyboardMessage(&keyboardController->moveDown, isDown);
                }
                if (vkCode == 'D')
                {
                    win32ProcessKeyboardMessage(&keyboardController->moveRight, isDown);
                }
                if (vkCode == 'Q')
                {
                    win32ProcessKeyboardMessage(&keyboardController->leftShoulder, isDown);
                }
                if (vkCode == 'E')
                {
                    win32ProcessKeyboardMessage(&keyboardController->rightShoulder, isDown);
                }
                if (vkCode == VK_UP)
                {
                    win32ProcessKeyboardMessage(&keyboardController->actionUp, isDown);
                }
                if (vkCode == VK_LEFT)
                {
                    win32ProcessKeyboardMessage(&keyboardController->actionLeft, isDown);
                }
                if (vkCode == VK_DOWN)
                {
                    win32ProcessKeyboardMessage(&keyboardController->actionDown, isDown);
                }
                if (vkCode == VK_RIGHT)
                {
                    win32ProcessKeyboardMessage(&keyboardController->actionRight, isDown);
                }
                if (vkCode == VK_SPACE)
                {
                }
                if (vkCode == VK_ESCAPE)
                {
                }
            }

            bool32 altKeyWasDown = (message.lParam & (1 << 29));
            if ((vkCode == VK_F4) && altKeyWasDown)
            {
                globalRunning = false;
            }
        }
        break;
        default:
        {
            TranslateMessage(&message);
            DispatchMessageA(&message);
        }
        }
    }
}

internal real32
win32ProcessXInputStickValue(SHORT value, SHORT deadzoneThreshold)
{
    real32 result = 0;
    if (value < -deadzoneThreshold)
    {
        result = (real32)value / 32768.0f;
    }
    else if (value > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
    {
        result = (real32)value / 32767.0f;
    }
    return result;
}

inline LARGE_INTEGER
win32GetWallClock()
{
    LARGE_INTEGER result;
    QueryPerformanceCounter(&result);
    return result;
}

inline real32
win32GetSecondsElapsed(LARGE_INTEGER start, LARGE_INTEGER end)
{
    real32 result = (real32)(end.QuadPart - start.QuadPart) / (real32)globalPerfCountFrequency;
    return result;
}

internal void
win32DebugDrawVertical(win32_offscreen_buffer *globalBackBuffer, int x, int top, int bottom, uint32 color)
{
    uint8 *pixel = ((uint8 *)globalBackBuffer->memory + x * globalBackBuffer->bytesPerPixel + top * globalBackBuffer->pitch);
    for (int y = top; y < bottom; y++)
    {
        *(uint32 *)pixel = color;
        pixel += globalBackBuffer->pitch;
    }
}

inline void
win32DrawSoundBufferMarker(win32_offscreen_buffer *backBuffer,
                           win32_sound_output *soundOutput,
                           real32 c, int padX, int top, int bottom,
                           DWORD value, uint32 color)
{
    ASSERT(value < soundOutput->secondaryBufferSize);
    real32 xReal32 = c * (real32)value;
    int x = padX + (int)xReal32;
    win32DebugDrawVertical(backBuffer, x, top, bottom, color);
}

internal void
win32DebugSyncDisplay(win32_offscreen_buffer *backBuffer,
                      int markerCount, win32_debug_time_marker *markers,
                      win32_sound_output *soundOutput, real32 targetSecondsPerFrame)
{
    int padX = 16;
    int padY = 16;

    int top = padY;
    int bottom = backBuffer->height - padY;

    real32 c = (real32)(backBuffer->width - 2 * padX) / (real32)soundOutput->secondaryBufferSize;
    for (int markerIndex = 0; markerIndex < markerCount; markerIndex++)
    {
        win32_debug_time_marker *thismarker = &markers[markerIndex];
        win32DrawSoundBufferMarker(backBuffer, soundOutput, c, padX, top, bottom, thismarker->playCursor, 0xffffffff);
        win32DrawSoundBufferMarker(backBuffer, soundOutput, c, padX, top, bottom, thismarker->writeCursor, 0xffff00ff);
    }
}

int CALLBACK
WinMain(HINSTANCE instance, HINSTANCE prevInstance,
        PSTR commandLine, int showindowClassode)
{
    LARGE_INTEGER perfCountFrequencyResult;
    QueryPerformanceFrequency(&perfCountFrequencyResult);
    globalPerfCountFrequency = perfCountFrequencyResult.QuadPart;

    UINT desiredSchedularMS = 1;

    bool32 sleepIsGranular = (timeBeginPeriod(desiredSchedularMS) == TIMERR_NOERROR);

    win32LoadXInput();

    win32ResizeDIBSection(&globalBackBuffer, 1280, 720);
    WNDCLASSA windowClass = {};

    windowClass.lpfnWndProc = win32MainWindowindowCallback;
    windowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    windowClass.hInstance = instance;
    windowClass.lpszClassName = "handmadeHeroWindowClass";

#define framesOfAudioLatency 3
#define monitorRefreshHertz 60
#define gameUpdateHz (monitorRefreshHertz / 2)

    real32 targetSecondsPerFrame = 1.0f / (real32)gameUpdateHz;

    if (RegisterClassA(&windowClass))
    {
        HWND window = CreateWindowExA(0,
                                      windowClass.lpszClassName,
                                      "Handmade hero",
                                      WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                      CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                      0, 0, instance,
                                      0);

        if (window)
        {
            HDC deviceContext = GetDC(window);
            win32_sound_output soundOutput = {};

            soundOutput.samplesPerSeconds = 48000;
            soundOutput.runningSampleIndex = 0;
            soundOutput.bytesPerSample = sizeof(int16) * 2;
            soundOutput.secondaryBufferSize = soundOutput.samplesPerSeconds * soundOutput.bytesPerSample;
            soundOutput.latencySampleCount = framesOfAudioLatency * (soundOutput.samplesPerSeconds / gameUpdateHz);
            win32InitDSound(window, soundOutput.samplesPerSeconds, soundOutput.secondaryBufferSize);
            win32ClearBuffer(&soundOutput);
            globalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

            globalRunning = true;
#if 0
            while (globalRunning)
            {
                DWORD playCursor;
                DWORD writeCursor;
                globalSecondaryBuffer->GetCurrentPosition(&playCursor, &writeCursor);

                char textBuffer[256];
                _snprintf_s(textBuffer, sizeof(textBuffer), "PC: %u WC: %u\n", playCursor, writeCursor);
                OutputDebugStringA(textBuffer);
            }
#endif
            int16 *samples = (int16 *)VirtualAlloc(0, soundOutput.secondaryBufferSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

#if HANDMADE_INTERNAL
            LPVOID baseAddress = (LPVOID)TERABYTES(2);
#else
            LPVOID baseAddress = 0;
#endif
            game_memory gameMemory = {};
            gameMemory.permanentStorageSize = MEGABYTES(64);
            gameMemory.transientStorageSize = GIGABYTES(1);

            uint64 totalSize = gameMemory.permanentStorageSize + gameMemory.transientStorageSize;
            gameMemory.permanentStorage = VirtualAlloc(baseAddress, (size_t)totalSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
            gameMemory.transientStorage = ((uint8 *)gameMemory.permanentStorage + gameMemory.permanentStorageSize);

            if (samples && gameMemory.permanentStorage && gameMemory.transientStorage)
            {
                game_input input[2] = {};
                game_input *newInput = &input[0];
                game_input *oldInput = &input[1];

                int debugTimeMarkerIndex = 0;
                win32_debug_time_marker debugTimeMarkers[gameUpdateHz / 2] = {0};

                DWORD lastPlayCursor = 0;
                bool32 soundIsValid = false;

                LARGE_INTEGER lastCounter = win32GetWallClock();
                uint64 lastCycleCount = __rdtsc();

                while (globalRunning)
                {
                    game_controller_input *oldKeyboardController = getController(oldInput, 0);
                    game_controller_input *newKeyboardController = getController(newInput, 0);
                    *newKeyboardController = {};

                    newKeyboardController->isConnected = true;

                    for (int buttonIndex = 0; buttonIndex < ARRAY_COUNT(newKeyboardController->buttons); buttonIndex++)
                    {
                        newKeyboardController->buttons[buttonIndex].endedDown = oldKeyboardController->buttons[buttonIndex].endedDown;
                    }

                    win32ProcessPendingMessage(newKeyboardController);

                    DWORD maxControllerCount = XUSER_MAX_COUNT;
                    if (maxControllerCount > (ARRAY_COUNT(newInput->controllers) - 1))
                    {
                        maxControllerCount = (ARRAY_COUNT(newInput->controllers) - 1);
                    }
                    for (DWORD controllerIndex = 0; controllerIndex < maxControllerCount; controllerIndex++)
                    {
                        int ourControllerIndex = controllerIndex + 1;
                        game_controller_input *oldController = getController(oldInput, ourControllerIndex);
                        game_controller_input *newController = getController(newInput, ourControllerIndex);

                        XINPUT_STATE controllerState;
                        if (XInputGetState(controllerIndex, &controllerState) == ERROR_SUCCESS)
                        {
                            // THIS CONTROLLER IS PLUGGED IN
                            newController->isConnected = true;
                            XINPUT_GAMEPAD *pad = &controllerState.Gamepad;

                            if ((newController->stickAverageX != 0.0f) || (newController->stickAverageY != 0.0f))
                            {
                                newController->isAnalog = true;
                            }
                            newController->stickAverageX = win32ProcessXInputStickValue(pad->sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
                            newController->stickAverageY = win32ProcessXInputStickValue(pad->sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);

                            if (pad->wButtons & XINPUT_GAMEPAD_DPAD_UP)
                            {
                                newController->stickAverageX = 1.0f;
                                newController->isAnalog = false;
                            }
                            if (pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN)
                            {
                                newController->stickAverageY = -1.0f;
                                newController->isAnalog = false;
                            }
                            if (pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT)
                            {
                                newController->stickAverageX = -1.0f;
                                newController->isAnalog = false;
                            }
                            if (pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT)
                            {
                                newController->stickAverageY = 1.0f;
                                newController->isAnalog = false;
                            }

                            real32 threshold = 0.5f;
                            win32processXInputDigitalButton((newController->stickAverageX < -threshold) ? 1 : 0,
                                                            &oldController->moveLeft, 1,
                                                            &newController->moveLeft);
                            win32processXInputDigitalButton((newController->stickAverageX > threshold) ? 1 : 0,
                                                            &oldController->moveRight, 1,
                                                            &newController->moveRight);
                            win32processXInputDigitalButton((newController->stickAverageY < -threshold) ? 1 : 0,
                                                            &oldController->moveDown, 1,
                                                            &newController->moveDown);
                            win32processXInputDigitalButton((newController->stickAverageY > threshold) ? 1 : 0,
                                                            &oldController->moveUp, 1,
                                                            &newController->moveUp);

                            win32processXInputDigitalButton(pad->wButtons, &oldController->actionDown, XINPUT_GAMEPAD_A, &newController->actionDown);
                            win32processXInputDigitalButton(pad->wButtons, &oldController->actionRight, XINPUT_GAMEPAD_B, &newController->actionRight);
                            win32processXInputDigitalButton(pad->wButtons, &oldController->actionLeft, XINPUT_GAMEPAD_X, &newController->actionLeft);
                            win32processXInputDigitalButton(pad->wButtons, &oldController->actionUp, XINPUT_GAMEPAD_Y, &newController->actionUp);
                            win32processXInputDigitalButton(pad->wButtons, &oldController->leftShoulder, XINPUT_GAMEPAD_LEFT_SHOULDER, &newController->leftShoulder);
                            win32processXInputDigitalButton(pad->wButtons, &oldController->rightShoulder, XINPUT_GAMEPAD_RIGHT_SHOULDER, &newController->rightShoulder);
                            win32processXInputDigitalButton(pad->wButtons, &oldController->start, XINPUT_GAMEPAD_START, &newController->start);
                            win32processXInputDigitalButton(pad->wButtons, &oldController->back, XINPUT_GAMEPAD_BACK, &newController->back);
                        }
                        else
                        {
                            // THIS CONTROLLER IS NOT PLUGGED IN
                            newController->isConnected = false;
                        }
                    }

                    DWORD byteToLock = 0;
                    DWORD targetCursor = 0;
                    DWORD bytesToWrite = 0;
                    if (soundIsValid)
                    {
                        byteToLock = (soundOutput.runningSampleIndex * soundOutput.bytesPerSample) % soundOutput.secondaryBufferSize;
                        targetCursor = (lastPlayCursor + soundOutput.latencySampleCount * soundOutput.bytesPerSample) % soundOutput.secondaryBufferSize;
                        if (byteToLock > targetCursor)
                        {
                            bytesToWrite = soundOutput.secondaryBufferSize - byteToLock;
                            bytesToWrite += targetCursor;
                        }
                        else
                        {
                            bytesToWrite = targetCursor - byteToLock;
                        }
                    }

                    game_sound_output_buffer soundBuffer = {};
                    soundBuffer.samplesPerSecond = soundOutput.samplesPerSeconds;
                    soundBuffer.sampleCount = bytesToWrite / soundOutput.bytesPerSample;
                    soundBuffer.samples = samples;

                    game_offscreen_buffer buffer;
                    buffer.height = globalBackBuffer.height;
                    buffer.width = globalBackBuffer.width;
                    buffer.memory = globalBackBuffer.memory;
                    buffer.pitch = globalBackBuffer.pitch;
                    gameUpdateAndRender(&gameMemory, newInput, &buffer, &soundBuffer);

                    if (soundIsValid)
                    {
#if HANDMADE_INTERNAL
                        DWORD playCursor;
                        DWORD writeCursor;
                        globalSecondaryBuffer->GetCurrentPosition(&playCursor, &writeCursor);
                        char textBuffer[256];
                        _snprintf_s(textBuffer, sizeof(textBuffer),
                                    "LPC: %u,       BTL: %u,    TC: %u,       BTW: %u,   -  PC: %u,     WC: %u\n",
                                    lastPlayCursor, byteToLock, targetCursor, bytesToWrite, playCursor, writeCursor);
                        OutputDebugStringA(textBuffer);
#endif
                        win32FillSoundBuffer(&soundOutput, byteToLock, bytesToWrite, &soundBuffer);
                    }

                    LARGE_INTEGER workCounter = win32GetWallClock();
                    real32 workSecondsElapsed = win32GetSecondsElapsed(lastCounter, workCounter);

                    real32 secondsElapsedForFrame = workSecondsElapsed;
                    if (secondsElapsedForFrame < targetSecondsPerFrame)
                    {
                        if (sleepIsGranular)
                        {
                            DWORD sleepMs1tenth = (DWORD)((1000.0f * (targetSecondsPerFrame - secondsElapsedForFrame)) / 10.0f);
                            DWORD sleepMs = sleepMs1tenth * 8; // change this 8 from [1,10]
                            if (sleepMs > 0)
                            {
                                Sleep(sleepMs);
                            }
                        }
                        // real32 testSecondsElapsedForFrame = win32GetSecondsElapsed(lastCounter, win32GetWallClock());
                        // ASSERT(testSecondsElapsedForFrame < targetSecondsPerFrame);
                        secondsElapsedForFrame = win32GetSecondsElapsed(lastCounter, win32GetWallClock());
                        while (secondsElapsedForFrame < targetSecondsPerFrame)
                        {
                            secondsElapsedForFrame = win32GetSecondsElapsed(lastCounter, win32GetWallClock());
                        }
                    }
                    else
                    {
                    }

                    LARGE_INTEGER endCounter = win32GetWallClock();
                    real32 msPerFrame = 1000.0f * win32GetSecondsElapsed(lastCounter, endCounter);
                    lastCounter = endCounter;

                    win32_window_dimensions dimensions = win32GetWindowDimensions(window);
#if HANDMADE_INTERNAL
                    win32DebugSyncDisplay(&globalBackBuffer, ARRAY_COUNT(debugTimeMarkers), debugTimeMarkers, &soundOutput, targetSecondsPerFrame);
#endif
                    win32DisplayBufferInWindow(&globalBackBuffer, deviceContext, dimensions.width, dimensions.height);

                    DWORD playCursor;
                    DWORD writeCursor;
                    if (globalSecondaryBuffer->GetCurrentPosition(&playCursor, &writeCursor) == DS_OK)
                    {
                        lastPlayCursor = playCursor;
                        if (!soundIsValid)
                        {
                            soundOutput.runningSampleIndex = writeCursor / soundOutput.bytesPerSample;
                            soundIsValid = true;
                        }
                    }
                    else
                    {
                        soundIsValid = false;
                    }
#if HANDMADE_INTERNAL
                    {
                        ASSERT(debugTimeMarkerIndex < ARRAY_COUNT(debugTimeMarkers));
                        win32_debug_time_marker *marker = &debugTimeMarkers[debugTimeMarkerIndex++];
                        if (debugTimeMarkerIndex == ARRAY_COUNT(debugTimeMarkers))
                        {
                            debugTimeMarkerIndex = 0;
                        }
                        marker->playCursor = playCursor;
                        marker->writeCursor = writeCursor;
                    }
#endif

                    game_input *temp = newInput;
                    newInput = oldInput;
                    oldInput = temp;

                    uint64 endCycleCount = __rdtsc();
                    uint64 cycleElapsed = endCycleCount - lastCycleCount;
                    lastCycleCount = endCycleCount;

                    real64 fps = 0.0f;
                    real64 mcpf = ((real64)cycleElapsed / (1000.0f * 1000.0f));

                    char FPSbuffer[256];
                    _snprintf_s(FPSbuffer, sizeof(FPSbuffer), "ms/f: %.2f, fps: %.2f, mc/f: %.2f\n", msPerFrame, fps, mcpf);
                    OutputDebugStringA(FPSbuffer);
                }
            }
            else
            {
            }
        }
        else
        {
        }
    }
    else
    {
    }

    return 0;
}