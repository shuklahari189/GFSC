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

internal void
CatStrings(size_t sourceACount, char *sourceA, size_t sourceBCount, char *sourceB, size_t destCount, char *dest)
{
    for (int index = 0; index < sourceACount; index++)
    {
        *dest++ = *sourceA++;
    }
    for (int index = 0; index < sourceBCount; index++)
    {
        *dest++ = *sourceB++;
    }
    *dest++ = 0;
}

internal int StringLength(char *string)
{
    int count = 0;
    while (*string++)
    {
        ++count;
    }
    return count;
}

internal void
win32BuildExePathFileName(win32_state *state, char *fileName, int destCount, char *dest)
{
    CatStrings(state->onePastLastExeFileNameSlash - state->exeFileName, state->exeFileName, StringLength(fileName), fileName, destCount, dest);
}

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
                    DEBUGPlatformFreeFileMemory(thread, result.contents);
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

inline FILETIME
win32GetLastWriteTime(char *fileName)
{
    FILETIME lastWriteTime = {};

    WIN32_FILE_ATTRIBUTE_DATA data;
    if (GetFileAttributesExA(fileName, GetFileExInfoStandard, &data))
    {
        lastWriteTime = data.ftLastWriteTime;
    }

    return lastWriteTime;
}

internal win32_game_code
win32LoadGameCode(char *sourceDllName, char *tempDllName)
{
    win32_game_code result = {};

    result.DLLLastWriteTime = win32GetLastWriteTime(sourceDllName);

    CopyFile(sourceDllName, tempDllName, FALSE);
    result.gameCodeDll = LoadLibraryA(tempDllName);
    if (result.gameCodeDll)
    {
        result.UpdateAndRender = (game_update_and_render *)GetProcAddress(result.gameCodeDll, "gameUpdateAndRender");
        result.GetSoundSamples = (game_get_sound_samples *)GetProcAddress(result.gameCodeDll, "gameGetSoundSamples");
        result.isValid = (result.UpdateAndRender && result.GetSoundSamples);
    }

    if (!result.isValid)
    {
        result.UpdateAndRender = 0;
        result.GetSoundSamples = 0;
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
    gameCode->UpdateAndRender = 0;
    gameCode->GetSoundSamples = 0;
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

internal win32_window_dimensions
win32GetWindowDimensions(HWND window)
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
                  //   0, 0, windowWidth, windowHeight,
                  0, 0, buffer->width, buffer->height,
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
#if 0
        if (wParam == TRUE)
        {
            SetLayeredWindowAttributes(window, RGB(0, 0, 0), 255, LWA_ALPHA);
        }
        else
        {
            SetLayeredWindowAttributes(window, RGB(0, 0, 0), 64, LWA_ALPHA);
        }
#endif
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
    if (newState->endedDown != isDown)
    {
        newState->endedDown = isDown;
        newState->halfTransitionState++;
    }
}

internal void
win32processXInputDigitalButton(DWORD XInputButtonState, game_button_state *oldState, DWORD buttonBit, game_button_state *newState)
{
    newState->endedDown = ((XInputButtonState & buttonBit) == buttonBit);
    newState->halfTransitionState = (oldState->endedDown != newState->endedDown) ? 1 : 0;
}

internal void
win32GetInputFileLocation(win32_state *state, bool32 inputStream, int slotIndex, int destCount, char *dest)
{
    char temp[64];
    wsprintf(temp, "loopEdit_%d_%s.hmi", slotIndex, inputStream ? "input" : "state");
    win32BuildExePathFileName(state, temp, destCount, dest);
}

internal win32_replay_buffer *
win32GetReplayBuffer(win32_state *state, int unsigned index)
{
    ASSERT(index < ARRAY_COUNT(state->replayBuffers));
    win32_replay_buffer *result = &state->replayBuffers[index];
    return result;
}

internal void
win32BeginRecordingInput(win32_state *state, int inputRecordingIndex)
{
    win32_replay_buffer *replayBuffer = win32GetReplayBuffer(state, inputRecordingIndex);
    if (replayBuffer->memoryBlock)
    {
        state->inputRecordingIndex = inputRecordingIndex;
        char fileName[WIN32_STATE_FILE_NAME_COUNT];
        win32GetInputFileLocation(state, true, inputRecordingIndex, sizeof(fileName), fileName);
        state->recordingHandle = CreateFileA(fileName, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
#if 0
        LARGE_INTEGER filePosition;
        filePosition.QuadPart = state->totalSize;
        SetFilePointerEx(state->recordingHandle, filePosition, 0, FILE_BEGIN);
#endif
        CopyMemory(replayBuffer->memoryBlock, state->gameMemoryBlock, state->totalSize);
    }
}

internal void
win32EndRecordingInput(win32_state *state)
{
    CloseHandle(state->recordingHandle);
    state->inputRecordingIndex = 0;
}

internal void
win32BeginPlayBackInput(win32_state *state, int inputPlayingIndex)
{
    win32_replay_buffer *replayBuffer = win32GetReplayBuffer(state, inputPlayingIndex);
    if (replayBuffer->memoryBlock)
    {
        state->inputPlayingIndex = inputPlayingIndex;
        char fileName[WIN32_STATE_FILE_NAME_COUNT];
        win32GetInputFileLocation(state, true, inputPlayingIndex, sizeof(fileName), fileName);
        state->playBackHandle = CreateFileA(fileName, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
#if 0
        LARGE_INTEGER filePosition;
        filePosition.QuadPart = state->totalSize;
        SetFilePointerEx(state->playBackHandle, filePosition, 0, FILE_BEGIN);
#endif
        CopyMemory(state->gameMemoryBlock, replayBuffer->memoryBlock, state->totalSize);
    }
}

internal void
win32EndPlayBackInput(win32_state *state)
{
    CloseHandle(state->playBackHandle);
    state->inputPlayingIndex = 0;
}

internal void
win32RecordInput(win32_state *state, game_input *newInput)
{
    DWORD bytesWritten;
    WriteFile(state->recordingHandle, newInput, sizeof(*newInput), &bytesWritten, 0);
}

internal void
win32PlaybackInput(win32_state *state, game_input *newInput)
{
    DWORD bytesRead = 0;
    if (ReadFile(state->playBackHandle, newInput, sizeof(*newInput), &bytesRead, 0))
    {
        if (bytesRead == 0)
        {
            int playingIndex = state->inputPlayingIndex;
            win32EndPlayBackInput(state);
            win32BeginPlayBackInput(state, playingIndex);
            ReadFile(state->playBackHandle, newInput, sizeof(*newInput), &bytesRead, 0);
        }
    }
}

internal void
win32ProcessPendingMessage(win32_state *state, game_controller_input *keyboardController)
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
                if (vkCode == 'L')
                {
                    if (isDown)
                    {
                        if (state->inputPlayingIndex == 0)
                        {
                            if (state->inputRecordingIndex == 0)
                            {
                                win32BeginRecordingInput(state, 1);
                            }
                            else
                            {
                                win32EndRecordingInput(state);
                                win32BeginPlayBackInput(state, 1);
                            }
                        }
                        else
                        {
                            win32EndPlayBackInput(state);
                        }
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

#if 0
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
#endif

internal void
win32GetExeFileName(win32_state *state)
{

    DWORD sizeofFileName = GetModuleFileNameA(0, state->exeFileName, sizeof(state->exeFileName));
    state->onePastLastExeFileNameSlash = state->exeFileName + sizeofFileName;
    for (char *scan = state->exeFileName; *scan; ++scan)
    {
        if (*scan == '\\')
        {
            state->onePastLastExeFileNameSlash = scan + 1;
        }
    }
}

int CALLBACK
WinMain(HINSTANCE instance, HINSTANCE prevInstance,
        PSTR commandLine, int showindowClassode)
{
    win32_state win32State = {};

    LARGE_INTEGER perfCountFrequencyResult;
    QueryPerformanceFrequency(&perfCountFrequencyResult);
    globalPerfCountFrequency = perfCountFrequencyResult.QuadPart;

    win32GetExeFileName(&win32State);
    char sourceGameCodeDllFullPath[WIN32_STATE_FILE_NAME_COUNT];
    win32BuildExePathFileName(&win32State, "handmade.dll", sizeof(sourceGameCodeDllFullPath), sourceGameCodeDllFullPath);
    char tempGameCodeDllFullPath[WIN32_STATE_FILE_NAME_COUNT];
    win32BuildExePathFileName(&win32State, "handmade_temp.dll", sizeof(tempGameCodeDllFullPath), tempGameCodeDllFullPath);

    UINT desiredSchedularMS = 1;
    bool32 sleepIsGranular = (timeBeginPeriod(desiredSchedularMS) == TIMERR_NOERROR);

    win32LoadXInput();

    win32ResizeDIBSection(&globalBackBuffer, 1280, 720);
    WNDCLASSA windowClass = {};

    windowClass.lpfnWndProc = win32MainWindowindowCallback;
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.hInstance = instance;
    windowClass.lpszClassName = "handmadeHeroWindowClass";

    if (RegisterClassA(&windowClass))
    {
        HWND window = CreateWindowExA(
            0, // WS_EX_TOPMOST | WS_EX_LAYERED
            windowClass.lpszClassName,
            "Handmade hero",
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            0, 0, instance,
            0);

        if (window)
        {
            win32_sound_output soundOutput = {};

            int monitorRefreshHertz = 60;
            HDC refreshDC = GetDC(window);
            int win32RefreshRate = GetDeviceCaps(refreshDC, VREFRESH);
            ReleaseDC(window, refreshDC);
            if (win32RefreshRate > 1)
            {
                monitorRefreshHertz = win32RefreshRate;
            }
            real32 gameUpdateHz = (monitorRefreshHertz / 4.0f);
            real32 targetSecondsPerFrame = 1.0f / (real32)gameUpdateHz;

            soundOutput.samplesPerSeconds = 48000;
            soundOutput.runningSampleIndex = 0;
            soundOutput.bytesPerSample = sizeof(int16) * 2;
            soundOutput.secondaryBufferSize = soundOutput.samplesPerSeconds * soundOutput.bytesPerSample;
            soundOutput.safetyBytes = (int)(((real32)soundOutput.samplesPerSeconds * (real32)soundOutput.bytesPerSample / gameUpdateHz) / 3.0f);

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

            win32State.totalSize = gameMemory.permanentStorageSize + gameMemory.transientStorageSize;
            win32State.gameMemoryBlock = VirtualAlloc(baseAddress, (size_t)win32State.totalSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
            gameMemory.permanentStorage = win32State.gameMemoryBlock;
            gameMemory.transientStorage = ((uint8 *)gameMemory.permanentStorage + gameMemory.permanentStorageSize);

            for (int replayIndex = 0; replayIndex < ARRAY_COUNT(win32State.replayBuffers); replayIndex++)
            {
                win32_replay_buffer *replayBuffer = &win32State.replayBuffers[replayIndex];

                win32GetInputFileLocation(&win32State, false, replayIndex, sizeof(replayBuffer->fileName), replayBuffer->fileName);

                replayBuffer->fileHandle = CreateFileA(replayBuffer->fileName, GENERIC_READ | GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);

                LARGE_INTEGER maxSize;
                maxSize.QuadPart = win32State.totalSize;
                replayBuffer->memoryMap = CreateFileMapping(replayBuffer->fileHandle, 0, PAGE_READWRITE, maxSize.HighPart, maxSize.LowPart, 0);
                DWORD error = GetLastError();

                replayBuffer->memoryBlock = MapViewOfFile(replayBuffer->memoryMap, FILE_MAP_ALL_ACCESS, 0, 0, win32State.totalSize);
                if (replayBuffer->memoryBlock)
                {
                }
                else
                {
                    // TODO: log
                }
            }

            if (samples && gameMemory.permanentStorage && gameMemory.transientStorage)
            {
                game_input input[2] = {};
                game_input *newInput = &input[0];
                game_input *oldInput = &input[1];

                LARGE_INTEGER lastCounter = win32GetWallClock();
                LARGE_INTEGER flipWallClock = win32GetWallClock();

                int debugTimeMarkerIndex = 0;
                win32_debug_time_marker debugTimeMarkers[30] = {0};

                DWORD audioLatencyBytes = 0;
                real32 audioLatencySeconds = 0;
                bool32 soundIsValid = false;

                uint64 lastCycleCount = __rdtsc();

                win32_game_code game = win32LoadGameCode(sourceGameCodeDllFullPath, tempGameCodeDllFullPath);

                while (globalRunning)
                {
                    FILETIME newDllWriteTime = win32GetLastWriteTime(sourceGameCodeDllFullPath);
                    if (CompareFileTime(&newDllWriteTime, &game.DLLLastWriteTime) != 0)
                    {
                        win32UnloadGameCode(&game);
                        game = win32LoadGameCode(sourceGameCodeDllFullPath, tempGameCodeDllFullPath);
                    }
                    game_controller_input *oldKeyboardController = getController(oldInput, 0);
                    game_controller_input *newKeyboardController = getController(newInput, 0);
                    *newKeyboardController = {};

                    newKeyboardController->isConnected = true;

                    for (int buttonIndex = 0; buttonIndex < ARRAY_COUNT(newKeyboardController->buttons); buttonIndex++)
                    {
                        newKeyboardController->buttons[buttonIndex].endedDown = oldKeyboardController->buttons[buttonIndex].endedDown;
                    }

                    win32ProcessPendingMessage(&win32State, newKeyboardController);

                    if (!globalPause)
                    {
                        POINT mouseP;
                        GetCursorPos(&mouseP);
                        ScreenToClient(window, &mouseP);
                        newInput->mouseX = mouseP.x;
                        newInput->mouseY = mouseP.y;
                        newInput->mouseZ = 0; // TODO: MIDDLE MOUSE WHEEL SUPPORT
                        win32ProcessKeyboardMessage(&newInput->mouseButtons[0], GetKeyState(VK_LBUTTON) & (1 << 15));
                        win32ProcessKeyboardMessage(&newInput->mouseButtons[1], GetKeyState(VK_MBUTTON) & (1 << 15));
                        win32ProcessKeyboardMessage(&newInput->mouseButtons[2], GetKeyState(VK_RBUTTON) & (1 << 15));
                        win32ProcessKeyboardMessage(&newInput->mouseButtons[3], GetKeyState(VK_XBUTTON1) & (1 << 15));
                        win32ProcessKeyboardMessage(&newInput->mouseButtons[4], GetKeyState(VK_XBUTTON2) & (1 << 15));

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
                                newController->isAnalog = oldController->isAnalog;

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

                        thread_context thread = {};

                        game_offscreen_buffer buffer;
                        buffer.height = globalBackBuffer.height;
                        buffer.width = globalBackBuffer.width;
                        buffer.memory = globalBackBuffer.memory;
                        buffer.pitch = globalBackBuffer.pitch;
                        buffer.bytesPerPixel = globalBackBuffer.bytesPerPixel;

                        if (win32State.inputRecordingIndex)
                        {
                            win32RecordInput(&win32State, newInput);
                        }
                        if (win32State.inputPlayingIndex)
                        {
                            win32PlaybackInput(&win32State, newInput);
                        }

                        if (game.UpdateAndRender)
                        {
                            game.UpdateAndRender(&thread, &gameMemory, newInput, &buffer);
                        }

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

                            DWORD expectedSoundBytesPerFrame = (DWORD)((real32)(soundOutput.samplesPerSeconds * soundOutput.bytesPerSample) / gameUpdateHz);
                            real32 secondsLeftUntillFlip = targetSecondsPerFrame - fromBeginToAudioSeconds;
                            DWORD expectedBytesUntillFlip = (DWORD)((secondsLeftUntillFlip / targetSecondsPerFrame) * (real32)expectedSoundBytesPerFrame);

                            DWORD expectedFrameBoundaryByte = playCursor + expectedBytesUntillFlip;
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

                            if (game.GetSoundSamples)
                            {
                                game.GetSoundSamples(&thread, &gameMemory, &soundBuffer);
                            }
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
#if 0
                            char textBuffer[256];
                            _snprintf_s(textBuffer, sizeof(textBuffer),
                                        "BTL: %u,    TC: %u,       BTW: %u,   -  PC: %u,     WC: %u,      DELTA: %u,           (%fs)\n",
                                        byteToLock, targetCursor, bytesToWrite, playCursor, writeCursor, audioLatencyBytes, audioLatencySeconds);
                            OutputDebugStringA(textBuffer);
#endif
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
                                // *****
                                // DWORD sleepMs = (DWORD)(1000.0f * (targetSecondsPerFrame - secondsElapsedForFrame));
                                // *****

                                // *****
                                DWORD sleepMs1tenth = (DWORD)((1000.0f * (targetSecondsPerFrame - secondsElapsedForFrame)) / 10.0f);
                                DWORD sleepMs = sleepMs1tenth * 8;
                                // *****

                                if (sleepMs > 0)
                                {
                                    Sleep(sleepMs);
                                }
                            }

                            // *****
                            // real32 testSecondsElapsedForFrame = win32GetSecondsElapsed(lastCounter, win32GetWallClock());
                            // if (testSecondsElapsedForFrame < targetSecondsPerFrame)
                            // {
                            // }
                            // *****

                            // *****
                            real32 secondsElapsedForFrame = win32GetSecondsElapsed(lastCounter, win32GetWallClock());
                            // *****

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
                        HDC deviceContext = GetDC(window);
                        win32DisplayBufferInWindow(&globalBackBuffer, deviceContext, dimensions.width, dimensions.height);
                        ReleaseDC(window, deviceContext);

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
#if 0
                        char FPSbuffer[256];
                        _snprintf_s(FPSbuffer, sizeof(FPSbuffer), "ms/f: %.2f, fps: %.2f, mc/f: %.2f\n", msPerFrame, fps, mcpf);
                        OutputDebugStringA(FPSbuffer);
#endif

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
