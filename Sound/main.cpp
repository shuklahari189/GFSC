#include <windows.h>

static bool gameIsRunning;

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
        }
    }
    else
    {
        // failed to create window
    }
}