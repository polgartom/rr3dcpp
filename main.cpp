#include "rr3dcpp.h"

// #define WINDOW_WIDTH  800
// #define WINDOW_HEIGHT 800
int WINDOW_WIDTH  = 800;
int WINDOW_HEIGHT = 800;

#define RGB_COLOR(r, g, b) (((r & 0xFF) << 16) | ((g & 0xFF) << 8) | ((b & 0xFF) << 0))

inline void set_pixel(int x, int y, u32 color)
{
    if (x >= 0 && x < WINDOW_WIDTH && y >= 0 && y < WINDOW_HEIGHT) {
        *((static_cast<u32 *>(global_back_buffer.memory)) + (y * WINDOW_WIDTH + x)) = color;
    }
}

inline u32 get_pixel(int x, int y)
{
    if (x >= 0 && x < WINDOW_WIDTH && y >= 0 && y < WINDOW_HEIGHT) {
        return *((static_cast<u32 *>(global_back_buffer.memory)) + (y * WINDOW_WIDTH + x));
    }
    return 0;
}

inline void set_zbuf(int x, int y, float z)
{
    if (x >= 0 && x < WINDOW_WIDTH && y >= 0 && y < WINDOW_HEIGHT) {
        *((static_cast<float *>(global_back_buffer.zbuffer)) + (y * WINDOW_WIDTH + x)) = z;
    }
}

inline float get_zbuf(int x, int y)
{
    if (x >= 0 && x < WINDOW_WIDTH && y >= 0 && y < WINDOW_HEIGHT) {
        return *((static_cast<float *>(global_back_buffer.zbuffer)) + (y * WINDOW_WIDTH + x));
    }
    return 0.0f;
}

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

inline float scr_rev_x(float x)
{
    return x * 2 / WINDOW_WIDTH - 1.0;
}

inline float scr_y(float y)
{
    return (y + 1.0) * WINDOW_WIDTH / 2;
}

inline float scr_rev_y(float y)
{
    return y * 2 / WINDOW_WIDTH - 1.0;
}

inline float map_z(float z, float zmin, float zmax)
{
    // return 1.0 - ((z - zmax) / (zmin - zmax)) + 1.0;
    return (1.0 - ((z - zmax) / (zmin - zmax)));
}

inline Vector3 project_to_screen(Vector3 v)
{
    Vector3 r;
    r.x = scr_x(v.x);
    r.y = scr_y(v.y);
    r.z = 0.0f;
    
    return r;
}

static float angle = 0.00f; 

void draw_line(float x1, float y1, float x2, float y2, float z1, float z2, u32 color = RGB_COLOR(255, 255, 255))
{
    bool steep = false;
    if (abs(x1 - x2) < abs(y1 - y2)) {
        steep = true;
        swap(&x1, &y1);
        swap(&x2, &y2);
    }

    if (x1 > x2) {
        swap(&x1, &x2);
        swap(&y1, &y2);
    }

    float xr1 = scr_x(x1);
    float xr2 = scr_x(x2);
    float delta = xr2 - xr1;

    // float z = max(z1, z2);
    float z = max(z1, z2);
    // if (z < 0.0f || z >= 1.0f) return;
    
    for (float x = xr1; x < xr2; x += 1.0f) {
        float t = (x - xr1) / delta;
        float y = y1*(1.0f-t) + (y2*t);
        // float z = z1*(1.0f-t) + (z2*t);
        y = scr_y(y);

        // if (z > 0.00001f) {

        if (steep) {
            set_pixel(y, x, color);
            float cz = get_zbuf(y, x);
            if (z > cz) {
                set_zbuf(y, x, z);
            }
        } else {
            set_pixel(x, y, color);
            float cz = get_zbuf(x, y);
            if (z > cz) {
                set_zbuf(x, y, z);
            }
        }
    }
}

inline void draw_line(Vector3 start, Vector3 end, u32 color = RGB_COLOR(255, 255, 255))
{
    float x1 = start.x;
    float y1 = start.y;
    float x2 = end.x;
    float y2 = end.y;
    
    draw_line(x1, y1, x2, y2, start.z, end.z, color);
}

#define MAX3(_a, _b, _c) (max(_c, max(_a, _b)))
#define MIN3(_a, _b, _c) (min(_c, min(_a, _b)))

