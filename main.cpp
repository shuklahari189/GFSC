#include <windows.h>

LRESULT Wndproc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;
    switch (message)
    {
    case WM_CLOSE:
    {
        OutputDebugStringA("closed\n");
        PostQuitMessage(0);
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
        while (true)
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
        // TODO:
    }

    return 0;
}