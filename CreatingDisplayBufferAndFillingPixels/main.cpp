#include <windows.h>
#include <stdint.h>

typedef uint64_t uint64;
typedef uint32_t uint32;
typedef uint16_t uint16;
typedef uint8_t uint8;

typedef int64_t int64;
typedef int32_t int32;
typedef int16_t int16;
typedef int8_t int8;

struct DisplayBuffer
{
    uint16 width;
    uint16 height;
    BITMAPINFO info;
    void *memory;
    uint32 pitch;
};

struct Dimensions
{
    uint16 width;
    uint16 height;
};

static DisplayBuffer displayBuffer;
static bool gameIsRunning;

static void createDisplayBuffer(DisplayBuffer *buffer, int width, int height)
{
    if (buffer->memory)
    {
        VirtualFree(buffer->memory, 0, MEM_RELEASE);
    }

    buffer->width = width;
    buffer->height = height;
    buffer->info.bmiHeader.biSize = sizeof(buffer->info.bmiHeader);
    buffer->info.bmiHeader.biWidth = width;
    // Negative height for top down bitmap
    buffer->info.bmiHeader.biHeight = -height;
    buffer->info.bmiHeader.biPlanes = 1;
    buffer->info.bmiHeader.biBitCount = 32;
    buffer->info.bmiHeader.biCompression = BI_RGB;
    uint8 bytesPerPixels = 4;
    uint32 bitmapMemroySize = width * height * bytesPerPixels;
    buffer->memory = VirtualAlloc(0, bitmapMemroySize, MEM_COMMIT, PAGE_READWRITE);
    buffer->pitch = width * bytesPerPixels;
}

static void fillDisplayBuffer(DisplayBuffer *buffer)
{
    uint8 *row = (uint8 *)buffer->memory;
    for (int y = 0; y < buffer->height; y++)
    {
        uint32 *pixel = (uint32 *)row;
        for (int x = 0; x < buffer->width; x++)
        {
            uint8 r = 255;
            uint8 g = 0;
            uint8 b = 0;
            *pixel++ = (r << 16) | (g << 8) | b;
        }
        row += buffer->pitch;
    }
}

static void displayBufferOnWindow(HDC deviceContext, DisplayBuffer *buffer, int windowWidth, int windowHeight)
{
    StretchDIBits(deviceContext,
                  0, 0, windowWidth, windowHeight,
                  0, 0, buffer->width, buffer->height,
                  buffer->memory, &buffer->info, DIB_RGB_COLORS, SRCCOPY);
}

static Dimensions getWindowDimensions(HWND window)
{
    RECT clientrect;
    GetClientRect(window, &clientrect);
    Dimensions dimension;
    dimension.height = clientrect.bottom;
    dimension.width = clientrect.right;
    return dimension;
}

LRESULT CALLBACK windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CLOSE:
    {
        gameIsRunning = false;
    }
        return 0;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC deviceContext = BeginPaint(window, &ps);
        fillDisplayBuffer(&displayBuffer);
        Dimensions windowDimensions = getWindowDimensions(window);
        displayBufferOnWindow(deviceContext, &displayBuffer, windowDimensions.width, windowDimensions.height);
        EndPaint(window, &ps);
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

    createDisplayBuffer(&displayBuffer, 1920, 1080);
    if (window)
    {
        gameIsRunning = true;
        HDC deviceContext = GetDC(window);
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

            fillDisplayBuffer(&displayBuffer);
            Dimensions windowDimensions = getWindowDimensions(window);
            displayBufferOnWindow(deviceContext, &displayBuffer, windowDimensions.width, windowDimensions.height);
        }
        ReleaseDC(window, deviceContext);
    }
    else
    {
        // failed to create window
    }
}