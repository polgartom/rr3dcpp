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

Vector3 z_scale(Vector3 v)
{
    float scale = 2.0f / (2.0f + v.z);
    v.x = v.x * scale;
    v.y = v.y * scale;
    return v; 
}

void transform_x(Model *m, float x)
{
    For (m->vectors) it->x += x;
}

void transform_y(Model *m, float y)
{
    For (m->vectors) it->y += y;
}

void transform_z(Model *m, float z)
{
    For (m->vectors) it->z += z;
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
    
    float z = max(z1, z2);
    
    for (float x = xr1; x < xr2; x += 1.0f) {
        float t = (x - xr1) / delta;
        float y = y1*(1.0f-t) + (y2*t);
        // float z = z1*(1.0f-t) + (z2*t);
        y = scr_y(y);

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
    start = z_scale(start);
    end = z_scale(end);
    
    if (start.x > 1.0f || start.x < -1.0f || start.y > 1.0f || start.y < -1.0f) return; 
    
    draw_line(start.x, start.y, end.x, end.y, start.z, end.z, color);
}

#define MAX3(_a, _b, _c) (max(_c, max(_a, _b)))
#define MIN3(_a, _b, _c) (min(_c, min(_a, _b)))

inline void draw_triangle(Vector3 v1, Vector3 v2, Vector3 v3, u32 color = RGB_COLOR(255, 255, 255))
{
#if 0
    // Fill rectangle
    
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
        Vector3 vs1 =  {scr_rev_x(x), scr_rev_y(y), zmin};
        Vector3 vs2 = {c->x, c->y, zmax};
        draw_line(vs1.x, vs1.y, vs2.x, vs2.y, vs1.z, vs2.z, color);
        x+=m;
    }
#endif
 
    draw_line(v1, v2, color);
    draw_line(v2, v3, color);
    draw_line(v3, v1, color);
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
            scale(m, 0.035f);
            
            // Model *m = parse_obj_file(string_create(".\\obj\\cow.obj"));
            // scale(m, 0.12f);

            // Model *m = parse_obj_file(string_create(".\\obj\\teapot.obj"));
            // scale(m, 0.27f);

            // Model *m = parse_obj_file(string_create(".\\obj\\pumpkin.obj"));
            // scale(m, 0.07f);
            
            float zmax, zmin = 0.0f;
            For_Index (m->vectors) {
                auto it = &m->vectors[it_index];
                if (it->z > zmax) zmax = it->z;
                if (it->z < zmin) zmin = it->z;
            }
            
            For_Index (m->vectors) {
                auto it = &m->vectors[it_index];
                it->z = map_z(it->z, zmin, zmax) - 1.0;
                CLOG_VEC(*it);
            }

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
                
                // CLOG1(CLOG_D(vk_alt_was_down));
                
                if (vk_key_pressed == VK_UP) {
                    if (vk_alt_was_down) {
                        transform_z(m, 0.01);
                    } else {
                        transform_y(m, 0.01);
                    }
                } else if (vk_key_pressed == VK_DOWN) {
                    if (vk_alt_was_down) {
                        transform_z(m, -0.01);
                    } else {
                        transform_y(m, -0.01);
                    }
                } else if (vk_key_pressed == VK_LEFT) {                    
                    if (vk_alt_was_down) {
                        rotate_y(m, -0.01);
                    } else {
                        transform_x(m, -0.01);
                    }
                } else if (vk_key_pressed == VK_RIGHT) {
                    if (vk_alt_was_down) {
                        rotate_y(m, 0.01);
                    } else {
                        transform_x(m, 0.01);
                    }
                }
                
                render_obj_file(&global_back_buffer, m);
           
                // for (u32 y = 0; y < WINDOW_HEIGHT; y++) {
                //     for (u32 x = 0; x < WINDOW_WIDTH; x++) {
                //         u32 p = get_pixel(x, y);
                //         float zval = get_zbuf(x, y);
                //         u8 r = ((p >> 16) & 0xFF) * (zval);
                //         u8 g = ((p >> 8) & 0xFF) * (zval);
                //         u8 b = ((p >> 0) & 0xFF) * (zval);
                //         set_pixel(x, y, RGB_COLOR(r, g, b));
                //     }
                // }
                
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