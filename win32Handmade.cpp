#include "handmade.h"

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
global_variable bool32 globalPause;

DEBUG_PLATFORM_FREE_FILE_MEMORY(DEBUGPlatformFreeFileMemory)
{
    if (memory)
    {
        VirtualFree(memory, 0, MEM_RELEASE);
    }
}

DEBUG_PLATFORM_READ_ENTIRE_FILE(DEBUGPlatformReadEntireFile)
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

DEBUG_PLATFORM_WRITE_ENTIRE_FILE(DEBUGPlatformWriteEntireFile)
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

struct win32_game_code
{
    HMODULE gameCodeDll;
    game_update_and_render *UpdateAndRender;
    game_get_sound_samples *GetSoundSamples;

    bool32 isValid;
};

internal win32_game_code
win32LoadGameCode()
{
    win32_game_code result = {};

    CopyFile("handmade.dll", "handmade_temp.dll", FALSE);
    result.gameCodeDll = LoadLibraryA("handmade_temp.dll");
    if (result.gameCodeDll)
    {
        result.UpdateAndRender = (game_update_and_render *)GetProcAddress(result.gameCodeDll, "gameUpdateAndRender");
        result.GetSoundSamples = (game_get_sound_samples *)GetProcAddress(result.gameCodeDll, "gameGetSoundSamples");
        result.isValid = (result.UpdateAndRender && result.GetSoundSamples);
    }

    if (!result.isValid)
    {
        result.UpdateAndRender = gameUpdateAndRenderStub;
        result.GetSoundSamples = gameGetSoundSamplesStub;
    }

    return result;
}

