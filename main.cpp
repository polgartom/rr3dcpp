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

    Model *m = new Model();

    float r = 0.0f;
    int ri = 0;
    bool s = true;
    String rem = obj;
    
    while (obj.count-1) {
        if (*obj.data == 'v') {
            string_advance(&obj, 2);
            if (*obj.data != '-' && !IS_DIGIT(*obj.data)) continue;
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
        else if (*obj.data == 'f') {
            string_advance(&obj, 2);
            if (!(*obj.data >= '1' && *obj.data <= '9')) continue;

            Face f = {0};

            ri = string_to_int(obj, &s, &rem);
            assert(s);
            obj = rem;
            f.v1 = ri-1;
            
            // skip not handled stuffs, every vertex index start after ' ' (space character)
            obj = eat_string_until(obj, ' '); 
            
            ri = string_to_int(obj, &s, &rem);
            assert(s);
            obj = rem;
            f.v2 = ri-1;
            
            // skip not handled stuffs, every vertex index start after ' ' (space character)
            obj = eat_string_until(obj, ' ');
            
            ri = string_to_int(obj, &s, &rem);
            assert(s);
            obj = rem;
            f.v3 = ri-1;
            
            array_add(&m->faces, f);
        }
        
        string_advance(&obj);
    }

    string_free(&obj);

    clog("obj parsed: " SFMT "\n", SARG(obj_filename));
    clog("vertices: %d ; faces: %d\n", m->vectors.count, m->faces.count);
    
    return m;
}

inline float scr_x(float x)
{
    return (x + 1.0) * WINDOW_WIDTH / 2;   
}

inline float scr_y(float y)
{
    return (y + 1.0) * WINDOW_HEIGHT / 2;   
}

inline Vector3 project_to_screen(Vector3 v)
{
    Vector3 r;
    r.x = scr_x(v.x);
    r.y = scr_y(v.y);
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

inline void scale(Model *m, float scale)
{    
    m->scale = scale;
    for (int i = 0; i < m->vectors.count; i++) {
        Vector3 *v = &m->vectors[i];
        v->x *= scale;
        v->y *= scale;
        v->z *= scale;
        // This is a fucking bug or what??????????? -> ..\main.cpp(127): error C2064: term does not evaluate to a function taking 2 arguments
        // scale(v, scale);
    }
}

inline void rotate_x(Vector3 *v, float angle) 
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

inline void rotate_x(Model *m, float scale)
{
    for (int i = 0; i < m->vectors.count; i++) {
        rotate_x(&m->vectors[i], scale);
    }
}

inline void rotate_y(Vector3 *v, float angle) 
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

inline void rotate_y(Model *m, float scale)
{
    for (int i = 0; i < m->vectors.count; i++) {
        rotate_y(&m->vectors[i], scale);
    }
}

inline void rotate_z(Vector3 *v, float angle) 
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

inline void rotate_z(Model *m, float scale)
{
    for (int i = 0; i < m->vectors.count; i++) {
        rotate_z(&m->vectors[i], scale);
    }
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

    float xr1 = scr_x(x1);
    float xr2 = scr_x(x2);

    float delta = xr2 - xr1;
    for (float x = xr1; x < xr2; x += 1.0f) {
        float t = (x - xr1) / delta;
        float y = y1*(1.0f-t) + (y2*t);
        y = scr_y(y);

        if (steep) {
            SET_PIXEL(y, x, RGB_COLOR(255, 255, 255));
        } else {
            SET_PIXEL(x, y, RGB_COLOR(255, 255, 255));
        }
    }
}

inline void draw_line(Vector3 start, Vector3 end)
{
    float x1 = start.x;
    float y1 = start.y;
    float x2 = end.x;
    float y2 = end.y;
    
    draw_line(x1, y1, x2, y2);
}

inline void draw_mesh(Model *m)
{
    // for (s64 i = 0; i < m->vectors.count; i++) {
    //     Vector3 v = m->vectors[i];
    //     // clog(VEC_FMT "\n", VEC_ARG(v));
    //     scale(&v, ss);
    //     rotate_y(&v, angle);
    //     v = project_to_screen(v);
    //     int x = (int)v.x;
    //     int y = (int)v.y;

    //     SET_PIXEL(x, y, RGB_COLOR(255, 255, 255));
    // }

    for (s64 i = 0; i < m->faces.count; i++) {    
        Face f = m->faces[i];
        Vector3 v1 = m->vectors[f.v1];
        Vector3 v2 = m->vectors[f.v2];
        Vector3 v3 = m->vectors[f.v3];

        rotate_y(&v1, angle);
        rotate_y(&v2, angle);
        rotate_y(&v3, angle);
        
        draw_line(v1, v2);
        draw_line(v2, v3);
        draw_line(v3, v1);
    }    
}

inline void render_obj_file(Win32_Offscreen_Buffer *buffer, Model *m, int width, int height)
{
    angle += 0.005f;
    
    draw_mesh(m);
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
            scale(m, 0.04f);
            rotate_x(m, 0.3f);

            // Model *m = parse_obj_file(string_create(".\\obj\\cow.obj"));
            // scale(m, 0.12f);

            // Model *m = parse_obj_file(string_create(".\\obj\\teapot.obj"));
            // scale(m, 0.27f);

            // Model *m = parse_obj_file(string_create(".\\obj\\pumpkin.obj"));
            // scale(m, 0.07f);

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