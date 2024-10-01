#include <windows.h>

#define LocalPersist static
#define GlobalVariable static
#define Internal static

// TODO: global for now
GlobalVariable bool running;

GlobalVariable BITMAPINFO bitmapInfo;
GlobalVariable HBITMAP bitmapHandle;
GlobalVariable void *bitmapMemory;
GlobalVariable HDC bitmapDeviceContext;

Internal void 
win32ResizeDIBsection(int width, int height)
{
    if (bitmapHandle)
    {
        DeleteObject(bitmapHandle);
    }

    if(!bitmapDeviceContext)
    {
        // TODO: should we recreate under certain special circumstances ?
        bitmapDeviceContext = CreateCompatibleDC(0);
    }

    bitmapInfo.bmiHeader.biSize = sizeof(bitmapInfo);
    bitmapInfo.bmiHeader.biWidth = width;
    bitmapInfo.bmiHeader.biHeight = height;
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 32; // acutally need 24 buit for dword alignment 32
    bitmapInfo.bmiHeader.biCompression = BI_RGB;

    bitmapHandle = CreateDIBSection(
        bitmapDeviceContext,
        &bitmapInfo,
        DIB_RGB_COLORS,
        &bitmapMemory,
        0, 0);
}

Internal void 
win32UpdateWindow(HDC deviceContext, int x, int y, int width, int height)
{
    StretchDIBits(
        deviceContext,
        x, y, width, height,
        x, y, width, height,
        bitmapMemory,
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
        win32UpdateWindow(deviceContext, x, y, width, height);
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

    HWND windowHandle = CreateWindowExA(
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

    if (windowHandle)
    {
        MSG message;
        running = true;
        while (running)
        {
            BOOL messageResult = GetMessageA(&message, 0, 0, 0);
            if (messageResult > 0)
            {
                TranslateMessage(&message);
                DispatchMessageA(&message);
            }
            else
            {
                break;
            }
        }
    }
    else
    {
        // TODO: logging system ?
    }

    return 0;
}