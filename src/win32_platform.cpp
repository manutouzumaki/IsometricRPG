#include <windows.h>
#include <Windowsx.h>
#include <xinput.h>
#include <math.h>
#include <stdio.h>

#include "platform.h"

#define XInputGetState XInputGetState_
#define XInputSetState XInputSetState_

#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
    return ERROR_DEVICE_NOT_CONNECTED;
}

#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
    return ERROR_DEVICE_NOT_CONNECTED;
}

global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;
global_variable b8 globalRunning = false;

// this have to be in the same oprder than the buttons on the InputState struct
global_variable u16 XInputButtons[] = 
{
    XINPUT_GAMEPAD_DPAD_UP,
    XINPUT_GAMEPAD_DPAD_DOWN,
    XINPUT_GAMEPAD_DPAD_LEFT,
    XINPUT_GAMEPAD_DPAD_RIGHT,
    XINPUT_GAMEPAD_START,
    XINPUT_GAMEPAD_BACK,
    XINPUT_GAMEPAD_A,
    XINPUT_GAMEPAD_B,
    XINPUT_GAMEPAD_X,
    XINPUT_GAMEPAD_Y
};

struct BackBuffer
{
    HBITMAP handle;
    void *memory;
    i32 width;
    i32 height;
};

struct GameCode
{
    game_update_and_render *UpdateAndRender;
};

LRESULT CALLBACK WindowProc(HWND hwnd, u32 msg, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = {};
    switch(msg)
    {
        case WM_CLOSE:
        {
            globalRunning = false;
        }break;
        case WM_DESTROY:
        {
            globalRunning = false;
        }break;


        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYUP:
        {
            Assert(!"ERROR: Input was not cache correctly")
        }break;


        default:
        {
           result = DefWindowProcA(hwnd, msg, wParam, lParam);
        }break; 
    }
    return result;
}

internal
HWND CreateAndInitWindow(i32 x, i32 y, i32 width, i32 height, HINSTANCE hInstance, i32 cmdShow)
{
    HWND result = 0;
    WNDCLASSEX windowClass = {};
    windowClass.cbSize = sizeof( WNDCLASSEX ) ;
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = hInstance;
    windowClass.hCursor = LoadCursor( 0, IDC_ARROW );
    windowClass.hbrBackground = ( HBRUSH )( COLOR_WINDOW + 1 );
    windowClass.lpszMenuName = 0;
    windowClass.lpszClassName = "WonderlandRPG";
    
    RegisterClassEx(&windowClass);

    RECT rect = {(LONG)x, (LONG)y, (LONG)width, (LONG)height};
    AdjustWindowRect( &rect, WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU, false );
    result = CreateWindowA("WonderlandRPG", "WonderlandRPG",
                           WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,
                           CW_USEDEFAULT, CW_USEDEFAULT,
                           rect.right - rect.left,
                           rect.bottom - rect.top,
                           0, 0, hInstance, 0);

    ShowWindow(result, cmdShow);

    return result;
}

internal
BackBuffer CreateBackBuffer(HDC deviceContext, i32 width, i32 height)
{
    BackBuffer backBuffer = {};
    BITMAPINFO bufferInfo = {}; 
    bufferInfo.bmiHeader.biSize = sizeof(bufferInfo.bmiHeader);
    bufferInfo.bmiHeader.biWidth = width;
    bufferInfo.bmiHeader.biHeight = -height;
    bufferInfo.bmiHeader.biPlanes = 1;
    bufferInfo.bmiHeader.biBitCount = 32;
    bufferInfo.bmiHeader.biCompression = BI_RGB;

    backBuffer.width = width;
    backBuffer.height = height;

    backBuffer.handle = CreateDIBSection(deviceContext, &bufferInfo,
                                         DIB_RGB_COLORS, &backBuffer.memory,
                                         0, 0);
    return backBuffer;
}

internal
void PresentBackBuffer(HDC deviceContext, BackBuffer *backBuffer)
{
    HDC backBufferDC = CreateCompatibleDC(deviceContext);
    SelectObject(backBufferDC, backBuffer->handle);
    BitBlt(deviceContext, 0, 0,
           backBuffer->width, backBuffer->height,
           backBufferDC, 0, 0, SRCCOPY);
    DeleteDC(backBufferDC);
}

internal
void LoadXInput()
{
    HMODULE XInputLibrary = LoadLibraryA("xinput1_4.dll");
    if(!XInputLibrary)
    {
        XInputLibrary = LoadLibraryA("xinput9_1_0.dll");
    }
    if(!XInputLibrary)
    {
        XInputLibrary = LoadLibraryA("xinput1_3.dll");
    }

    if(XInputLibrary)
    {
        XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState"); 
        XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
        if(!XInputGetState) XInputGetState = XInputGetStateStub; 
        if(!XInputSetState) XInputSetState = XInputSetStateStub; 
    }
}

