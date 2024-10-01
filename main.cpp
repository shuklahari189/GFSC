#include <windows.h>
#define LocalPersist static
#define GlobalVariable static
#define Internal static

// TODO: global for now
GlobalVariable bool running;

LRESULT Wndproc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
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

int WINAPI WinMain(HINSTANCE instance, HINSTANCE prevInstance, PSTR commandLine, int showCode)
{
    WNDCLASSA windowClass = {};
    windowClass.style = CS_VREDRAW | CS_HREDRAW | CS_OWNDC;
    windowClass.lpfnWndProc = Wndproc;
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