internal void
win32UnloadGameCode(win32_game_code *gameCode)
{
    if (gameCode->gameCodeDll)
    {
        FreeLibrary(gameCode->gameCodeDll);
        gameCode->gameCodeDll = 0;
    }
    gameCode->isValid = false;
    gameCode->UpdateAndRender = gameUpdateAndRenderStub;
    gameCode->GetSoundSamples = gameGetSoundSamplesStub;
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
#if HANDMADE_INTERNAL
                if (vkCode == 'P')
                {
                    if (isDown)
                    {
                        globalPause = !globalPause;
                    }
                }
#endif
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
win32DebugDrawVertical(win32_offscreen_buffer *backBuffer, int x, int top, int bottom, uint32 color)
{
    if (top <= 0)
    {
        top = 0;
    }
    if (bottom > backBuffer->height)
    {
        bottom = backBuffer->height;
    }
    if ((x >= 0) && (x < backBuffer->width))
    {
        uint8 *pixel = ((uint8 *)backBuffer->memory + x * backBuffer->bytesPerPixel + top * backBuffer->pitch);
        for (int y = top; y < bottom; y++)
        {
            *(uint32 *)pixel = color;
            pixel += backBuffer->pitch;
        }
    }
}

inline void
win32DrawSoundBufferMarker(win32_offscreen_buffer *backBuffer,
                           win32_sound_output *soundOutput,
                           real32 c, int padX, int top, int bottom,
                           DWORD value, uint32 color)
{
    real32 xReal32 = c * (real32)value;
    int x = padX + (int)xReal32;
    win32DebugDrawVertical(backBuffer, x, top, bottom, color);
}

internal void
win32DebugSyncDisplay(win32_offscreen_buffer *backBuffer,
                      int markerCount, win32_debug_time_marker *markers,
                      int currentMarkerIndex,
                      win32_sound_output *soundOutput, real32 targetSecondsPerFrame)
{
    int padX = 16;
    int padY = 16;

    int lineHeight = 64;

    real32 c = (real32)(backBuffer->width - 2 * padX) / (real32)soundOutput->secondaryBufferSize;
    for (int markerIndex = 0; markerIndex < markerCount; markerIndex++)
    {
        win32_debug_time_marker *thismarker = &markers[markerIndex];
        ASSERT(thismarker->outputPlayCursor < soundOutput->secondaryBufferSize);
        ASSERT(thismarker->outputWriteCursor < soundOutput->secondaryBufferSize);
        ASSERT(thismarker->outputLocation < soundOutput->secondaryBufferSize);
        ASSERT(thismarker->outputByteCount < soundOutput->secondaryBufferSize);
        ASSERT(thismarker->flipPlayCursor < soundOutput->secondaryBufferSize);
        ASSERT(thismarker->flipWriteCursor < soundOutput->secondaryBufferSize);

        DWORD playColor = 0xffffffff;
        DWORD writeColor = 0xffff0000;
        DWORD expectedFlipColor = 0xffffff00;
        DWORD playWindowColor = 0xffff00ff;

        int top = padY;
        int bottom = padY + lineHeight;
        if (markerIndex == currentMarkerIndex)
        {
            top += lineHeight + padY;
            bottom += lineHeight + padY;

            int firstTop = top;
            win32DrawSoundBufferMarker(backBuffer, soundOutput, c, padX, top, bottom, thismarker->outputPlayCursor, playColor);
            win32DrawSoundBufferMarker(backBuffer, soundOutput, c, padX, top, bottom, thismarker->outputWriteCursor, writeColor);

            top += lineHeight + padY;
            bottom += lineHeight + padY;

            win32DrawSoundBufferMarker(backBuffer, soundOutput, c, padX, top, bottom, thismarker->outputLocation, playColor);
            win32DrawSoundBufferMarker(backBuffer, soundOutput, c, padX, top, bottom, thismarker->outputLocation + thismarker->outputByteCount, writeColor);

            top += lineHeight + padY;
            bottom += lineHeight + padY;

            win32DrawSoundBufferMarker(backBuffer, soundOutput, c, padX, firstTop, bottom, thismarker->expectedFlipPlayCursor, expectedFlipColor);
        }

        win32DrawSoundBufferMarker(backBuffer, soundOutput, c, padX, top, bottom, thismarker->flipPlayCursor, playColor);
        win32DrawSoundBufferMarker(backBuffer, soundOutput, c, padX, top, bottom, thismarker->flipPlayCursor + 480 * soundOutput->bytesPerSample, playWindowColor);
        win32DrawSoundBufferMarker(backBuffer, soundOutput, c, padX, top, bottom, thismarker->flipWriteCursor, writeColor);
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
            soundOutput.latencySampleCount = 3 * (soundOutput.samplesPerSeconds / gameUpdateHz);
            soundOutput.safetyBytes = (soundOutput.samplesPerSeconds * soundOutput.bytesPerSample / gameUpdateHz) / 3;

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
            gameMemory.DEBUGPlatformFreeFileMemory = DEBUGPlatformFreeFileMemory;
            gameMemory.DEBUGPlatformReadEntireFile = DEBUGPlatformReadEntireFile;
            gameMemory.DEBUGPlatformWriteEntireFile = DEBUGPlatformWriteEntireFile;

            uint64 totalSize = gameMemory.permanentStorageSize + gameMemory.transientStorageSize;
            gameMemory.permanentStorage = VirtualAlloc(baseAddress, (size_t)totalSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
            gameMemory.transientStorage = ((uint8 *)gameMemory.permanentStorage + gameMemory.permanentStorageSize);

            if (samples && gameMemory.permanentStorage && gameMemory.transientStorage)
            {
                game_input input[2] = {};
                game_input *newInput = &input[0];
                game_input *oldInput = &input[1];

                LARGE_INTEGER lastCounter = win32GetWallClock();
                LARGE_INTEGER flipWallClock = win32GetWallClock();

                int debugTimeMarkerIndex = 0;
                win32_debug_time_marker debugTimeMarkers[gameUpdateHz / 2] = {0};

                DWORD audioLatencyBytes = 0;
                real32 audioLatencySeconds = 0;
                bool32 soundIsValid = false;

                uint64 lastCycleCount = __rdtsc();

                win32_game_code game = win32LoadGameCode();
                uint32 loadCounter = 0;

                while (globalRunning)
                {
                    if (loadCounter++ > 120)
                    {
                        win32UnloadGameCode(&game);
                        game = win32LoadGameCode();
                        loadCounter = 0;
                    }
                    game_controller_input *oldKeyboardController = getController(oldInput, 0);
                    game_controller_input *newKeyboardController = getController(newInput, 0);
                    *newKeyboardController = {};

                    newKeyboardController->isConnected = true;

                    for (int buttonIndex = 0; buttonIndex < ARRAY_COUNT(newKeyboardController->buttons); buttonIndex++)
                    {
                        newKeyboardController->buttons[buttonIndex].endedDown = oldKeyboardController->buttons[buttonIndex].endedDown;
                    }

                    win32ProcessPendingMessage(newKeyboardController);
                    if (!globalPause)
                    {
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

                        game_offscreen_buffer buffer;
                        buffer.height = globalBackBuffer.height;
                        buffer.width = globalBackBuffer.width;
                        buffer.memory = globalBackBuffer.memory;
                        buffer.pitch = globalBackBuffer.pitch;
                        game.UpdateAndRender(&gameMemory, newInput, &buffer);

                        LARGE_INTEGER audioWallClock = win32GetWallClock();
                        real32 fromBeginToAudioSeconds = win32GetSecondsElapsed(flipWallClock, audioWallClock);

                        DWORD playCursor;
                        DWORD writeCursor;
                        if (globalSecondaryBuffer->GetCurrentPosition(&playCursor, &writeCursor) == DS_OK)
                        {
                            if (!soundIsValid)
                            {
                                soundOutput.runningSampleIndex = writeCursor / soundOutput.bytesPerSample;
                                soundIsValid = true;
                            }

                            DWORD byteToLock = (soundOutput.runningSampleIndex * soundOutput.bytesPerSample) % soundOutput.secondaryBufferSize;

                            DWORD expectedSoundBytesPerFrame = (soundOutput.samplesPerSeconds * soundOutput.bytesPerSample) / gameUpdateHz;
                            real32 secondsLeftUntillFlip = targetSecondsPerFrame - fromBeginToAudioSeconds;
                            DWORD expectedBytesUntillFlip = (DWORD)((secondsLeftUntillFlip / targetSecondsPerFrame) * (real32)expectedSoundBytesPerFrame);

                            DWORD expectedFrameBoundaryByte = playCursor + expectedSoundBytesPerFrame;
                            DWORD safeWriteCursor = writeCursor;
                            if (safeWriteCursor < playCursor)
                            {
                                safeWriteCursor += soundOutput.secondaryBufferSize;
                            }
                            ASSERT(safeWriteCursor >= playCursor);
                            safeWriteCursor += soundOutput.safetyBytes;

                            bool32 audioCardIsLowLatency = (safeWriteCursor < expectedFrameBoundaryByte);

                            DWORD targetCursor = 0;
                            if (audioCardIsLowLatency)
                            {
                                targetCursor = (expectedFrameBoundaryByte + expectedSoundBytesPerFrame);
                            }
                            else
                            {
                                targetCursor = (writeCursor + expectedSoundBytesPerFrame + soundOutput.safetyBytes);
                            }
                            targetCursor = targetCursor % soundOutput.secondaryBufferSize;

                            DWORD bytesToWrite = 0;
                            if (byteToLock > targetCursor)
                            {
                                bytesToWrite = soundOutput.secondaryBufferSize - byteToLock;
                                bytesToWrite += targetCursor;
                            }
                            else
                            {
                                bytesToWrite = targetCursor - byteToLock;
                            }
                            game_sound_output_buffer soundBuffer = {};
                            soundBuffer.samplesPerSecond = soundOutput.samplesPerSeconds;
                            soundBuffer.sampleCount = bytesToWrite / soundOutput.bytesPerSample;
                            soundBuffer.samples = samples;
                            game.GetSoundSamples(&gameMemory, &soundBuffer);
#if HANDMADE_INTERNAL
                            win32_debug_time_marker *marker = &debugTimeMarkers[debugTimeMarkerIndex];
                            marker->outputPlayCursor = playCursor;
                            marker->outputWriteCursor = writeCursor;
                            marker->outputLocation = byteToLock;
                            marker->outputByteCount = bytesToWrite;
                            marker->expectedFlipPlayCursor = expectedFrameBoundaryByte;

                            DWORD unwrappedWriteCursor = writeCursor;
                            if (unwrappedWriteCursor < playCursor)
                            {
                                unwrappedWriteCursor += soundOutput.secondaryBufferSize;
                            }
                            audioLatencyBytes = unwrappedWriteCursor - playCursor;
                            audioLatencySeconds = ((real32)audioLatencyBytes / (real32)soundOutput.bytesPerSample) / (real32)soundOutput.samplesPerSeconds;

                            char textBuffer[256];
                            _snprintf_s(textBuffer, sizeof(textBuffer),
                                        "BTL: %u,    TC: %u,       BTW: %u,   -  PC: %u,     WC: %u,      DELTA: %u,           (%fs)\n",
                                        byteToLock, targetCursor, bytesToWrite, playCursor, writeCursor, audioLatencyBytes, audioLatencySeconds);
                            OutputDebugStringA(textBuffer);
#endif
                            win32FillSoundBuffer(&soundOutput, byteToLock, bytesToWrite, &soundBuffer);
                        }
                        else
                        {
                            soundIsValid = false;
                        }

                        LARGE_INTEGER workCounter = win32GetWallClock();
                        real32 workSecondsElapsed = win32GetSecondsElapsed(lastCounter, workCounter);

                        real32 secondsElapsedForFrame = workSecondsElapsed;
                        if (secondsElapsedForFrame < targetSecondsPerFrame)
                        {
                            if (sleepIsGranular)
                            {
                                // DWORD sleepMs = (DWORD)(1000.0f * (targetSecondsPerFrame - secondsElapsedForFrame));
                                DWORD sleepMs1tenth = (DWORD)((1000.0f * (targetSecondsPerFrame - secondsElapsedForFrame)) / 10.0f);
                                DWORD sleepMs = sleepMs1tenth * 8;
                                if (sleepMs > 0)
                                {
                                    Sleep(sleepMs);
                                }
                            }
                            // real32 testSecondsElapsedForFrame = win32GetSecondsElapsed(lastCounter, win32GetWallClock());
                            // if (testSecondsElapsedForFrame < targetSecondsPerFrame)
                            // {
                            // }
                            real32 secondsElapsedForFrame = win32GetSecondsElapsed(lastCounter, win32GetWallClock());
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
                        win32DebugSyncDisplay(&globalBackBuffer, ARRAY_COUNT(debugTimeMarkers), debugTimeMarkers, debugTimeMarkerIndex - 1, &soundOutput, targetSecondsPerFrame);
#endif
                        win32DisplayBufferInWindow(&globalBackBuffer, deviceContext, dimensions.width, dimensions.height);

                        flipWallClock = win32GetWallClock();
#if HANDMADE_INTERNAL
                        {
                            DWORD playCursor;
                            DWORD writeCursor;
                            if (globalSecondaryBuffer->GetCurrentPosition(&playCursor, &writeCursor) == DS_OK)
                            {
                                ASSERT(debugTimeMarkerIndex < ARRAY_COUNT(debugTimeMarkers));
                                win32_debug_time_marker *marker = &debugTimeMarkers[debugTimeMarkerIndex];

                                marker->flipPlayCursor = playCursor;
                                marker->flipWriteCursor = writeCursor;
                            }
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

#if HANDMADE_INTERNAL
                        debugTimeMarkerIndex++;
                        if (debugTimeMarkerIndex == ARRAY_COUNT(debugTimeMarkers))
                        {
                            debugTimeMarkerIndex = 0;
                        }
#endif
                    }
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