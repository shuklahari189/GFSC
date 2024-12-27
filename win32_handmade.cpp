#include <stdint.h>
#include <math.h>

#define internal static
#define local_persit static
#define global_variable static

#define Pi32 3.14159265359f

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int32 bool32;

typedef float real32;
typedef double real64;

#include "handmade.cpp"

#include <windows.h>
#include <stdio.h>
#include <malloc.h>
#include <xinput.h>
#include <dsound.h>
#include "win32_handmade.h"

global_variable int globalRunning;
global_variable win32OffScreenBuffer globalBackBuffer;
global_variable LPDIRECTSOUNDBUFFER globalSecondaryBuffer;

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

internal debugReadFileResult
DEBUGPlatformReadEntireFile(char *fileName)
{
    debugReadFileResult result = {};

    HANDLE fileHandle = CreateFileA(fileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if (fileHandle != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER fileSize;
        if (GetFileSizeEx(fileHandle, &fileSize))
        {
            uint32 fileSize32 = safeTruncateUInt32(fileSize.QuadPart);
            result.contents = VirtualAlloc(0, (uint32)fileSize.QuadPart, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
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
        }
        CloseHandle(fileHandle);
    }
    return result;
}

internal void
DEBUGPlatformFreeFileMemory(void *memory)
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
    return result;
}

internal void
win32clearBuffer(win32SoundOutput *soundOutput)
{
    void *region1;
    DWORD region1Size;
    void *region2;
    DWORD region2Size;
    if (SUCCEEDED(globalSecondaryBuffer->Lock(0, soundOutput->secondaryBufferSize, &region1, &region1Size, &region2, &region2Size, 0)))
    {
        uint8 *destSamples = (uint8 *)region1;
        for (DWORD byteIndex = 0; byteIndex < region1Size; byteIndex++)
        {
            *destSamples++ = 0;
        }
        destSamples = (uint8 *)region2;
        for (DWORD byteIndex = 0; byteIndex < region2Size; byteIndex++)
        {
            *destSamples++ = 0;
        }
        globalSecondaryBuffer->Unlock(region1, region1Size, region2, region2Size);
    }
}

internal void
win32fillSoundBuffer(win32SoundOutput *soundOutput, gameSoundOutputBuffer *sourceBuffer, DWORD byteToLock, DWORD bytesToWrite)
{
    void *region1;
    DWORD region1Size;
    void *region2;
    DWORD region2Size;

    if (SUCCEEDED(globalSecondaryBuffer->Lock(byteToLock, bytesToWrite, &region1, &region1Size, &region2, &region2Size, 0)))
    {
        DWORD region1SampleCount = region1Size / soundOutput->bytesPerSample;
        int16 *destSamples = (int16 *)region1;
        int16 *sourceSamples = sourceBuffer->samples;
        for (DWORD sampleIndex = 0; sampleIndex < region1SampleCount; sampleIndex++)
        {
            *destSamples++ = *sourceSamples++;
            *destSamples++ = *sourceSamples++;
            ++soundOutput->runningSampleIndex;
        }

        DWORD region2SampleCount = region2Size / soundOutput->bytesPerSample;
        destSamples = (int16 *)region2;
        for (DWORD sampleIndex = 0; sampleIndex < region2SampleCount; sampleIndex++)
        {
            *destSamples++ = *sourceSamples++;
            *destSamples++ = *sourceSamples++;
            ++soundOutput->runningSampleIndex;
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
        ASSERT(!"KEYBOARD INPUT CAME IN THROUGH A NON-DISPATCH MESSAGE");
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

internal void
win32processKeyboardMessage(gameButtonState *newState, bool32 isDown)
{
    newState->endedDown = isDown;
    ++newState->halfTransitionCount;
}

internal void
win32processXinputDigitalButton(DWORD xInputButtonState, gameButtonState *oldState, DWORD buttonBit, gameButtonState *newState)
{
    newState->endedDown = ((xInputButtonState & buttonBit) == buttonBit);
    newState->halfTransitionCount = (oldState->endedDown != newState->endedDown) ? 1 : 0;
}

internal void
win32processPendingMessages(gameControllerInput *keyboardController)
{
    MSG message;
    while (PeekMessageA(&message, 0, 0, 0, PM_REMOVE))
    {
        switch (message.message)
        {
        case WM_QUIT:
        {
            globalRunning = false;
        }
        break;
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYUP:
        case WM_KEYDOWN:
        {
            uint32 vkCode = (uint32)message.wParam;
            bool32 wasDown = ((message.lParam & (1 << 30)) != 0);
            bool32 isDown = ((message.lParam & (1 << 31)) == 0);
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
                if (vkCode == 'Q')
                {
                    win32processKeyboardMessage(&keyboardController->leftShoulder, isDown);
                }
                if (vkCode == 'E')
                {
                    win32processKeyboardMessage(&keyboardController->rightShoulder, isDown);
                }
                if (vkCode == VK_UP)
                {
                    win32processKeyboardMessage(&keyboardController->up, isDown);
                }
                if (vkCode == VK_DOWN)
                {
                    win32processKeyboardMessage(&keyboardController->down, isDown);
                }
                if (vkCode == VK_LEFT)
                {
                    win32processKeyboardMessage(&keyboardController->left, isDown);
                }
                if (vkCode == VK_RIGHT)
                {
                    win32processKeyboardMessage(&keyboardController->right, isDown);
                }
                if (vkCode == VK_SPACE)
                {
                }
                if (vkCode == VK_ESCAPE)
                {
                    globalRunning = false;
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
        break;
        };
    }
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

            win32SoundOutput soundOutput = {};

            soundOutput.samplesPerSecond = 48000;
            soundOutput.bytesPerSample = sizeof(int16) * 2;
            soundOutput.secondaryBufferSize = soundOutput.samplesPerSecond * soundOutput.bytesPerSample;
            soundOutput.latencySampleCount = soundOutput.samplesPerSecond / 15;
            win32initDSound(window, soundOutput.samplesPerSecond, soundOutput.secondaryBufferSize);
            win32clearBuffer(&soundOutput);
            globalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

            globalRunning = true;

            int16 *samples = (int16 *)VirtualAlloc(0, soundOutput.secondaryBufferSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

#if HANDMADE_INTERNAL
            LPVOID baseAddress = (LPVOID)TERABYTES(2);
#else
            LPVOID baseAddress = 0;
#endif
            gameMemory gameMem = {};
            gameMem.permanentStorageSize = MEGABYTES(64);
            gameMem.transientStorageSize = GIGABYTES(1);

            uint64 totalSize = gameMem.permanentStorageSize + gameMem.transientStorageSize;
            gameMem.permanentStorage = VirtualAlloc(baseAddress, (uint32)totalSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
            gameMem.transientStorage = ((uint8 *)gameMem.permanentStorage + gameMem.permanentStorageSize);

            if (samples && gameMem.permanentStorage && gameMem.transientStorage)
            {
                gameInput input[2] = {};
                gameInput *newInput = &input[0];
                gameInput *oldInput = &input[1];

                LARGE_INTEGER lastCounter;
                QueryPerformanceCounter(&lastCounter);
                uint64 lastCycleCount = __rdtsc();
                while (globalRunning)
                {
                    gameControllerInput *keyboardController = &newInput->controllers[0];
                    gameControllerInput zeroController = {};
                    *keyboardController = zeroController;

                    win32processPendingMessages(keyboardController);

                    DWORD maxControllerCount = XUSER_MAX_COUNT;
                    if (maxControllerCount > ARRAY_COUNT(newInput->controllers))
                    {
                        maxControllerCount = ARRAY_COUNT(newInput->controllers);
                    }
                    for (DWORD controllerIndex = 0; controllerIndex < maxControllerCount; controllerIndex++)
                    {
                        gameControllerInput *oldController = &oldInput->controllers[controllerIndex];
                        gameControllerInput *newController = &newInput->controllers[controllerIndex];

                        XINPUT_STATE controllerState;
                        if (XInputGetState(controllerIndex, &controllerState) == ERROR_SUCCESS)
                        {
                            // controllerIndex'th controller is plugged in
                            XINPUT_GAMEPAD *pad = &controllerState.Gamepad;

                            bool32 up = pad->wButtons & XINPUT_GAMEPAD_DPAD_UP;
                            bool32 down = pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN;
                            bool32 left = pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT;
                            bool32 right = pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT;

                            newController->isAnalog = true;
                            newController->startX = oldController->endX;
                            newController->startY = oldController->endY;

                            real32 x;
                            if (pad->sThumbLX < 0)
                            {
                                x = (real32)pad->sThumbLX / 32768.0f;
                            }
                            else
                            {
                                x = (real32)pad->sThumbLX / 32767.0f;
                            }
                            newController->minX = newController->maxX = newController->endX = x;

                            real32 y;
                            if (pad->sThumbLY < 0)
                            {
                                y = (real32)pad->sThumbLY / 32768.0f;
                            }
                            else
                            {
                                y = (real32)pad->sThumbLY / 32767.0f;
                            }
                            newController->minY = newController->maxY = newController->endY = y;

                            win32processXinputDigitalButton(pad->wButtons, &oldController->up, XINPUT_GAMEPAD_Y, &newController->up);
                            win32processXinputDigitalButton(pad->wButtons, &oldController->down, XINPUT_GAMEPAD_A, &newController->down);
                            win32processXinputDigitalButton(pad->wButtons, &oldController->left, XINPUT_GAMEPAD_X, &newController->left);
                            win32processXinputDigitalButton(pad->wButtons, &oldController->right, XINPUT_GAMEPAD_B, &newController->right);
                            win32processXinputDigitalButton(pad->wButtons, &oldController->leftShoulder, XINPUT_GAMEPAD_LEFT_SHOULDER, &newController->leftShoulder);
                            win32processXinputDigitalButton(pad->wButtons, &oldController->rightShoulder, XINPUT_GAMEPAD_RIGHT_SHOULDER, &newController->rightShoulder);

                            // bool32 start = pad->wButtons & XINPUT_GAMEPAD_START;
                            // bool32 back = pad->wButtons & XINPUT_GAMEPAD_BACK;
                        }
                        else
                        {
                            // controllerIndex'th controller is not plugged in
                        }
                    }

                    DWORD byteToLock = 0;
                    DWORD targetCursor = 0;
                    DWORD bytesToWrite = 0;
                    DWORD playCursor = 0;
                    DWORD writeCursor = 0;
                    bool32 soundIsValid = false;
                    if (SUCCEEDED(globalSecondaryBuffer->GetCurrentPosition(&playCursor, &writeCursor)))
                    {
                        byteToLock = (soundOutput.runningSampleIndex * soundOutput.bytesPerSample) % soundOutput.secondaryBufferSize;
                        targetCursor = (playCursor + (soundOutput.latencySampleCount * soundOutput.bytesPerSample)) % soundOutput.secondaryBufferSize;
                        if (byteToLock > targetCursor)
                        {
                            bytesToWrite = soundOutput.secondaryBufferSize - byteToLock;
                            bytesToWrite += targetCursor;
                        }
                        else
                        {
                            bytesToWrite = targetCursor - byteToLock;
                        }
                        soundIsValid = true;
                    }

                    gameSoundOutputBuffer soundBuffer = {};
                    soundBuffer.samplesPerSecond = soundOutput.samplesPerSecond;
                    soundBuffer.sampleCount = bytesToWrite / soundOutput.bytesPerSample;
                    soundBuffer.samples = samples;

                    gameOffScreenBuffer buffer = {};
                    buffer.memory = globalBackBuffer.memory;
                    buffer.width = globalBackBuffer.width;
                    buffer.height = globalBackBuffer.height;
                    buffer.pitch = globalBackBuffer.pitch;
                    gameUpdateAndRender(&gameMem, newInput, &buffer, &soundBuffer);

                    if (soundIsValid)
                    {
                        win32fillSoundBuffer(&soundOutput, &soundBuffer, byteToLock, bytesToWrite);
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
#if 0
                char buffer[256];
                sprintf(buffer, "%.02fms/f, %.02ff/s, %.02fmc/f\n", msPerFrame, FPS, MCPF);
                OutputDebugStringA(buffer);
#endif
                    lastCounter = endCounter;
                    lastCycleCount = endCycleCount;

                    gameInput *temp = newInput;
                    newInput = oldInput;
                    oldInput = temp;
                }
            }
        }
    }

    return 0;
}