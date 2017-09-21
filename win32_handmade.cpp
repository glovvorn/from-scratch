#include <windows.h>
#include <stdint.h>
#include <xinput.h>
#include <dsound.h>

#define internal static
#define local_persist static
#define global_variable static

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int32 bool32;

struct win32_offscreen_buffer
{
    BITMAPINFO Info;
    void *Memory;
    int Width;
    int Height;
    int Pitch;
};

struct win32_window_dimensions
{
    int Width;
    int Height;
};

//TODO(greg): this is global for now
global_variable bool Running;
global_variable win32_offscreen_buffer GlobalBackBuffer;
global_variable LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;

//NOTE(greg): XInputGetState
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
    return (ERROR_DEVICE_NOT_CONNECTED);
}
global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputSetState XInputSetState_

//NOTE(greg): XInputSetState
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
    return (ERROR_DEVICE_NOT_CONNECTED);
}
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputGetState XInputGetState_

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);

internal void Win32LoadXInput(void)
{
    //TODO(greg): test this on Win versions
    HMODULE XInputLibrary = LoadLibraryA("xinput1_4.dll");
    if (!XInputLibrary)
    {
        HMODULE XInputLibrary = LoadLibraryA("xinput1_3.dll");
    }
    else
    {
        // TODO(greg): logging
    }

    if (XInputLibrary)
    {
        XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
        XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
    }
    else
    {
        // TODO(greg): logging
    }
}

internal void Win32InitDSound(HWND Window, int32 SamplesPerSecond, int32 BufferSize)
{
    HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");

    if (DSoundLibrary)
    {
        direct_sound_create *DirectSoundCreate = (direct_sound_create *)GetProcAddress(DSoundLibrary, "DirectSoundCreate");

        //TODO(greg): Check if we need DirectSound8 or 7
        LPDIRECTSOUND DirectSound;
        if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0)))
        {
            WAVEFORMATEX WaveFormat = {};
            WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
            WaveFormat.nChannels = 2;
            WaveFormat.wBitsPerSample = 16;
            WaveFormat.nBlockAlign = (WaveFormat.nChannels * WaveFormat.wBitsPerSample) / 8;
            WaveFormat.nSamplesPerSec = SamplesPerSecond;
            WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec * WaveFormat.nBlockAlign;
            WaveFormat.cbSize = 0;

            if (SUCCEEDED(DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY)))
            {
                DSBUFFERDESC BufferDescription = {};
                BufferDescription.dwSize = sizeof(BufferDescription);
                BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

                LPDIRECTSOUNDBUFFER PrimaryBuffer;
                if (SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &PrimaryBuffer, 0)))
                {
                    if (SUCCEEDED(PrimaryBuffer->SetFormat(&WaveFormat)))
                    {
                        //NOTE(greg):
                    }
                    else
                    {
                        //TODO(greg): Diagnostic
                    }
                }
            }
            else
            {
                //TODO(greg): logging
            }

            DSBUFFERDESC BufferDescription = {};
            BufferDescription.dwSize = sizeof(BufferDescription);
            BufferDescription.dwFlags = 0;
            BufferDescription.dwBufferBytes = BufferSize;
            BufferDescription.lpwfxFormat = &WaveFormat;

            if (SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &GlobalSecondaryBuffer, 0)))
            {
            }
        }
        else
        {
            // TODO(greg): logging
        }
    }
    else
    {
        // TODO(greg): logging
    }
}

internal win32_window_dimensions Win32GetWindowDimensions(HWND Window)
{
    win32_window_dimensions Result;

    RECT ClientRect;
    GetClientRect(Window, &ClientRect);
    Result.Height = ClientRect.bottom - ClientRect.top;
    Result.Width = ClientRect.right - ClientRect.left;

    return Result;
}

internal void RenderWeirdGradient(win32_offscreen_buffer *Buffer, int XOffset, int YOffset)
{
    uint8 *Row = (uint8 *)Buffer->Memory;
    for (int Y = 0; Y < Buffer->Height; Y++)
    {
        uint32 *Pixel = (uint32 *)Row;
        for (int X = 0; X < Buffer->Width; X++)
        {
            uint8 Blue = (X + XOffset);
            uint8 Green = (Y + YOffset);
            uint8 Red = 0;

            *Pixel = (Red << 16) | ((Green << 8) | Blue);
            *Pixel++;
        }

        Row += Buffer->Pitch;
    }
}

