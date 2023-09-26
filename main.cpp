#include "rr3dcpp.h"

#define WINDOW_WIDTH  800
#define WINDOW_HEIGHT 800

#define SET_PIXEL(__x, __y, _color) { \
    int _x = (int)__x; \
    int _y = (int)__y; \
    if (_x >= 0 && _x < WINDOW_WIDTH && _y >= 0 && _y < WINDOW_HEIGHT) { \
        u32 _p = (_y) * (WINDOW_HEIGHT) + (_x); \
        if (_p >= 1 && _p <= ((global_back_buffer.bitmap_memory_size-1)/global_back_buffer.bytes_per_pixel)) { \
            *(((u32 *)global_back_buffer.memory)+_p) = (u32)_color; \
        } \
    } else { \
    \
    }\
}\

#define RGB_COLOR(r, g, b) (((r&0xFF) << 16) | ((g&0xFF) << 8) | ((b&0xFF) << 0))

Model *parse_obj_file(String obj_filename) 
{
    String obj = read_entire_file(obj_filename.data);    
    String _obj = obj; // backup

    Model *m = new Model();

    float r = 0.0f;
    int ri = 0;
    bool s = true;
    String rem = obj;
    
    while (obj.count-1) {
        char c = *obj.data;
        if (c == 'v') {
            string_advance(&obj, 2);

            Vector3 v = {0};

            r = string_to_float(obj, &s, &rem);
            assert(s);
            obj = rem;
            v.x = r;

            r = string_to_float(obj, &s, &rem);
            assert(s);
            obj = rem;
            v.y = r;

            r = string_to_float(obj, &s, &rem);
            assert(s);
            obj = rem;
            v.z = r;

            array_add(&m->vectors, v);
                        
        }
        
        string_advance(&obj);
    }

    obj = _obj;
    while (obj.count-1) {
        char c = *obj.data;

        if (c == 'f') {
            string_advance(&obj, 2);

            Face f = {0};

            ri = string_to_int(obj, &s, &rem);
            assert(s);
            obj = rem;
            f.v1 = &m->vectors[ri-1];
            
            ri = string_to_int(obj, &s, &rem);
            assert(s);
            obj = rem;
            f.v2 = &m->vectors[ri-1];
            
            ri = string_to_int(obj, &s, &rem);
            assert(s);
            obj = rem;
            f.v3 = &m->vectors[ri-1];

            array_add(&m->faces, f);
        }
        
        string_advance(&obj);
    }
    
    return m;
}

Vector3 project_to_screen(Vector3 v)
{
    Vector3 r;
    r.x = (v.x+1.0)*WINDOW_WIDTH/2;
    r.y = (v.y+1.0)*WINDOW_HEIGHT/2;
    r.z = 0.0f;
    
    return r;
}

inline Vector3 multiply(Vector3 v, Matrix3 m) 
{
    Vector3 r;
    r.x = (v.x * m._00) + (v.y * m._01) + (v.z * m._02);
    r.y = (v.x * m._10) + (v.y * m._11) + (v.z * m._12);
    r.z = (v.x * m._20) + (v.y * m._21) + (v.z * m._22);
    
    return r;
}

inline void scale(Vector3 *v, float scale)
{
    v->x *= scale;
    v->y *= scale;
    v->z *= scale;
}

void rotate_x(Vector3 *v, float angle) 
{
    Matrix3 m = {
        1.0f, 0.0f, 0.0f,
        0.0f, (float)cos(angle), (float)-sin(angle),
        0.0f, (float)sin(angle), (float)cos(angle),
    };

    Vector3 r = multiply(*v, m);

    v->x = r.x;
    v->y = r.y;
    v->z = r.z;
}

void rotate_y(Vector3 *v, float angle) 
{
    Matrix3 m = {
        (float)cos(angle), 0, (float)sin(angle),
        0, 1, 0,
        (float)-sin(angle), 0, (float)cos(angle),
    };

    Vector3 r = multiply(*v, m);

    v->x = r.x;
    v->y = r.y;
    v->z = r.z;
}

void rotate_z(Vector3 *v, float angle) 
{
    Matrix3 m = {
        (float)cos(angle), (float)-sin(angle), 0,
        (float)sin(angle), (float)cos(angle), 0,
        1, 0, 1,
    };

    Vector3 r = multiply(*v, m);

    v->x = r.x;
    v->y = r.y;
    v->z = r.z;
}

static float angle = 0.00f; 

