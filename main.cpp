#include "rr3dcpp.h"

float lerp(float a, float b, float f)
{
    return a * (1.0 - f) + (b * f);
}

// #define WINDOW_WIDTH  800
// #define WINDOW_HEIGHT 800
int WINDOW_WIDTH  = 800;
int WINDOW_HEIGHT = 800;

Model *selected_model = nullptr;
Model *hovered_model = nullptr;

#define RGB_COLOR(r, g, b) (((r & 0xFF) << 16) | ((g & 0xFF) << 8) | ((b & 0xFF) << 0))

inline Vector3 rgb_unpack(u32 rgb)
{
    return {
        static_cast<float>((rgb >> 16) & 0xFF),
        static_cast<float>((rgb >> 8) & 0xFF),
        static_cast<float>((rgb >> 0) & 0xFF)
    };
}

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

inline Vector3 project(Vector3 v)
{
    float f_near = 0.1f;
    float f_far  = 1000.0f;
    // float f_fov  = 90.0f;
    float f_fov  = 90.0f;
    float f_aspect_ratio = ((float)WINDOW_HEIGHT / (float)WINDOW_WIDTH);
    float f_fov_rad = 1.0f / tanf(f_fov * 0.5f / 180.0f * 3.14159f);

    Matrix4 m = {0};
    m._00 = f_aspect_ratio * f_fov_rad;
    m._11 = f_fov_rad;
    m._22 = f_far / (f_far - f_near);
    m._32 = (-f_far * f_near) / (f_far - f_near);
    m._23 = 1.0f;
    m._33 = 0.0f;

    return multiply(v, m);
}

inline void transform_x(Model *m, float x)
{
    m->x += x;
}

inline void transform_y(Model *m, float y)
{
    m->y += y;
}

inline void transform_z(Model *m, float z)
{
    m->z += z;
}

inline float map_z(float z, float zmin, float zmax)
{
    // return 1.0 - ((z - zmax) / (zmin - zmax)) + 1.0;
    return (((z - zmax) / (zmin - zmax)));
}

void draw_2dline(Model *m, Vector2 start, Vector2 end)
{
    float x1 = start.x;
    float y1 = start.y;
    float x2 = end.x;
    float y2 = end.y;

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

    float xr1 = x1 * m->texture.get_width();
    float xr2 = x2 * m->texture.get_width();
    float delta = xr2 - xr1;

    int left_offset = (WINDOW_WIDTH/2) - (m->texture.get_width()/2);
    int top_offset = 200;
    
    for (float x = xr1; x < xr2; x += 1.0f) {
        float t = (x - xr1) / delta;
        float y = y1*(1.0f-t) + (y2*t);
        y = y * m->texture.get_height();

        u32 color = RGB_COLOR(50, 50, 50);

        if (steep) {
            set_pixel(y+left_offset, x+top_offset, color);
        } else {
            set_pixel(x+left_offset, y+top_offset, color);
        }
    }
}

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

    for (float x = xr1; x < xr2; x += 1.0f) {
        float t = (x - xr1) / delta;
        float y = y1*(1.0f-t) + (y2*t);
        y = scr_y(y);

        if (steep) {
            set_pixel(y, x, color);
        } else {
            set_pixel(x, y, color);
        }
    }
}

inline void draw_line(Vector3 start, Vector3 end, u32 color = RGB_COLOR(255, 255, 255))
{
    if (start.x > 1.0f || start.x < -1.0f || start.y > 1.0f || start.y < -1.0f || start.z < 0.1f || start.z > 1000.0f) {
        return;
    }

    if (end.x > 1.0f || end.x < -1.0f || end.y > 1.0f || end.y < -1.0f || end.z < 0.1f || end.z > 1000.0f) {
        return;
    }

    draw_line(start.x, start.y, end.x, end.y, start.z, end.z, color);
}

#define MAX3(_a, _b, _c) (max(_c, max(_a, _b)))
#define MIN3(_a, _b, _c) (min(_c, min(_a, _b)))

