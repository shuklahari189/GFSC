#include <windows.h>
#include <cstdint>

// TODO: defines for standard types

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
renderWeirdGradient(win32OffscreenBuffer buffer, int xOffset, int yOffset)
{
    unsigned char *row = (unsigned char *)buffer.memory;
    for (int y = 0; y < buffer.height; y++)
    {
        unsigned int *pixels = (unsigned int *)row;
        for (int x = 0; x < buffer.width; x++)
        {
            unsigned char r = (unsigned char)x + xOffset;
            unsigned char g = (unsigned char)y + yOffset;
            unsigned char b = 100;
            // Big endian, therefore
            *pixels++ = (r << 16) | (g << 8) | b;
        }
        row += buffer.pitch;
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
    buffer->memory = VirtualAlloc(0, bitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
    buffer->pitch = buffer->width * bytesPerPixels;
}

Internal void
win32DisplayBufferInWindow(win32OffscreenBuffer buffer, HDC deviceContext,
                           int windowWidth, int windowHeight)
{
    StretchDIBits(
        deviceContext,
        0, 0, windowWidth, windowHeight,
        0, 0, buffer.width, buffer.height,
        buffer.memory,
        &buffer.info,
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
    case WM_PAINT:
    {
        PAINTSTRUCT paint;
        HDC deviceContext = BeginPaint(window, &paint);
        int x = paint.rcPaint.left;
        int y = paint.rcPaint.top;
        int width = paint.rcPaint.right - paint.rcPaint.left;
        int height = paint.rcPaint.bottom - paint.rcPaint.top;
        win32WindowDimension dimensions = win32getWindowDimensions(window);
        win32DisplayBufferInWindow(globalBackBuffer, deviceContext, dimensions.width, dimensions.height);
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

int WINAPI
WinMain(HINSTANCE instance, HINSTANCE prevInstance, PSTR commandLine, int showCode)
{
    WNDCLASSA windowClass = {};
    win32ResizeDIBsection(&globalBackBuffer, 1280, 720);

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
        HDC deviceContext = GetDC(window);

        globalRunning = true;
        int xOffset = 0;
        int yOffset = 0;

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

            renderWeirdGradient(globalBackBuffer, xOffset++, yOffset);
            win32WindowDimension dimensions = win32getWindowDimensions(window);
            win32DisplayBufferInWindow(globalBackBuffer, deviceContext, dimensions.width, dimensions.height);
        }
    }
    else
    {
        // TODO: logging system ?
    }

    return 0;
}