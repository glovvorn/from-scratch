#include <windows.h>

#define internal static
#define local_persist static
#define global_variable static

//TODO(greg): this is global for now
global_variable bool Running;

LRESULT CALLBACK MainWindowCallback(HWND Window,
                                    UINT Message,
                                    WPARAM WParam,
                                    LPARAM LParam)
{
    LRESULT Result = 0;
  
    switch (Message)
    {
        case WM_SIZE:
        {

        } break;
        case WM_DESTROY:
        {
            //TODO(greg): recreate window?
            Running = false;
        } break;
        case WM_CLOSE:
        {
            //TODO(greg): Handle this with a confirmation message
            Running = false;
        } break;
        case WM_ACTIVATEAPP:
        {

        } break;
        case WM_PAINT:
        {
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(Window, &Paint);
            int x = Paint.rcPaint.left;
            int y = Paint.rcPaint.top;
            int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;
            int Width = Paint.rcPaint.right - Paint.rcPaint.left;
            local_persist DWORD Operation = WHITENESS;
            PatBlt(DeviceContext, x, y, Width, Height, Operation);
            if (Operation == WHITENESS)
            {
                Operation = BLACKNESS;
            }
            else
            {
                Operation = WHITENESS;
            }
            EndPaint(Window, &Paint);
        } break;
        default:
        {
            Result = DefWindowProc(Window, Message, WParam, LParam);
        } break;
    }

    return Result;
}

int CALLBACK WinMain(
    HINSTANCE Instance,
    HINSTANCE PrevInstance,
    LPSTR CmdLine,
    int CmdShow)
{
    WNDCLASS WindowClass = {};

    WindowClass.lpfnWndProc = MainWindowCallback;
    WindowClass.hInstance = Instance;
    //WindowClass.hIcon = ;
    WindowClass.lpszClassName = "HandmadeHeroWindowClass";

    if (RegisterClass(&WindowClass))
    {
        HWND WindowHandle = 
            CreateWindowEx(
                0,
                WindowClass.lpszClassName,
                "Handmade Hero",
                WS_OVERLAPPEDWINDOW|WS_VISIBLE,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                0,
                0,
                Instance,
                0);

        if (WindowHandle)
        {
            Running = true;
            while(Running)
            {
                MSG Message;
                BOOL MessageResult = GetMessage(&Message, 0, 0, 0);
                if (MessageResult > 0)
                {
                    TranslateMessage(&Message);
                    DispatchMessage(&Message);
                }
                else
                {
                    break;
                }
            }
        }
        else
        {
            //TODO(greg): Logging
        }
    }
    else
    {
        //TODO(greg): Logging
    }

    return (0);
}