#define OLIVEC_SIGN(T, x) ((T)((x) > 0) - (T)((x) < 0))
inline bool olivec_barycentric(float x1, float y1, float x2, float y2, float x3, float y3, float xp, float yp, float *u1, float *u2, float *det)
{
    *det = ((x1 - x3)*(y2 - y3) - (x2 - x3)*(y1 - y3));
    *u1  = ((y2 - y3)*(xp - x3) + (x3 - x2)*(yp - y3));
    *u2  = ((y3 - y1)*(xp - x3) + (x1 - x3)*(yp - y3));
    float u3 = *det - *u1 - *u2;
    return (
               (OLIVEC_SIGN(float, *u1) == OLIVEC_SIGN(float, *det) || *u1 == 0) &&
               (OLIVEC_SIGN(float, *u2) == OLIVEC_SIGN(float, *det) || *u2 == 0) &&
               (OLIVEC_SIGN(float, u3) == OLIVEC_SIGN(float, *det) || u3 == 0)
           );
}

inline void draw_triangle(Vector3 v1, Vector3 v2, Vector3 v3, u32 color = RGB_COLOR(255, 255, 255))
{
    draw_line(v1, v2, color);
    draw_line(v2, v3, color);
    draw_line(v3, v1, color);
}

inline void rotate(Vector3 *v, float x, float y, float z)
{
    rotate_x(v, x);
    rotate_y(v, y);
    rotate_z(v, z);
}

inline void rotate(Vector3 *v, Model *m)
{
    rotate(v, m->rx, m->ry, m->rz);
}

inline void transform(Vector3 *v, float x, float y, float z)
{
    v->x += x;
    v->y += y;
    v->z += z;
}

inline void transform(Vector3 *v, Model *m)
{
    v->x += m->x;
    v->y += m->y;
    v->z += m->z;
}

inline Vector3 to_scr_coords(Vector3 v)
{
    Vector3 r = { scr_x(v.x), scr_y(v.y), v.z };
    return r;
}

Vector3 light_dir = {0.1, 0.1, -1};

