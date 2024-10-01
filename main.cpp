#include <windows.h>

#define LocalPersist static
#define GlobalVariable static
#define Internal static

// TODO: global for now
GlobalVariable bool running;
GlobalVariable int bitmapWidth;
GlobalVariable int bitmapHeight;

GlobalVariable BITMAPINFO bitmapInfo;
GlobalVariable void *bitmapMemory;
GlobalVariable int bytesPerPixels = 4;

Internal void
renderWeirdGradient(int xOffset, int yOffset)
{
    unsigned char *row = (unsigned char *)bitmapMemory;
    int pitch = bitmapWidth * bytesPerPixels;
    for (int y = 0; y < bitmapHeight; y++)
    {
        // unsigned char *pixels = (unsigned char *)row;
        // for (int x = 0; x < bitmapWidth; x++)
        // {
        //     // Big endian, therefore
        //     *pixels = (unsigned char)x + xOffset; // B
        //     pixels++;
        //     *pixels = (unsigned char)y + yOffset; // G
        //     pixels++;
        //     *pixels = 100; // R
        //     pixels++;
        //     *pixels = 100; // Padding
        //     pixels++;
        // }
        unsigned int *pixels = (unsigned int *)row;
        for (int x = 0; x < bitmapWidth; x++)
        {
            unsigned char r = (unsigned char)x + xOffset;
            unsigned char g = (unsigned char)x + xOffset;
            unsigned char b = 100;
            // Big endian, therefore
            *pixels++ = (r << 16) | (g << 8) | b;
        }
        row += pitch;
    }
}

Internal void
win32ResizeDIBsection(int width, int height)
{
    if (bitmapMemory)
    {
        VirtualFree(bitmapMemory, 0, MEM_RELEASE);
    }
    bitmapWidth = width;
    bitmapHeight = height;
    bitmapInfo.bmiHeader.biSize = sizeof(bitmapInfo);
    bitmapInfo.bmiHeader.biWidth = bitmapWidth;
    bitmapInfo.bmiHeader.biHeight = -bitmapHeight;
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 32; // acutally need 24 buit for dword alignment 32
    bitmapInfo.bmiHeader.biCompression = BI_RGB;

    int bitmapMemorySize = bitmapWidth * bitmapHeight * bytesPerPixels;
    bitmapMemory = VirtualAlloc(0, bitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

    renderWeirdGradient(0, 0);
}

Internal void
win32UpdateWindow(HDC deviceContext, RECT *clientRect, int x, int y, int width, int height)
{
    int windowWidth = clientRect->right - clientRect->left;
    int windowHeight = clientRect->bottom - clientRect->top;

    StretchDIBits(
        deviceContext,
        0, 0, bitmapWidth, bitmapHeight,
        0, 0, windowWidth, windowHeight, bitmapMemory,
        &bitmapInfo,
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
        RECT clientRect;
        GetClientRect(window, &clientRect);
        int width = clientRect.right - clientRect.left;
        int height = clientRect.bottom - clientRect.top;
        win32ResizeDIBsection(width, height);
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
        running = false;
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
        RECT clientRect;
        GetClientRect(window, &clientRect);
        win32UpdateWindow(deviceContext, &clientRect, x, y, width, height);
        EndPaint(window, &paint);
    }
    break;
    case WM_DESTROY:
    {

        // TODO: handle this with an error,then recreate window ?
        running = false;
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
    windowClass.style = CS_VREDRAW | CS_HREDRAW | CS_OWNDC;
    windowClass.lpfnWndProc = win32MainWindowCallback;
    windowClass.hInstance = instance;
    windowClass.lpszClassName = "HandmadeHeroClass";

    RegisterClassA(&windowClass);

    HWND window = CreateWindowExA(
        0,
        windowClass.lpszClassName,
        "HandmadeHeroWindow",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT,
        CW_USEDEFAULT, CW_USEDEFAULT,
        NULL,
        NULL,
        instance,
        NULL);

    if (window)
    {
        running = true;
        int xOffset = 0;
        int yOffset = 0;
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

            renderWeirdGradient(xOffset++, yOffset);
            HDC deviceContext = GetDC(window);
            RECT clientRect;
            GetClientRect(window, &clientRect);
            int windowWidth = clientRect.right - clientRect.left;
            int windowHeight = clientRect.bottom - clientRect.top;
            win32UpdateWindow(deviceContext, &clientRect, 0, 0, windowWidth, windowHeight);
            ReleaseDC(window, deviceContext);
        }
    }
    else
    {
        // TODO: logging system ?
    }

    return 0;
}