#include <windows.h>
#include <dsound.h>

#include <math.h>
#define Pi32 3.14159265359f

// standard type defines
#include <stdint.h>
typedef int8_t int8;
typedef uint8_t uint8;
typedef int16_t int16;
typedef uint16_t uint16;
typedef int32_t int32;
typedef uint32_t uint32;
typedef int bool32;
typedef float real32;
typedef double real64;

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

static LPDIRECTSOUNDBUFFER globalSecondaryBuffer;

#define CREATE_DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter);
typedef CREATE_DIRECT_SOUND_CREATE(direct_sound_create);

static void initDSound(HWND window, int32 samplesPerSecond, int32 bufferSize)
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

static void fillSoundBuffer(SoundOutput *soundOutput, DWORD byteToLock, DWORD bytesToWrite)
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

static bool32 gameIsRunning;
LRESULT CALLBACK windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CLOSE:
    {
        gameIsRunning = false;
    }
        return 0;
    }
    return DefWindowProcA(window, message, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE preveInstance, PSTR arguments, int showCode)
{
    WNDCLASSA windowClass = {};
    windowClass.hInstance = instance;
    windowClass.lpfnWndProc = windowCallback;
    windowClass.lpszClassName = "className";
    windowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    RegisterClassA(&windowClass);
    HWND window = CreateWindowExA(0, windowClass.lpszClassName, "windowName",
                                  WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                  CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                  0, 0, instance, 0);

    if (window)
    {
        SoundOutput soundOutput = {};
        soundOutput.samplesPerSecond = 48000;                                                        // 48 khz = 48000 samples per second
        soundOutput.toneHz = 256;                                                                    // no of samples between (Two peeks) theoratical
        soundOutput.toneVolume = 3000;                                                               // max value [lower than (2 ^ 16 - 1) =  65535]
        soundOutput.runningSampleIndex = 0;                                                          // incremented every [left right] = 1 sample filled
        soundOutput.wavePeriod = soundOutput.samplesPerSecond / soundOutput.toneHz;                  // 48000 / 256 = 187 no of samples between (Two peeks) in actual buffer
        soundOutput.bytesPerSample = sizeof(int16) * 2;                                              // [left(2byte) right(2byte)] = 4bytes
        soundOutput.secondaryBufferSize = soundOutput.samplesPerSecond * soundOutput.bytesPerSample; // 48000 * 4bytes
        soundOutput.latencySampleCount = soundOutput.samplesPerSecond / 15;

        initDSound(window, soundOutput.samplesPerSecond, soundOutput.secondaryBufferSize);

        fillSoundBuffer(&soundOutput, 0, (soundOutput.latencySampleCount * soundOutput.bytesPerSample));

        globalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

        gameIsRunning = true;
        while (gameIsRunning)
        {
            MSG message;
            while (PeekMessageA(&message, 0, 0, 0, PM_REMOVE))
            {
                if (message.message == WM_QUIT)
                {
                    gameIsRunning = false;
                }
                TranslateMessage(&message);
                DispatchMessageA(&message);
            }

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
                fillSoundBuffer(&soundOutput, byteToLock, bytesToWrite);
            }
        }
    }
}