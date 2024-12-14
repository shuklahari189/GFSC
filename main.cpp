#include <windows.h>
#include <stdint.h>

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
renderWeirdGradient(win32OffScreenBuffer buffer, int xOffset, int yOffset)
{
    uint8 *row = (uint8 *)buffer.memory;
    for (int y = 0;
         y < buffer.height;
         y++)
    {
        uint32 *pixel = (uint32 *)row;
        for (int x = 0;
             x < buffer.width;
             x++)
        {
            //            red              green                      blue
            *pixel++ = (0) << 16 | (uint8(x + xOffset) << 8) | uint8(y + yOffset);
        }
        row += buffer.pitch;
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
    buffer->memory = VirtualAlloc(0, bitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
    buffer->pitch = width * bytesPerPixel;
}

internal void
win32DisplayBufferInWindow(HDC deviceContext,
                           int windowWidth, int windowHeight,
                           win32OffScreenBuffer buffer)
{
    StretchDIBits(deviceContext,
                  0, 0, windowWidth, windowHeight,
                  0, 0, buffer.width, buffer.height,
                  buffer.memory, &buffer.info,
                  DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK win32MainWindowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
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
    case WM_PAINT:
    {
        PAINTSTRUCT paint;
        HDC deviceContext = BeginPaint(window, &paint);
        win32WindowDimensions dimension = win32GetWindowDimension(window);
        win32DisplayBufferInWindow(deviceContext, dimension.width, dimension.height,
                                   globalBackBuffer);
        EndPaint(window, &paint);
    }
        return 0;
    }
    return DefWindowProcA(window, message, wParam, lParam);
}

int CALLBACK WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR commandLine, int showCode)
{
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
                renderWeirdGradient(globalBackBuffer, xOffset, yOffset);

                win32WindowDimensions dimension = win32GetWindowDimension(window);
                win32DisplayBufferInWindow(deviceContext, dimension.width, dimension.height, globalBackBuffer);
                ++xOffset;
            }
        }
    }

    return 0;
}