internal void Win32ResizeDIBSection(win32_offscreen_buffer *Buffer, int Width, int Height)
{
    // TODO(greg): bulletproof this
    // Maybe don't free first, free after, then free first if that fails

    if (Buffer->Memory)
    {
        VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
    }

    int BytesPerPixel = 4;

    Buffer->Width = Width;
    Buffer->Height = Height;

    Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
    Buffer->Info.bmiHeader.biWidth = Buffer->Width;
    Buffer->Info.bmiHeader.biHeight = -Buffer->Height;
    Buffer->Info.bmiHeader.biPlanes = 1;
    Buffer->Info.bmiHeader.biBitCount = 32;
    Buffer->Info.bmiHeader.biCompression = BI_RGB;

    int BitmapMemorySize = (Buffer->Width * Buffer->Height) * BytesPerPixel;
    Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

    Buffer->Pitch = Width * BytesPerPixel;

    // TODO(greg): Clear this to black
}

internal void Win32DisplayBufferInWindow(win32_offscreen_buffer *Buffer, HDC DeviceContext, int WindowWidth, int WindowHeight)
{
    //TODO(greg): Aspect ratio correction
    StretchDIBits(DeviceContext,
                  0, 0, WindowWidth, WindowHeight,
                  0, 0, Buffer->Width, Buffer->Height,
                  Buffer->Memory,
                  &Buffer->Info,
                  DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK Win32MainWindowCallback(HWND Window,
                                         UINT Message,
                                         WPARAM WParam,
                                         LPARAM LParam)
{
    LRESULT Result = 0;

    switch (Message)
    {
    case WM_DESTROY:
    {
        //TODO(greg): recreate window?
        Running = false;
    }
    break;
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_KEYDOWN:
    case WM_KEYUP:
    {
        uint32 VKCode = WParam;
        bool WasDown = ((LParam & (1 << 30)) != 0);
        bool IsDown = ((LParam & (1 << 31)) == 0);
        if (WasDown != IsDown)
        {
            if (VKCode == 'W')
            {
            }
            else if (VKCode == 'A')
            {
            }
            else if (VKCode == 'S')
            {
            }
            else if (VKCode == 'D')
            {
            }
            else if (VKCode == 'Q')
            {
            }
            else if (VKCode == 'E')
            {
            }
            else if (VKCode == VK_UP)
            {
            }
            else if (VKCode == VK_LEFT)
            {
            }
            else if (VKCode == VK_DOWN)
            {
            }
            else if (VKCode == VK_RIGHT)
            {
            }
            else if (VKCode == VK_ESCAPE)
            {
            }
            else if (VKCode == VK_SPACE)
            {
            }
        }

        bool32 AltKeyWasDown = (LParam & (1 << 29));
        if ((VKCode == VK_F4) && AltKeyWasDown)
        {
            Running = false;
        }
    }
    break;
    case WM_CLOSE:
    {
        //TODO(greg): Handle this with a confirmation message
        Running = false;
    }
    break;
    case WM_ACTIVATEAPP:
    {
    }
    break;
    case WM_PAINT:
    {
        PAINTSTRUCT Paint;
        HDC DeviceContext = BeginPaint(Window, &Paint);

        win32_window_dimensions Dimensions = Win32GetWindowDimensions(Window);

        Win32DisplayBufferInWindow(&GlobalBackBuffer, DeviceContext, Dimensions.Width, Dimensions.Height);
        EndPaint(Window, &Paint);
    }
    break;
    default:
    {
        Result = DefWindowProc(Window, Message, WParam, LParam);
    }
    break;
    }

    return Result;
}

int CALLBACK WinMain(
    HINSTANCE Instance,
    HINSTANCE PrevInstance,
    LPSTR CmdLine,
    int CmdShow)
{
    Win32LoadXInput();

    WNDCLASSA WindowClass = {};

    Win32ResizeDIBSection(&GlobalBackBuffer, 1280, 720);

    WindowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    WindowClass.lpfnWndProc = Win32MainWindowCallback;
    WindowClass.hInstance = Instance;
    //WindowClass.hIcon = ;
    WindowClass.lpszClassName = "HandmadeHeroWindowClass";

    if (RegisterClass(&WindowClass))
    {
        HWND Window =
            CreateWindowEx(
                0,
                WindowClass.lpszClassName,
                "Handmade Hero",
                WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                0,
                0,
                Instance,
                0);

        if (Window)
        {
            HDC DeviceContext = GetDC(Window);

            // NOTE(greg): graphics test
            int XOffset = 0;
            int YOffset = 0;

            // NOTE(greg): sound test
            int SamplesPerSecond = 48000;
            int Hz = 256;
            int16 ToneVolume = 6000;
            uint32 RunningSampleIndex = 0;
            int SquareWavePeriod = SamplesPerSecond / Hz;
            int HalfSquareWavePeriod = SquareWavePeriod / 2;
            int BytesPerSample = sizeof(int16) * 2;
            int SecondaryBufferSize = SamplesPerSecond * BytesPerSample;

            Win32InitDSound(Window, SamplesPerSecond, SecondaryBufferSize);
            GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

            Running = true;
            while (Running)
            {
                MSG Message;
                while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
                {
                    if (Message.message == WM_QUIT)
                    {
                        Running = false;
                    }

                    TranslateMessage(&Message);
                    DispatchMessage(&Message);
                }

                //TODO(greg): should we poll this more frequently?
                for (DWORD ControllerIndex = 0; ControllerIndex < XUSER_MAX_COUNT; ControllerIndex++)
                {
                    XINPUT_STATE ControllerState;
                    if (XInputGetState_(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
                    {
                        //NOTE(greg): This controller is plugged in
                        XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;

                        bool Up = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
                        bool Down = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
                        bool Left = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
                        bool Right = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
                        bool Start = (Pad->wButtons & XINPUT_GAMEPAD_START);
                        bool Back = (Pad->wButtons & XINPUT_GAMEPAD_BACK);
                        bool LeftShoulder = (Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
                        bool RightShoulder = (Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
                        bool AButton = (Pad->wButtons & XINPUT_GAMEPAD_A);
                        bool BButton = (Pad->wButtons & XINPUT_GAMEPAD_B);
                        bool XButton = (Pad->wButtons & XINPUT_GAMEPAD_X);
                        bool YButton = (Pad->wButtons & XINPUT_GAMEPAD_Y);

                        int16 StickX = Pad->sThumbLX;
                        int16 StickY = Pad->sThumbLY;

                        XOffset += StickX >> 12;
                        YOffset += StickY >> 12;
                    }
                    else
                    {
                        //NOTE(greg): This controller is unplugged
                    }
                }

                RenderWeirdGradient(&GlobalBackBuffer, XOffset, YOffset);

                DWORD PlayCursor;
                DWORD WriteCursor;

                // NOTE(greg): DirectSound output test
                if (SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor)))
                {
                    DWORD ByteToLock = (RunningSampleIndex * BytesPerSample) % SecondaryBufferSize;
                    DWORD BytesToWrite;
                    if (ByteToLock > PlayCursor)
                    {
                        BytesToWrite = (SecondaryBufferSize - ByteToLock);
                        BytesToWrite += PlayCursor;
                    }
                    else
                    {
                        BytesToWrite = PlayCursor - ByteToLock;
                    }
                    
                    VOID *Region1;
                    DWORD Region1Size;
                    VOID *Region2;
                    DWORD Region2Size;

                    if (SUCCEEDED(GlobalSecondaryBuffer->Lock(ByteToLock, BytesToWrite,
                                                              &Region1, &Region1Size,
                                                              &Region2, &Region2Size,
                                                              0)))
                    {

                        //TODO(greg): assert that region1size/region2size is valid
                        //TODO(greg): Collapse these two loops
                        int16 *SampleOut = (int16 *)Region1;
                        DWORD Region1SampleCount = Region1Size / BytesPerSample;
                        for (DWORD SampleIndex = 0; SampleIndex < Region1Size; SampleIndex++)
                        {
                            int16 SampleValue = (RunningSampleIndex / HalfSquareWavePeriod) % 2 ? ToneVolume : -ToneVolume;
                            *SampleOut++ = SampleValue;
                            *SampleOut++ = SampleValue;
                            RunningSampleIndex++;
                        }

                        SampleOut = (int16 *)Region2;
                        DWORD Region2SampleCount = Region2Size / BytesPerSample;
                        for (DWORD SampleIndex = 0; SampleIndex < Region2Size; SampleIndex++)
                        {
                            int16 SampleValue = (RunningSampleIndex / HalfSquareWavePeriod) % 2 ? ToneVolume : -ToneVolume;
                            *SampleOut++ = SampleValue;
                            *SampleOut++ = SampleValue;
                            RunningSampleIndex++;
                        }

                        GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
                    }
                }

                win32_window_dimensions Dimensions = Win32GetWindowDimensions(Window);
                Win32DisplayBufferInWindow(&GlobalBackBuffer, DeviceContext, Dimensions.Width, Dimensions.Height);
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