void draw_line(float x1, float y1, float x2, float y2)
{
    bool steep = false;
    if ((x1 - x2) < (y1 - y2)) {
        steep = true;
        float temp = x1;
        x1 = y1;
        y1 = temp;
        
        temp = x2;
        x2 = y2;
        y2 = temp;
    }

    if (x1 > x2) {
        float temp = x2;
        x2 = x1;
        x1 = temp;
        
        temp = y2;
        y2 = y1;
        y1 = temp;
    }

    float delta = x2 - x1;
    for (float x = x1; x < x2; x += 0.001f) {
        float t = (x - x1) / delta;
        float y = y1*(1.0-t) + (y2*t);
        
        Vector3 v = { x, y, 0 };
        v = project_to_screen(v);
        
        if (steep) {
            SET_PIXEL((int)v.y, (int)v.x, RGB_COLOR(255, 255, 255));
        } else {
            SET_PIXEL((int)v.x, (int)v.y, RGB_COLOR(255, 255, 255));
        }
    }
}
void draw_line(Vector3 start, Vector3 end)
{
    float x1 = start.x;
    float y1 = start.y;
    float x2 = end.x;
    float y2 = end.y;
    
    draw_line(x1, y1, x2, y2);
}

static float ss = 0.04f;

void render_obj_file(Win32_Offscreen_Buffer *buffer, Model *m, int width, int height)
{
    angle += 0.005f;
    
    // for (s64 i = 0; i < m->vectors.count; i++) {
    //     Vector3 v = m->vectors[i];
    //     scale(&v, ss);
    //     rotate_y(&v, angle);
    //     v = project_to_screen(v);
    //     if (v.x > lg.x) lg = v;
    //     int x = (int)v.x;
    //     int y = (int)v.y;

    //     SET_PIXEL(x, y, RGB_COLOR(255, 255, 255));
    // }

    for (s64 i = 0; i < m->faces.count; i++) {
        Face f = m->faces[i];
        Vector3 v1 = *m->faces[i].v1;
        Vector3 v2 = *m->faces[i].v2;
        Vector3 v3 = *m->faces[i].v3;
        
        rotate_y(&v1, angle);
        rotate_y(&v2, angle);
        rotate_y(&v3, angle);
        
        scale(&v1, ss);
        scale(&v2, ss);
        scale(&v3, ss);
        
        draw_line(v1, v2);
        draw_line(v2, v3);
        draw_line(v3, v1);
    }
}

int CALLBACK
WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR command_line, int show_code)
{
    Win32LoadXInput();

    WNDCLASSA window_class = {};
    
    Win32ResizeDIBSection(&global_back_buffer, WINDOW_WIDTH, WINDOW_HEIGHT);
    
    window_class.style = CS_HREDRAW|CS_VREDRAW;
    window_class.lpfnWndProc = MainWindowCallback;
    window_class.hInstance = instance;
    // window_class.hIcon;
    window_class.lpszClassName = "RR3DCPPWindowClass";

    int x = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
    x = x;

    assert(AllocConsole()); // @temporary

    if (RegisterClass(&window_class)) {
        HWND window =
            CreateWindowExA(
                0,
                window_class.lpszClassName,
                "RR3DCPP",
                WS_OVERLAPPEDWINDOW|WS_VISIBLE,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                WINDOW_WIDTH, // CW_USEDEFAULT
                WINDOW_HEIGHT, // CW_USEDEFAULT
                0,
                0,
                instance,
                0);

        if (window) {
            MSG message;
            global_running = true;
            int xoffset = 0;
            int yoffset = 0;
            
            //Win32InitDSound();
            
            Model *m = parse_obj_file(string_create(".\\obj\\teddy.obj"));
            ss = 0.04f;

            // Model *m = parse_obj_file(string_create(".\\obj\\cow.obj"));
            // ss = 0.12f;

            // Model *m = parse_obj_file(string_create(".\\obj\\teapot.obj"));
            // ss = 0.27f;

            // Model *m = parse_obj_file(string_create(".\\obj\\pumpkin.obj"));
            // ss = 0.007f;

            while (global_running) {            
                MSG message;
                while (PeekMessage(&message, 0, 0, 0, PM_REMOVE)) {
                    if (message.message == WM_QUIT) {
                        global_running = false;
                    }
                    
                    TranslateMessage(&message);
                    DispatchMessageA(&message); // call the MainWindowCallback fn
                }
                
                HDC device_context = GetDC(window);
                Win32_Window_Dimension dimension = Win32GetWindowDimension(window);
                
                ZERO_MEMORY(global_back_buffer.memory, global_back_buffer.bitmap_memory_size);
                render_obj_file(&global_back_buffer, m, dimension.width, dimension.height);
                       
                Win32DisplayBuffer(&global_back_buffer, device_context, dimension.width, dimension.height, 0, 0);
                ReleaseDC(window, device_context);
            }
        } else {
            // @Todo: Logging
        }

    } else {
        // @Todo: Logging
    }

    return 0;
}