inline void draw_mesh(Model *m)
{
    float x_min = -0.1;
    float y_min = -0.1;
    float x_max = 0.0f;
    float y_max = 0.0f;

    rotate_y(&light_dir, 0.005);
    rotate_x(&light_dir, -0.005);

    for (Face f : m->faces) {
        if (f.v1 >= m->vectors.count || f.v2 >= m->vectors.count || f.v3 >= m->vectors.count) return;
        Vector3 v1 = m->vectors[f.v1];
        Vector3 v2 = m->vectors[f.v2];
        Vector3 v3 = m->vectors[f.v3];
        
        // Rotations
        rotate(&v1, m);
        rotate(&v2, m);
        rotate(&v3, m);

        // Transform (offset)
        transform(&v1, m);
        transform(&v2, m);
        transform(&v3, m);
 
        Vector3 normal = vec_normal(v1, v2, v3);
        
        // Temporary back face culling
        constexpr const Vector3 view = {0, 0, -1};
        if (dot_product(normal, view) <= 0) {
            continue;
        }
        
        Vector3 varying_intensity;
        {
            Vector3 norm = m->normals[f.vn1];
            varying_intensity.m[0] = max(0.0f, dot_product(norm, light_dir));
            
            norm = m->normals[f.vn2];
            varying_intensity.m[1] = max(0.0f, dot_product(norm, light_dir));
            
            norm = m->normals[f.vn3];
            varying_intensity.m[2] = max(0.0f, dot_product(norm, light_dir));
        }
        
        // Projection
        // v1 = project(v1);
        // v2 = project(v2);
        // v3 = project(v3);
        
        // Scaling
        // @Todo: put the scaling here
        // draw_triangle(v1, v2, v3);
        
#if 1
        auto A = to_scr_coords(v1);
        auto B = to_scr_coords(v2);
        auto C = to_scr_coords(v3);

        int xmin = MIN3(A.x, B.x, C.x);
        int xmax = MAX3(A.x, B.x, C.x);
        if (xmin < 0)            xmin = 0;
        if (xmax > WINDOW_WIDTH) xmax = WINDOW_WIDTH;
        
        int ymin = MIN3(A.y, B.y, C.y);
        int ymax = MAX3(A.y, B.y, C.y);
        if (xmin < 0)             ymin = 0;
        if (xmax > WINDOW_HEIGHT) ymax = WINDOW_HEIGHT;

        // auto UV_A = m->uvs[f.vt1];
        // auto UV_B = m->uvs[f.vt2];
        // auto UV_C = m->uvs[f.vt3];

        // Matrix3 varying_uv = {
        //     UV_A.x, UV_A.y, 0,
        //     UV_B.x, UV_B.y, 0,
        //     UV_C.x, UV_C.y, 0,
        // };
        
        for (int x = xmin; x <= xmax; x++) {
            for (int y = ymin; y <= ymax; y++) {
                float u1, u2, det = 0;
                if (olivec_barycentric(A.x, A.y, B.x, B.y, C.x, C.y, x, y, &u1, &u2, &det)) {
                    float u3 = det - u1 - u2;
                    float z = 1/v1.z*u1/det + 1/v2.z*u2/det + 1/v3.z*u3/det;
                                        
                    Vector3 bar = {u1/det, u2/det, u3/det};
                    float intensity = dot_product(varying_intensity, bar);
                    if (intensity>.85) intensity = 1;
                    else if (intensity>.60) intensity = .80;
                    else if (intensity>.45) intensity = .60;
                    else if (intensity>.30) intensity = .45;
                    else if (intensity>.15) intensity = .30;
                    else intensity = 0;
                    
                    u32 color = RGB_COLOR(
                        static_cast<u8>(255 * intensity * z),
                        static_cast<u8>(255 * intensity * z),
                        static_cast<u8>(255 * intensity * z)
                    );
                    
                    if (z > get_zbuf(x, y)) {
                        set_pixel(x, y, color);
                        set_zbuf(x, y, z);
                    }
                }
            }
        }
    
#endif

    }
    
    // m->sx = x_min;
    // m->sw = x_max - x_min;
    // m->sy = y_min;
    // m->sh = y_max - y_min;
    
    // if (mouse_x >= x_min && mouse_x <= x_max 
    //     && abs(mouse_y-WINDOW_HEIGHT) >= m->sy && abs(mouse_y-WINDOW_HEIGHT) <= m->sy + m->sh) {    
    //     hovered_model = m;
    // }
    
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

            Array<Model *> models;
            
            // {
            //     Model *m = parse_obj_file(".\\obj\\teddy.obj");
            //     m->name = "teddy";
            //     m->ry = 3.0f;
            //     m->x = 1.8f;
            //     m->z = 2.5f;
            //     scale(m, 0.035f);
            //     m->r = 200; m->g = 100; m->b = 10;
                
            //     array_add(&models, m);
            // }
            
            // {
            //     Model *m = parse_obj_file(".\\obj\\teddy.obj");
            //     m->name = "teddy2";
            //     m->ry = 4.0f;
            //     m->x = 0.2f;
            //     m->y = -0.12f;
            //     m->z = 0.3f;
            //     scale(m, 0.010f);
            //     m->r = 50; m->g = 200; m->b = 80;
                
            //     array_add(&models, m);
            // }
            
            // {
            //     Model *m = parse_obj_file((".\\obj\\cow.obj"));
            //     m->name = "cow";
            //     m->x = -0.3f;
            //     m->z = 1.3f;
            //     scale(m, 0.12f);
            //     m->r = 50; m->g = 80; m->b = 80;
                
            //     array_add(&models, m);
            // }

            // {
            //     Model *m = parse_obj_file((".\\obj\\teapot.obj"));
            //     m->name = "teapot";
            //     m->x = 0.0f;
            //     m->y = -1.0f;
            //     m->z = 0.5f;
            //     scale(m, 0.27f);
            //     m->r = 30; m->g = 10; m->b = 255;
                
            //     array_add(&models, m);
            // }

            // {
            //     Model *m = parse_obj_file((".\\obj\\car_1924.obj"));
            //     m->name = "car_1924";
            //     m->x = 1.25f;
            //     m->y = -5.5f;
            //     m->z = 5.0;
            //     scale(m, 0.7f);
            //     m->r = 30; m->g = 10; m->b = 255;
            //     m->rx = -0.2f;
            //     // m->ry = -30.0f;
                
            //     array_add(&models, m);
            // }
            
            {
                Model *m = parse_obj_file((".\\obj\\boogie\\boogie.obj"));
                m->name = "boogie_body";
                m->x = 0.0f;
                m->y = 0.0f;
                m->z = 0.5;
                scale(m, 0.8f);
                m->r = 30; m->g = 10; m->b = 255;
                
                m->ry = 10.0f;
                
                array_add(&models, m);
                
                TGAImage uvtex(1024, 1024, TGAImage::RGB);
                assert(uvtex.read_tga_file("./obj/boogie/body_diffuse.tga"));
                uvtex.scale(1024/2, 1024/2);
                uvtex.flip_vertically();
                m->texture = uvtex;
            }
            
            {
                Model *m = parse_obj_file((".\\obj\\boogie\\head.obj"));
                m->name = "boogie_head";
                m->x = 0.0f;
                m->y = 0.0f;
                m->z = 0.5f;
                scale(m, 0.8f);
                m->r = 30; m->g = 10; m->b = 255;
                
                m->ry = 10.0f;
                
                array_add(&models, m);
                
                TGAImage uvtex(1024, 1024, TGAImage::RGB);
                assert(uvtex.read_tga_file("./obj/boogie/head_diffuse.tga"));
                uvtex.scale(1024/2, 1024/2);
                uvtex.flip_vertically();
                m->texture = uvtex;
            }
            
            // {
            //     Model *m = parse_obj_file((".\\obj\\boogie\\eyes.obj"));
            //     m->name = "boogie_eyes";
            //     m->x = 0.0f;
            //     m->y = 0.0f;
            //     m->z = 0.5f;
            //     // scale(m, 1.0f);
            //     m->r = 30; m->g = 10; m->b = 255;
                
            //     array_add(&models, m);
                
            //     TGAImage uvtex(1024, 1024, TGAImage::RGB);
            //     assert(uvtex.read_tga_file("./obj/boogie/eyes_diffuse.tga"));
            //     uvtex.scale(1024/2, 1024/2);
            //     uvtex.flip_vertically();
            //     m->texture = uvtex;
            // }
            
            For_Index (models) {
                Model *m = models[it_index];
                
                float xmax = 0.0f; float ymax = 0.0f; float zmax = 0.0f;
                float xmin = 0.0f; float ymin = 0.0f; float zmin = 0.0f;
    
                For (m->vectors) {
                    if (it->z > zmax) zmax = it->z;
                    else if (it->z < zmin) zmin = it->z;
                    
                    if (it->x > xmax) xmax = it->x;
                    else if (it->x < xmin) xmin = it->x;
                    
                    if (it->y > ymax) ymax = it->y;
                    else if (it->y < ymin) ymin = it->y;
                }
                
                m->w = xmax - xmin;
                m->h = ymax - ymin;
                m->z = map_z(zmin, zmin, zmax) + m->z;
                m->zh = zmax - zmin;
                CLOG1(CLOG_F(m->zh));
            }

            clog("{ vectors: %d }\n", models[0]->vectors.count);

            u64 cycle = 0;
            selected_model = nullptr;

            while (global_running) {
                mouse_events = 0;
                hovered_model = nullptr;
                
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

                For_Index (models) {
                    Model *m = models[it_index];
                    m->ry = (((float)mouse_x) / 500)*-1 + 10.0f;
                    
                    draw_mesh(m);
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