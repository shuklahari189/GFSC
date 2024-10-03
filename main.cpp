#include <windows.h>

static bool running;
static void *memory;
static BITMAPINFO info;
static int width;
static int height;

static void win32CreateDisplayBuffer(int bitmapWidth, int bitmapHeight)
{
    if (memory)
    {
        VirtualFree(memory, 0, MEM_RELEASE);
    }
    width = bitmapWidth;
    height = bitmapHeight;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biSize = sizeof(info.bmiHeader);
    info.bmiHeader.biWidth = bitmapWidth;
    info.bmiHeader.biHeight = -bitmapHeight;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biCompression = BI_RGB;

    int bytesPerPixel = 4;
    int memorySize = bitmapWidth * bitmapHeight * bytesPerPixel;
    memory = VirtualAlloc(0, memorySize, MEM_COMMIT, PAGE_READWRITE);
}

static void win32ShowDispayBuffer(HDC deviceContext, int windowWidth, int windowHeight)
{
    StretchDIBits(deviceContext,
                  0, 0, windowWidth, windowHeight,
                  0, 0, width, height,
                  memory, &info, DIB_RGB_COLORS, SRCCOPY);
}

static void win32FillDisplayBuffer()
{
    int pitch = width * 4;
    unsigned char *row = (unsigned char *)memory;
    for (int y = 0; y < height; y++)
    {
        unsigned int *pixel = (unsigned int *)row;
        for (int x = 0; x < width; x++)
        {
            unsigned char r = (unsigned char)x;
            unsigned char g = (unsigned char)y;
            unsigned char b = 0;
            *pixel = (r << 16) | (g << 8) | b;
            pixel++;
        }
        row += pitch;
    }
}

LRESULT CALLBACK win32MainWindowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CLOSE:
    {
        running = false;
        OutputDebugStringA("closing.\n");
    }
        return 0;
    case WM_PAINT:
    {
        PAINTSTRUCT paint;
        HDC deviceContext = BeginPaint(window, &paint);
        win32FillDisplayBuffer();
        win32ShowDispayBuffer(deviceContext, paint.rcPaint.right, paint.rcPaint.bottom);
        EndPaint(window, &paint);
    }
        return 0;
    }
    return DefWindowProcA(window, message, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE prevInstance, PSTR arguments, int showCode)
{

    WNDCLASSA windowClass = {};
    windowClass.hInstance = instance;
    windowClass.lpszClassName = "GameClass";
    windowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    windowClass.lpfnWndProc = win32MainWindowCallback;

    RegisterClassA(&windowClass);

    HWND window = CreateWindowExA(
        0,
        windowClass.lpszClassName,
        "Game",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        0, 0, instance, 0);

    if (window)
    {
        HDC deviceContext = GetDC(window);
        running = true;
        win32CreateDisplayBuffer(1080, 720);
        while (running)
        {
            MSG message;
            while (PeekMessageA(&message, 0, 0, 0, PM_REMOVE))
            {
                if (message.message == WM_QUIT)
                {
                    running = false;
                }
                TranslateMessage(&message);
                DispatchMessageA(&message);
            }
            RECT clientRect;
            GetClientRect(window, &clientRect);
            win32FillDisplayBuffer();
            win32ShowDispayBuffer(deviceContext, clientRect.right, clientRect.bottom);
        }
    }
    else
    {
        OutputDebugStringA("Error creating window.\n");
    }
}