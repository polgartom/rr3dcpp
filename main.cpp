#include "rr3dcpp.h"

// #define WINDOW_WIDTH  800
// #define WINDOW_HEIGHT 800
int WINDOW_WIDTH  = 800;
int WINDOW_HEIGHT = 800;

Model *selected_model = nullptr;
Model *hovered_model = nullptr;

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
inline bool olivec_barycentric(int x1, int y1, int x2, int y2, int x3, int y3, int xp, int yp, int *u1, int *u2, int *det)
{
    *det = ((x1 - x3)*(y2 - y3) - (x2 - x3)*(y1 - y3));
    *u1  = ((y2 - y3)*(xp - x3) + (x3 - x2)*(yp - y3));
    *u2  = ((y3 - y1)*(xp - x3) + (x1 - x3)*(yp - y3));
    int u3 = *det - *u1 - *u2;
    return (
               (OLIVEC_SIGN(int, *u1) == OLIVEC_SIGN(int, *det) || *u1 == 0) &&
               (OLIVEC_SIGN(int, *u2) == OLIVEC_SIGN(int, *det) || *u2 == 0) &&
               (OLIVEC_SIGN(int, u3) == OLIVEC_SIGN(int, *det) || u3 == 0)
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

inline Vector3 calc_normal(Vector3 v1, Vector3 v2, Vector3 v3)
{
    Vector3 normal, line1, line2 = {0};
    line1.x = v2.x - v1.x;
    line1.y = v2.y - v1.y;
    line1.z = v2.z - v1.z;
    
    line2.x = v3.x - v1.x;
    line2.y = v3.y - v1.y;
    line2.z = v3.z - v1.z;
    
    normal.x = line1.y * line2.z - line1.z * line2.y;
    normal.y = line1.z * line2.x - line1.x * line2.z;
    normal.z = line1.x * line2.y - line1.y * line2.x;

    float len = sqrtf(pow(normal.x, 2) + pow(normal.y, 2) + pow(normal.z, 2));
    normal.x /= len;
    normal.y /= len;
    normal.z /= len;
    
    return normal;
}

inline Vector3 to_scr_coords(Vector3 v)
{
    Vector3 r = { scr_x(v.x), scr_y(v.y), v.z };
    return r;
}

inline void draw_mesh(Model *m)
{
    float x_min = -0.1;
    float y_min = -0.1;
    float x_max = 0.0f;
    float y_max = 0.0f;

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
 
        Vector3 normal = calc_normal(v1, v2, v3);
        if (normal.x * (v1.x - 0) +
            normal.y * (v1.y - 0) +
            normal.z * (v1.z - 0) > 0.0f /* or < 0.0f, it depends */) {
            continue;
        }
        
        // Illumination
        // Vector3 light_dir = {0.0f, 0.0f, -1.0f};
        // float l = sqrtf(pow(light_dir.x, 2) + pow(light_dir.y, 2) + pow(light_dir.z, 2));
        // light_dir.x /= l;
        // light_dir.y /= l;
        // light_dir.z /= l;
        
        // float dp = normal.x * (light_dir.x) +
        //     normal.y * (light_dir.y) +
        //     normal.z * (light_dir.z);
                
        // u8 r = m->r * (normal.z/normal.y);
        u8 r = m->r * normal.x;
        u8 g = m->g * 1;
        u8 b = m->b * normal.z;
        u32 color = RGB_COLOR(r, g, b);
                
        // Projection
        v1 = project(v1);
        v2 = project(v2);
        v3 = project(v3);
        
        // Scaling
        // @Todo: put the scaling here
        // draw_triangle(v1, v2, v3);
        
#if 1
        v1 = to_scr_coords(v1);
        v2 = to_scr_coords(v2);
        v3 = to_scr_coords(v3);
    
        float xmax = MAX3(v1.x, v2.x, v3.x);
        float xmin = MIN3(v1.x, v2.x, v3.x);
        float ymax = MAX3(v1.y, v2.y, v3.y);
        float ymin = MIN3(v1.y, v2.y, v3.y);
        // float zmax = MAX3(v1.z, v2.z, v3.z);
        // float zmin = MIN3(v1.z, v2.z, v3.z);
        
        if (xmax > x_max) x_max = xmax;
        if (xmin < x_min || x_min < 0.0f) x_min = xmin; 
        if (ymax > y_max) y_max = ymax;
        if (ymin < y_min || y_min < 0.0f) y_min = ymin; 
        
        Vector2 vt1_ = m->uvs[f.vt1];
        Vector2 vt2_ = m->uvs[f.vt2];
        Vector2 vt3_ = m->uvs[f.vt3];
        Vector3 vt1 = {vt1_.x, vt1_.y, 1};
        Vector3 vt2 = {vt2_.x, vt2_.y, 1};
        Vector3 vt3 = {vt3_.x, vt3_.y, 1};
        
        float uv_xmax = MAX3(vt1.x, vt2.x, vt3.x);
        float uv_xmin = MIN3(vt1.x, vt2.x, vt3.x);
        float uv_ymax = MAX3(vt1.y, vt2.y, vt3.y);
        float uv_ymin = MIN3(vt1.y, vt2.y, vt3.y);
        
        for (float x = xmin; x <= xmax; x++) {
            for (float y = ymin; y <= ymax; y++) {
                int u1, u2, det = 0;
                if (olivec_barycentric(v1.x, v1.y, v2.x, v2.y, v3.x, v3.y, x, y, &u1, &u2, &det)) {               
                    int u3 = det - u1 - u2;
                    float z = 1/v1.z*u1/det + 1/v2.z*u2/det + 1/v3.z*u3/det;
                    if (z > get_zbuf(x, y)) {
                        // u8 c = 255 * (z);
                        // u32 color = RGB_COLOR(c, c, c);
                        float uv_x = (uv_xmax * (x / xmax));
                        float uv_y = (uv_ymax * (y / ymax));
                        if (olivec_barycentric(vt1.x, vt1.y, vt2.x, vt2.y, vt3.x, vt3.y, uv_x, uv_y, &u1, &u2, &det)) {
                            color = m->texture.get(uv_x*m->texture.get_width(), uv_y*m->texture.get_height()).val;
                            // u8 r = ((color>>16)&0xFF) * z;
                            // u8 g = ((color>>8)&0xFF) * z;
                            // u8 b = ((color>>0)&0xFF) * z;
                            // color = (r<<16) | (g<<8) | (b<<0);
                        }
                        
                        set_pixel(x, y, color);
                        set_zbuf(x, y, z);
                    }
                }
            }
        }
#endif
   
    }
    
    m->sx = x_min;
    m->sw = x_max - x_min;
    m->sy = y_min;
    m->sh = y_max - y_min;
    
    if (mouse_x >= x_min && mouse_x <= x_max 
        && abs(mouse_y-WINDOW_HEIGHT) >= m->sy && abs(mouse_y-WINDOW_HEIGHT) <= m->sy + m->sh) {    
        hovered_model = m;
    }
    
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

            {
                Model *m = parse_obj_file((".\\obj\\car_1924.obj"));
                m->name = "car_1924";
                m->x = 1.25f;
                m->y = -5.5f;
                m->z = 5.0;
                scale(m, 0.7f);
                m->r = 30; m->g = 10; m->b = 255;
                m->rx = -0.2f;
                // m->ry = -30.0f;
                
                array_add(&models, m);
            }

            TGAImage uvtex(1024, 1024, TGAImage::RGB);
            assert(uvtex.read_tga_file(".//uv_sample_texture.tga"));
            uvtex.scale(1024/2, 1024/2);
            // uvtex.flip_horizontally();
            uvtex.flip_vertically();
            
            For_Index (models) {
                Model *m = models[it_index];
                m->texture = uvtex;
                
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

            CLOG_START();
                clog("UV Sample: ");
                CLOG_D(uvtex.get_width());
                CLOG_D(uvtex.get_height());
                CLOG_D(uvtex.get_bytespp());
            CLOG_END();

            u64 cycle = 0;
            selected_model = nullptr;
            while (global_running) {
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

                // clog("m: x: %d ; y: %d\n", mouse_x, mouse_y);
                
                int w = uvtex.get_width();
                int h = uvtex.get_height();
                int x1 = (WINDOW_WIDTH/2) - (w/2);
                int y1 = 200;
                // int x1 = 0;
                // int y1 = 0;
                for (int x = 0; x < w; x++) {
                    for (int y = 0; y < h; y++) {
                        u32 p = uvtex.get(x, y).val;
                        set_pixel(x+x1, y+y1, p);
                        if (x == 0 || x == w-1) {
                            set_pixel(x+x1, y+y1, RGB_COLOR(255, 0, 80));
                        }
                    }
                }

                For_Index (models) {
                    Model *m = models[it_index];
                    
                    for (Face f : m->faces) {
                        Vector2 vt1 = m->uvs[f.vt1];
                        Vector2 vt2 = m->uvs[f.vt2];
                        Vector2 vt3 = m->uvs[f.vt3];
                        
                        // CLOG_VEC2(vt1);
                        // CLOG_VEC2(vt2);
                        // CLOG_VEC2(vt3);
                        
                        draw_2dline(m, vt1, vt2);
                        draw_2dline(m, vt2, vt3);
                        draw_2dline(m, vt3, vt1);
                    }
                    
                    // if (m->name == "teddy") {
                    //     rotate_y(m, 0.01);
                    //     rotate_x(m, 0.3);
                    // } else if (m->name == "cow") {
                    //     rotate_y(m, -0.05);
                    // } else if (!(m->name == "building")) {
                    //     rotate_y(m, -0.01);
                    // }
                    
                    draw_mesh(m);
                }
                
                // CLOG_START();
                //     CLOG_F(scr_rev_x(mouse_x));
                //     CLOG_F(scr_rev_y(abs(mouse_y-WINDOW_HEIGHT)));
                //     CLOG_D(mouse_left_down);
                // CLOG_END();
                
                if (mouse_left_down) {
                    if (selected_model) selected_model = nullptr;
                    else selected_model = hovered_model;
                }
                
                if (selected_model) {
                    auto m = selected_model;
                    Vector3 mouse = { scr_rev_x(mouse_x), scr_rev_y(abs(mouse_y-WINDOW_HEIGHT)), m->z };
                    m->x = mouse.x * m->z;
                    m->y = mouse.y * m->z;

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
                    
                    // CLOG_START();
                    //     CLOG_S(m->name);
                    //     CLOG_F(m->sx);
                    //     CLOG_F(m->sw);
                    //     CLOG_F(m->sy);
                    //     CLOG_F(m->sh);
                    // CLOG_END();
                }
                
                if (hovered_model) {
                    auto m = hovered_model;
                    
                    float m_cy = (m->sy + m->sh / 2);
                    float m_cx = (m->sx + m->sw / 2);

                    for (float x = m_cx; x <= m->sx + m->sw; x++) {
                        set_pixel(x, m_cy, RGB_COLOR(255, 80, 80));
                    }
                    
                    for (float y = m_cy; y <= m->sy + m->sh; y++) {
                        set_pixel(m_cx, y, RGB_COLOR(80, 255, 80));
                    }
                    
                    for (float x = m->sx-10; x <= m->sx + m->sw + 10; x++) {
                        set_pixel(x, m->sy + m->sh + 10, RGB_COLOR(255, 255, 255)); // top
                        set_pixel(x, m->sy - 10, RGB_COLOR(255, 255, 255)); // bottom
                    }
                    
                    for (float y = m->sy - 10; y <= m->sy + m->sh + 10; y++) {
                        set_pixel(m->sx - 10, y, RGB_COLOR(200, 200, 200));
                        set_pixel(m->sx + m->sw + 10, y, RGB_COLOR(200, 200, 200));
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