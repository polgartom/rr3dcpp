#include <windows.h>
#include <stdint.h>
#include <xinput.h>
#include <dsound.h>
#include <d3d9.h>
#include <windowsx.h>

void clog(const char *__fmt_msg, ...);

#define internal static
#define local_persist static
#define global_variable static

typedef uint8_t  uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;
typedef uint32_t bool32;

typedef int8_t  int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

struct Win32_Offscreen_Buffer
{
    BITMAPINFO info;
    void *memory;
    void *zbuffer;
    int bitmap_memory_size;
    int width;
    int height;
    int pitch;
    int bytes_per_pixel;
};

struct Win32_Window_Dimension
{
    int width;
    int height;
};

// TODO(polgar): XInputGetState
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE     *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
    return ERROR_DEVICE_NOT_CONNECTED;
}
global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

// TODO(polgar): XInputSetstate
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
    return ERROR_DEVICE_NOT_CONNECTED;
}
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);

// kacsa
uint32 vk_key_pressed = 0;
bool vk_alt_was_down = false;
s32 mouse_x = 0;
s32 mouse_y = 0;
bool mouse_left_down = false;

internal void
Win32LoadXInput(void)
{
    HMODULE XInputLibrary = LoadLibraryA("xinput1_3.dll");
    if (!XInputLibrary) {
        XInputLibrary = LoadLibraryA("xinput1_3.dll");
    }

    if (XInputLibrary) {
        XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
        XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
        
        // TODO(polgar): Diagnostic        
    } else {
        // TODO(polgar): Diagnostic        
    }
}

// TODO(polgar): This is a global for now.
global_variable bool global_running;
global_variable Win32_Offscreen_Buffer global_back_buffer;

internal Win32_Window_Dimension 
Win32GetWindowDimension(HWND window)
{
    Win32_Window_Dimension result;
    
    RECT client_rect;
    GetClientRect(window, &client_rect);
    result.width  = client_rect.right - client_rect.left;
    result.height = client_rect.bottom - client_rect.top;

    return result;    
}

internal void
RenderGradient(Win32_Offscreen_Buffer *buffer, int xoffset, int yoffset)
{
    // TODO(polgar): Let's see what the optimizer does
    uint8 *row = (uint8 *)buffer->memory;
    for (int y = 0; y < buffer->height; y++) {
        uint32 *pixel = (uint32 *)row;
        for (int x = 0; x < buffer->width; x++) {
            /*
                Memory:     BB GG RR xx
                Register:   xx RR GG BB
            */

            uint8 blue  = (x + xoffset);
            uint8 green = (y + yoffset);
            
            *pixel++ = ((green << 8) | blue);
        }
        
        row += buffer->pitch;
    }
}

internal void
Win32ResizeDIBSection(Win32_Offscreen_Buffer *buffer, int width, int height)
{
    // TODO(polgar): Bulletproof this
    // Maybe dont free first, free after, then free first if that fails.

    if (buffer->memory) {
        VirtualFree(buffer->memory, 0,  MEM_RELEASE);
    }
    if (buffer->zbuffer) {
        VirtualFree(buffer->zbuffer, 0,  MEM_RELEASE);
    }

    buffer->width = width;
    buffer->height = height;
    buffer->bytes_per_pixel = 4;

    buffer->info.bmiHeader.biSize = sizeof(buffer->info.bmiHeader);
    buffer->info.bmiHeader.biWidth = buffer->width;
    // NOTE(polgar): if we're using negative value for the height, then the 
    // frame buffer start at top-left corner, so less headache.
    // buffer->info.bmiHeader.biHeight = -buffer->height;
    buffer->info.bmiHeader.biHeight = buffer->height;
    buffer->info.bmiHeader.biPlanes = 1;
    buffer->info.bmiHeader.biBitCount = 32;
    buffer->info.bmiHeader.biCompression = BI_RGB;

    int bitmap_memory_size = (buffer->width*buffer->height)*buffer->bytes_per_pixel;    
    buffer->memory = VirtualAlloc(0, bitmap_memory_size, MEM_COMMIT, PAGE_READWRITE);
    buffer->zbuffer = VirtualAlloc(0, bitmap_memory_size, MEM_COMMIT, PAGE_READWRITE);
    buffer->bitmap_memory_size = bitmap_memory_size;
    
    buffer->pitch = width*buffer->bytes_per_pixel;
}

internal void
Win32DisplayBuffer(Win32_Offscreen_Buffer *buffer, HDC device_context, int window_width, int window_height, int x, int y)
{
    StretchDIBits(device_context, 
                  /*
                  x, y, width, height,
                  x, y, width, height,
                  */
                  0, 0, window_width, window_height,
                  0, 0, buffer->width, buffer->height,
                  buffer->memory,
                  &buffer->info,
                  DIB_RGB_COLORS,
                  SRCCOPY);
}

internal LRESULT CALLBACK
MainWindowCallback(HWND window, UINT message, WPARAM w_param, LPARAM l_param)
{
    LRESULT result = 0;

    mouse_left_down = false;

    switch (message) {
        case WM_SIZE: {
            break;
        }
        case WM_CLOSE: {
            global_running = false;
            break;
        }
        case WM_DESTROY: {
            global_running = false;
            break;
        }
        case WM_ACTIVATEAPP: {
            OutputDebugStringA("WM_ACTIVATEAPP\n");
            break;
        }
        case WM_MOUSEMOVE:
        case WM_LBUTTONDOWN: {
            mouse_x = GET_X_LPARAM(l_param); 
            mouse_y = GET_Y_LPARAM(l_param); 
            mouse_left_down = w_param & MK_LBUTTON;
            break;
        }
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP: {
            uint32 vk_code = w_param; 
            bool was_down  = (l_param & (1 << 30)) != 0;
            bool is_down   = (l_param & (1 << 31)) == 0; 
            
            vk_key_pressed = 0;
            if (is_down) {
                vk_key_pressed = vk_code;
                // kacsa   
            }
            
            if (is_down != was_down) {
                if (vk_code == 'W') {
                }
                else if (vk_code == 'A') {
                }
                else if (vk_code == 'S') {
                }
                else if (vk_code == 'D') {
                }
                else if (vk_code == 'Q') {
                }
                else if (vk_code == 'E') {
                }
                else if (vk_code == VK_UP) {
                }
                else if (vk_code == VK_LEFT) {
                }
                else if (vk_code == VK_RIGHT) {
                }
                else if (vk_code == VK_DOWN) {
                }
                else if (vk_code == VK_ESCAPE) {
                    // OutputDebugStringA("Escape: ");
                    clog("Escape: ");
                    if (is_down) {
                        clog("is_down\n");
                    } else if (was_down) {
                        clog("was_down\n");
                    }
                }
                else if (vk_code == VK_SPACE) {
                }
            }
            
            vk_alt_was_down = (l_param & (1 << 29)) != 0;
            if (vk_code == VK_F4 && vk_alt_was_down) {
                global_running = false;
            }
            
            break;
        }
        case WM_PAINT: {
            PAINTSTRUCT paint;
            HDC device_context = BeginPaint(window, &paint);
            int x = paint.rcPaint.left;
            int y = paint.rcPaint.left;
            
            Win32_Window_Dimension dimension = Win32GetWindowDimension(window);
            Win32DisplayBuffer(&global_back_buffer, device_context, dimension.width, dimension.height, x, y);
            
            EndPaint(window, &paint);
            
            break;
        }

        default: {
            result = DefWindowProc(window, message, w_param, l_param);
            break;
        }
    }

    return result;
}