inline void draw_triangle(Vector3 v1, Vector3 v2, Vector3 v3, u32 color = RGB_COLOR(255, 255, 255))
{
    float xmax = MAX3(v1.x, v2.x, v3.x);
    float xmin = MIN3(v1.x, v2.x, v3.x);
    
    float ymax = MAX3(v1.y, v2.y, v3.y);
    float ymin = MIN3(v1.y, v2.y, v3.y);
    
    float zmax = MAX3(v1.z, v2.z, v3.z);
    float zmin = MIN3(v1.z, v2.z, v3.z);
    
    Vector3 *a, *b, *c = nullptr;
    
    if (v1.y == ymin) a = &v1;
    else if (v2.y == ymin) a = &v2;
    else if (v3.y == ymin) a = &v3;
    
    if (v1.y == ymax) b = &v1;
    else if (v2.y == ymax) b = &v2;
    else if (v3.y == ymax) b = &v3;
    
    if (&v1 != a && &v1 != b) c = &v1;
    else if (&v2 != a && &v2 != b) c = &v2;
    else if (&v3 != a && &v3 != b) c = &v3;
    
    float y1 = scr_y(a->y);
    float y2 = scr_y(b->y);
    float x = scr_x(a->x);
    float m = ((a->x - b->x) / (a->y - b->y));
    for (float y = y1; y < y2; y += 1.0f) {
        draw_line(scr_rev_x(x), scr_rev_y(y), c->x, c->y, zmin, zmax, color);
        x+=m;
    }
 
    // draw_line(v1, v2, color);
    // draw_line(v2, v3, color);
    // draw_line(v3, v1, color);
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

    //     set_pixel(x, y, RGB_COLOR(255, 255, 255));
    // }

    for (s64 i = 0; i < m->faces.count; i++) {    
        Face f = m->faces[i];
        Vector3 v1 = m->vectors[f.v1];
        Vector3 v2 = m->vectors[f.v2];
        Vector3 v3 = m->vectors[f.v3];

        rotate_y(&v1, angle);
        rotate_y(&v2, angle);
        rotate_y(&v3, angle);
        
        draw_triangle(v1, v2, v3, RGB_COLOR(200, 200, 200));
    }    
}

inline void render_obj_file(Win32_Offscreen_Buffer *buffer, Model *m)
{
    angle += 0.010f;
    
    draw_mesh(m);
}

int CALLBACK
WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR command_line, int show_code)
{
    assert(AllocConsole()); // @temporary
    Win32LoadXInput();

    WNDCLASSA window_class = {};
    
    Win32ResizeDIBSection(&global_back_buffer, WINDOW_WIDTH, WINDOW_HEIGHT);
    clog("[Win32ResizeDIBSection]: %d %d\n", WINDOW_WIDTH, WINDOW_HEIGHT);
    
    window_class.style = CS_HREDRAW|CS_VREDRAW;
    window_class.lpfnWndProc = MainWindowCallback;
    window_class.hInstance = instance;
    // window_class.hIcon;
    window_class.lpszClassName = "RR3DCPPWindowClass";

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
            scale(m, 0.045f);
            
            float zmax, zmin = 0.0f;
            For (m->vectors) {
                if (it->z > zmax) zmax = it->z;
                if (it->z < zmin) zmin = it->z;
            }
            
            For_Index (m->vectors) {
                auto it = &m->vectors[it_index];
                it->z = map_z(it->z, zmin, zmax) - 0.5;
                PRINT_VEC(*it);
            }

            // Model *m = parse_obj_file(string_create(".\\obj\\cow.obj"));
            // scale(m, 0.12f);

            // Model *m = parse_obj_file(string_create(".\\obj\\teapot.obj"));
            // scale(m, 0.27f);

            // Model *m = parse_obj_file(string_create(".\\obj\\pumpkin.obj"));
            // scale(m, 0.07f);

            u64 cycle = 0;
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
                if (WINDOW_WIDTH != dimension.width || WINDOW_HEIGHT != dimension.height) {
                    WINDOW_WIDTH = dimension.width;
                    WINDOW_HEIGHT = dimension.height;
                    Win32ResizeDIBSection(&global_back_buffer, dimension.width, dimension.height);
                    clog("[Win32ResizeDIBSection]: %d %d\n", WINDOW_WIDTH, WINDOW_HEIGHT);
                }
                
                ZERO_MEMORY(global_back_buffer.memory, global_back_buffer.bitmap_memory_size);
                ZERO_MEMORY(global_back_buffer.zbuffer, global_back_buffer.bitmap_memory_size);
                render_obj_file(&global_back_buffer, m);
           
                // draw_triangle(
                //     {-0.8, -0.5, 0},
                //     {0.0, 0.1, 0},
                //     {-1.0, 0.8, 0}
                // );
                
                // draw_triangle(
                //     {-0.8, -0.5, 0},
                //     {0.0, 0.8, 0},
                //     {0.8, -0.5, 0},
                //     RGB_COLOR(200, 50, 90)
                // );
                
                // draw_triangle(
                //     {0.7, -0.5, 0},
                //     {0.2, 0.5, 0},
                //     {0.0, 0.7, 0},
                //     RGB_COLOR(50, 200, 100)
                // );
                
                // draw_triangle(
                //     {-0.7, -0.3, 0},
                //     {0.6, -0.4, 0},
                //     {0.7f, -0.45, 0},
                //     RGB_COLOR(50, 100, 200)
                // );
                
                for (u32 y = 0; y < WINDOW_HEIGHT; y++) {
                    for (u32 x = 0; x < WINDOW_WIDTH; x++) {
                        u32 p = get_pixel(x, y);
                        float zval = get_zbuf(x, y);
                        u8 r = ((p >> 16) & 0xFF) * (zval);
                        u8 g = ((p >> 8) & 0xFF) * (zval);
                        u8 b = ((p >> 0) & 0xFF) * (zval);
                        set_pixel(x, y, RGB_COLOR(r, g, b));
                    }
                }
                
                Win32DisplayBuffer(&global_back_buffer, device_context, dimension.width, dimension.height, 0, 0);
                ReleaseDC(window, device_context);
                
                cycle++;
            }
        } else {
            // @Todo: Logging
        }

    } else {
        // @Todo: Logging
    }

    return 0;
}