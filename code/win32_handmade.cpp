#include <cstdint>
#include <windows.h>
#include <xinput.h>

#define internal static
#define local_persist static
#define global_variable static

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int32_t bool32;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

struct win32_offscreen_buffer {
  BITMAPINFO Info;
  void *BitmapMemory;
  int Width;
  int Height;
  int BytesPerPixel;
};

// TODO: This is a global for now
global_variable bool32 GlobalRunning;
global_variable win32_offscreen_buffer GlobalBackBuffer;

struct win32_window_dimension {
  int Width;
  int Height;
};

// NOTE: XInputGetState
#define X_INPUT_GET_STATE(name)                                                \
  DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub) { return ERROR_DEVICE_NOT_CONNECTED; }
global_variable x_input_get_state *DyXInputGetState = XInputGetStateStub;

// NOTE: XInputSetState
#define X_INPUT_SET_STATE(name)                                                \
  DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub) { return ERROR_DEVICE_NOT_CONNECTED; }
global_variable x_input_set_state *DyXInputSetState = XInputSetStateStub;

internal void Win32LoadXInput(void) {
  // TODO: Test this on windows 8
  HMODULE XInputLibrary = LoadLibraryA("xinput1_4.dll");
  if (!XInputLibrary) {
    XInputLibrary = LoadLibraryA("xinput1_3.dll");
  }

  if (XInputLibrary) {
    DyXInputGetState =
        (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
    DyXInputSetState =
        (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
  }
}

internal win32_window_dimension GetWindowDimension(HWND Window) {
  RECT ClientRect;
  GetClientRect(Window, &ClientRect);
  int WindowWidth = ClientRect.right - ClientRect.left;
  int WindowHeight = ClientRect.bottom - ClientRect.top;

  return {WindowWidth, WindowHeight};
}

internal void RenderWeirdGradient(win32_offscreen_buffer *Buffer, int XOffset,
                                  int YOffset) {
  int Width = Buffer->Width;
  int Height = Buffer->Height;

  int Pitch = Width * Buffer->BytesPerPixel;
  uint8 *Row = (uint8 *)Buffer->BitmapMemory;
  for (int Y = 0; Y < Buffer->Height; ++Y) {
    uint32 *Pixel = (uint32 *)Row;
    for (int X = 0; X < Buffer->Width; ++X) {
      uint8 Blue = (X + XOffset);
      uint8 Green = (Y + YOffset);

      *Pixel++ = ((Green << 8) | Blue);
    }

    Row += Pitch;
  }
}

internal void Win32ResizeDIBSection(win32_offscreen_buffer *Buffer, int Width,
                                    int Height) {
  if (Buffer->BitmapMemory) {
    VirtualFree(Buffer->BitmapMemory, 0, MEM_RELEASE);
  }

  Buffer->Width = Width;
  Buffer->Height = Height;
  Buffer->BytesPerPixel = 4;

  // NOTE: When the biHight is negative, this is a clue to Windows to treat
  // this bitmap as top-down, not bottom-up, meaning that the first three bytes
  // of the image are the color for the top left pixel in the bitmap, not the
  // bottom left
  Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
  Buffer->Info.bmiHeader.biWidth = Buffer->Width;
  Buffer->Info.bmiHeader.biHeight = -Buffer->Height;
  Buffer->Info.bmiHeader.biPlanes = 1;
  Buffer->Info.bmiHeader.biBitCount = 32;
  Buffer->Info.bmiHeader.biCompression = BI_RGB;

  int BitmapMemorySize =
      (Buffer->Width * Buffer->Height) * Buffer->BytesPerPixel;
  Buffer->BitmapMemory =
      VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

  // TODO: Clear this to black
}

internal void Win32DisplayBufferInWindow(win32_offscreen_buffer *Buffer,
                                         HDC DeviceContext, int WindowWidth,
                                         int WindowHeight, int X, int Y,
                                         int Width, int Height) {
  // StretchDIBits(DeviceContext, X, Y, Width, Height, X, Y, Width, Height,
  //               BitmapMemory, &BitmapInfo, DIB_RGB_COLORS, SRCCOPY);
  StretchDIBits(DeviceContext, 0, 0, WindowWidth, WindowHeight, 0, 0,
                Buffer->Width, Buffer->Height, Buffer->BitmapMemory,
                &Buffer->Info, DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK Win32MainWindowCallback(HWND Window, UINT Message,
                                         WPARAM WParam, LPARAM LParam) {
  LRESULT Result = 0;

  switch (Message) {
  case WM_SIZE: {
  } break;
  case WM_DESTROY: {
    // TODO: Handle this with an error - recreate window
    GlobalRunning = false;
  } break;
  case WM_SYSKEYDOWN:
  case WM_SYSKEYUP:
  case WM_KEYDOWN:
  case WM_KEYUP: {
    uint32 VKCode = WParam;
    bool32 WasDown = (LParam & (1 << 30)) != 0;
    bool32 IsDown = (LParam & (1 << 31)) == 0;

    if (WasDown != IsDown) {
      if (VKCode == 'W') {
        OutputDebugStringA("W \n");
      } else if (VKCode == 'A') {
        OutputDebugStringA("A");
      } else if (VKCode == 'S') {
        OutputDebugStringA("S");
      } else if (VKCode == 'D') {
        OutputDebugStringA("D");
      } else if (VKCode == 'Q') {
        OutputDebugStringA("Q");
      } else if (VKCode == 'E') {
        OutputDebugStringA("E");
      } else if (VKCode == VK_UP) {
        OutputDebugStringA("UP");
      } else if (VKCode == VK_DOWN) {
        OutputDebugStringA("DOWN");
      } else if (VKCode == VK_LEFT) {
        OutputDebugStringA("LEFT");
      } else if (VKCode == VK_RIGHT) {
        OutputDebugStringA("RIGHT");
      } else if (VKCode == VK_ESCAPE) {
        OutputDebugStringA("ESCAPE");
        if (IsDown) {
          OutputDebugStringA("IsDown");
        } else {
          OutputDebugStringA("WasDown");
        }
        OutputDebugStringA("\n");
      } else if (VKCode == VK_SPACE) {
        OutputDebugStringA("SPACE");
      }
    }

    bool32 AltKeyWasDown = (LParam & (1 << 29)) != 0;
    if ((VKCode == VK_F4) && AltKeyWasDown) {
      GlobalRunning = false;
    }
  } break;
  case WM_CLOSE: {
    // TODO: Handle this with a message to the user?
    GlobalRunning = false;
  } break;
  case WM_ACTIVATEAPP: {
    OutputDebugStringA("WM_ACTIVATEAPP");
  } break;
  case WM_PAINT: {
    PAINTSTRUCT Paint;
    OutputDebugStringA("WM_ACTIVATEAPP");
    HDC DeviceContext = BeginPaint(Window, &Paint);
    int X = Paint.rcPaint.left;
    int Y = Paint.rcPaint.top;
    int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;
    int Width = Paint.rcPaint.right - Paint.rcPaint.left;

    win32_window_dimension Dim = GetWindowDimension(Window);

    RECT ClientRect;
    GetClientRect(Window, &ClientRect);
    Win32DisplayBufferInWindow(&GlobalBackBuffer, DeviceContext, Dim.Width,
                               Dim.Height, X, Y, Width, Height);
    EndPaint(Window, &Paint);
  } break;
  default: {
    Result = DefWindowProc(Window, Message, WParam, LParam);
  } break;
  }

  return Result;
}

int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE PrevInstance,
                     LPSTR CommandLine, int ShowCode) {
  Win32LoadXInput();

  Win32ResizeDIBSection(&GlobalBackBuffer, 1280, 720);

  WNDCLASSA WindowClass = {};
  WindowClass.style = CS_HREDRAW | CS_VREDRAW;
  WindowClass.lpfnWndProc = Win32MainWindowCallback;
  WindowClass.hInstance = Instance;
  WindowClass.lpszClassName = "handmadeHeroWindowClass";

  if (RegisterClass(&WindowClass)) {
    HWND Window = CreateWindowExA(0, WindowClass.lpszClassName, "Handmade",
                                  WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                  CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                  CW_USEDEFAULT, 0, 0, Instance, 0);

    if (Window) {
      GlobalRunning = true;
      int XOffset = 0;
      int YOffset = 0;
      while (GlobalRunning) {
        MSG Message;
        while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE)) {
          if (Message.message == WM_QUIT) {
            GlobalRunning = false;
          }
          TranslateMessage(&Message);
          DispatchMessageA(&Message);
        }

        // TODO: Should we pull this more frequently
        for (DWORD ControllerIndex = 0; ControllerIndex < XUSER_MAX_COUNT;
             ControllerIndex++) {
          XINPUT_STATE ControllerState;
          if (DyXInputGetState(ControllerIndex, &ControllerState) ==
              ERROR_SUCCESS) {
            // NOTE: Controller pluged in
            // TODO: See if packet number increments too rapidly, to check if we
            // are not polling controller inputs rapidly enough
            XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;

            bool32 Up = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
            bool32 Down = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
            bool32 Left = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
            bool32 Right = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
            bool32 Start = (Pad->wButtons & XINPUT_GAMEPAD_START);
            bool32 Back = (Pad->wButtons & XINPUT_GAMEPAD_BACK);
            bool32 LeftShoulder =
                (Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
            bool32 RightShoulder =
                (Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
            bool32 AButton = (Pad->wButtons & XINPUT_GAMEPAD_A);
            bool32 BButton = (Pad->wButtons & XINPUT_GAMEPAD_B);
            bool32 XButton = (Pad->wButtons & XINPUT_GAMEPAD_X);
            bool32 YButton = (Pad->wButtons & XINPUT_GAMEPAD_Y);

            int16 StickX = Pad->sThumbLX;
            int16 StickY = Pad->sThumbLY;

            XOffset += StickX >> 12;
            YOffset += StickY >> 12;
          } else {
            // NOTE: Controller is not available
          }
        }

        RenderWeirdGradient(&GlobalBackBuffer, XOffset, YOffset);
        HDC DeviceContext = GetDC(Window);

        {
          win32_window_dimension Dim = GetWindowDimension(Window);

          Win32DisplayBufferInWindow(&GlobalBackBuffer, DeviceContext,
                                     Dim.Width, Dim.Height, 0, 0, Dim.Width,
                                     Dim.Height);
          ReleaseDC(Window, DeviceContext);
        }
      }
    } else {
      // TODO: logging
    }
  } else {
    // TODO: logging
  }

  return 0;
}