internal
GameCode LoadGameCode()
{
    GameCode gameCode = {};
    HMODULE gameLibary = LoadLibraryA("game.dll");
    if(gameLibary)
    {
        gameCode.UpdateAndRender = (game_update_and_render *)GetProcAddress(gameLibary, "GameUpdateAndRender");
        if(!gameCode.UpdateAndRender)
        {
            Assert(!"ERROR!!! Game failed to LOAD...");
        }
    }
    return gameCode;
}

internal
f32 ProcessXInputStick(i16 value, i32 deadZoneValue)
{
    f32 result = 0;
    if(value < -deadZoneValue)
    {
        result = (f32)(value + deadZoneValue) / (32768.0f - deadZoneValue);
    }
    else if(value > deadZoneValue)
    {
        result = (f32)(value - deadZoneValue) / (32767.0f - deadZoneValue);
    }
    return result;
}

inline 
u32 SafeTruncateU64(u64 value)
{
    Assert(value <= 0xFFFFFFFF);
    u32 result = (u32)value;
    return(result);
}

DEBUG_READFILE(DEBUG_ReadFile)
{
    HANDLE fileHandle = CreateFileA(fileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if(fileHandle != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER fileSize;
        if(GetFileSizeEx(fileHandle, &fileSize))
        {
            u32 fileSize32 = SafeTruncateU64(fileSize.QuadPart);
            
            void *fileBuffer = PushArray(arena, u8, fileSize32);
            DWORD bytesRead = 0;
            if(ReadFile(fileHandle, fileBuffer, fileSize32, &bytesRead, 0) && (bytesRead == fileSize32))
            {
                return fileBuffer; 
            }
            else
            {
                OutputDebugString("ERROR loading FILE!!!\n");
                return 0; 
            }
        } 
    }
    return 0;
}

void ProcessKeyboardMessages(ButtonState *button, b8 isDown)
{
    if(button->isDown != isDown)
    {
        button->isDown = isDown;
    }
}

void PullWndMessages(InputState *currInput)
{
    MSG msg = {};
    while(PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
    {
        switch(msg.message)
        {
            case WM_KEYDOWN:
            case WM_SYSKEYDOWN:
            case WM_KEYUP:
            case WM_SYSKEYUP:
            { 
                b8 wasDown = ((msg.lParam & (1 << 30)) != 0);
                b8 isDown = ((msg.lParam & (1 << 31)) == 0);
                if(isDown != wasDown)
                {
                    DWORD vkCode = (DWORD)msg.wParam;
                    if(vkCode == 'W' || vkCode == VK_UP)
                    {
                        ProcessKeyboardMessages(&currInput->up, isDown);
                    }
                    if(vkCode == 'S' || vkCode == VK_DOWN)
                    { 
                        ProcessKeyboardMessages(&currInput->down, isDown);
                    }
                    if(vkCode == 'A' || vkCode == VK_LEFT)
                    {
                        ProcessKeyboardMessages(&currInput->left, isDown);
                    }
                    if(vkCode == 'D' || vkCode == VK_RIGHT)
                    {
                        ProcessKeyboardMessages(&currInput->right, isDown);
                    }
                    if(vkCode == VK_RETURN || vkCode == VK_SPACE)
                    {
                        ProcessKeyboardMessages(&currInput->start, isDown);
                    }
                    if(vkCode == VK_BACK || vkCode == VK_ESCAPE)
                    {
                        ProcessKeyboardMessages(&currInput->back, isDown);
                    }
                    if(vkCode == 0x31)
                    {
                        ProcessKeyboardMessages(&currInput->a, isDown);
                    }
                    if(vkCode == 0x32)
                    {
                        ProcessKeyboardMessages(&currInput->b, isDown);
                    }
                    if(vkCode == 0x33)
                    {
                        ProcessKeyboardMessages(&currInput->x, isDown);
                    }
                    if(vkCode == 0x34)
                    {
                        ProcessKeyboardMessages(&currInput->y, isDown);
                    }
                }
            }break;
            
            case WM_MOUSEMOVE:
            {
                currInput->mouseX = (i32)GET_X_LPARAM(msg.lParam); 
                currInput->mouseY = (i32)GET_Y_LPARAM(msg.lParam); 
            }break;
            case WM_LBUTTONDOWN:
            case WM_LBUTTONUP:
            case WM_RBUTTONDOWN:
            case WM_RBUTTONUP:
            case WM_MBUTTONDOWN:
            case WM_MBUTTONUP:
            {
                currInput->mouseLeft.isDown = ((msg.wParam & MK_LBUTTON) != 0);
                currInput->mouseMiddle.isDown = ((msg.wParam & MK_MBUTTON) != 0);
                currInput->mouseRight.isDown = ((msg.wParam & MK_RBUTTON) != 0);
            }break;

            default:
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }break;
        }
    }
}


i32 WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR cmdLine, i32 cmdShow)
{
    // TODO(manuto): WTF is this doing!!!!
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    HMODULE ole32Library = LoadLibraryA("ole32.dll");//!!
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    LARGE_INTEGER frequency = {};
    QueryPerformanceFrequency(&frequency);
    f64 invFrequency = 1.0f / (f64)frequency.QuadPart;
    
    timeBeginPeriod(1);

    LoadXInput();
    
    HWND hwnd = CreateAndInitWindow(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT,
                                    hInstance, cmdShow);

    HDC deviceContext = GetDC(hwnd);
     
    BackBuffer backBuffer = CreateBackBuffer(deviceContext, WINDOW_WIDTH, WINDOW_HEIGHT);
    
    GameCode game = LoadGameCode();

    GameMemory memory = {};
    memory.size = Megabyte(256); 
    memory.data = VirtualAlloc(0, memory.size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    memory.DEBUG_ReadFile = DEBUG_ReadFile;

    InputState inputs[2] = {};
    InputState *currInput = &inputs[0];
    InputState *lastInput = &inputs[1];

    globalRunning = true;
    
    LARGE_INTEGER lastCounter = {};
    QueryPerformanceCounter(&lastCounter);

    while(globalRunning)
    {               
/*
        LARGE_INTEGER workCounter = {};
        QueryPerformanceCounter(&workCounter);
        u64 deltaWork = workCounter.QuadPart - lastCounter.QuadPart;
        f32 secondsElapsed = ((f32)deltaWork * (f32)invFrequency);
        while(secondsElapsed < TARGET_SECONDS_PER_FRAME)
        {
            // TODO(manuto): improve Sleep granularity...
#if 0
            DWORD milisecondsToSleep = (DWORD)((TARGET_SECONDS_PER_FRAME - secondsElapsed) * 1000.0f);
            Sleep(milisecondsToSleep);  
#endif
            QueryPerformanceCounter(&workCounter);
            deltaWork = workCounter.QuadPart - lastCounter.QuadPart;
            secondsElapsed = ((f32)deltaWork * (f32)invFrequency);
        }
*/

        LARGE_INTEGER currentCounter = {};
        QueryPerformanceCounter(&currentCounter);
        f64 fps = (f64)frequency.QuadPart / (f64)(currentCounter.QuadPart - lastCounter.QuadPart);

        currInput->deltaTime = (f32)((f64)(currentCounter.QuadPart - lastCounter.QuadPart) / (f64)frequency.QuadPart);

#if 0    
        char buffer[100];
        sprintf_s(buffer, "fps: %f\n", fps);
        OutputDebugString(buffer);
#endif

        for(i32 i = 0; i < ArrayCount(currInput->buttons); ++i)
        {
            currInput->buttons[i].wasDown = false;
        }
        for(i32 i = 0; i < ArrayCount(currInput->mouseButtons); ++i)
        {
            currInput->mouseButtons[i].wasDown = false;
        }

        PullWndMessages(currInput);
        
        XINPUT_STATE state = {};
        if(XInputGetState(0, &state) == ERROR_SUCCESS)
        {
            XINPUT_GAMEPAD *pad = &state.Gamepad;
            for(i32 i = 0; i < ArrayCount(currInput->buttons); ++i)
            {
                currInput->buttons[i].isDown = pad->wButtons & XInputButtons[i];
            }
            currInput->leftStickX = ProcessXInputStick(pad->sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
            currInput->leftStickY = ProcessXInputStick(pad->sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
            currInput->rightStickX = ProcessXInputStick(pad->sThumbRX, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
            currInput->rightStickY = ProcessXInputStick(pad->sThumbRY, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
        }
        else
        {
            // NOTE(manuto): Controller is not connected
        }
        

        for(i32 i = 0; i < ArrayCount(currInput->buttons); ++i)
        {   
            if(lastInput->buttons[i].isDown)
            {
                currInput->buttons[i].wasDown = true; 
            }
        }
        for(i32 i = 0; i < ArrayCount(currInput->mouseButtons); ++i)
        {   
            if(lastInput->mouseButtons[i].isDown)
            {
                currInput->mouseButtons[i].wasDown = true; 
            }
        }
        
        GameBackBuffer gameBackBuffer = {};
        gameBackBuffer.memory = backBuffer.memory;
        gameBackBuffer.width = backBuffer.width;
        gameBackBuffer.height = backBuffer.height;
        game.UpdateAndRender(&memory, &gameBackBuffer, currInput);
        
        PresentBackBuffer(deviceContext, &backBuffer);
        
        *lastInput = *currInput;
        lastCounter = currentCounter;
        
    }

    VirtualFree(memory.data, 0, MEM_RELEASE);
    ReleaseDC(hwnd, deviceContext);

    OutputDebugString("Closing the Engine...\n");

    return 0;
}
