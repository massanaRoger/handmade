#include <cstdint>
#include <windows.h>

#define internal static
#define local_persist static
#define global_variable static

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

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
global_variable bool Running;
global_variable win32_offscreen_buffer GlobalBackBuffer;

struct win32_window_dimension {
  int Width;
  int Height;
};

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
    Running = false;
  } break;
  case WM_CLOSE: {
    // TODO: Handle this with a message to the user?
    Running = false;
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

  Win32ResizeDIBSection(&GlobalBackBuffer, 1280, 720);

  WNDCLASS WindowClass = {};
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
      Running = true;
      int XOffset = 0;
      int YOffset = 0;
      while (Running) {
        MSG Message;
        while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE)) {
          if (Message.message == WM_QUIT) {
            Running = false;
          }
          TranslateMessage(&Message);
          DispatchMessage(&Message);
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

        ++XOffset;
      }
    } else {
      // TODO: logging
    }
  } else {
    // TODO: logging
  }

  return 0;
}
