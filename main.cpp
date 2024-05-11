#include "rr3dcpp.h"

// #define WINDOW_WIDTH  800
// #define WINDOW_HEIGHT 800
int WINDOW_WIDTH  = 800;
int WINDOW_HEIGHT = 800;

Model *selected_model = nullptr;
Model *hovered_model = nullptr;

Camera cam = {0}; 

Vector3 light_dir = {0.1, 0.1, -1};
const float light_speed = 0.01f; // 0.005
float light_rot = 0.0f;

#define RGB_COLOR(r, g, b) (((r & 0xFF) << 16) | ((g & 0xFF) << 8) | ((b & 0xFF) << 0))

inline Vector3 func rgb_unpack(u32 rgb)
{
    return {
        static_cast<float>((rgb >> 16) & 0xFF),
        static_cast<float>((rgb >> 8) & 0xFF),
        static_cast<float>((rgb >> 0) & 0xFF)
    };
}

inline void func set_pixel(int x, int y, u32 color)
{
    if (x >= 0 && x < WINDOW_WIDTH && y >= 0 && y < WINDOW_HEIGHT) {
        *((static_cast<u32 *>(global_back_buffer.memory)) + (y * WINDOW_WIDTH + x)) = color;
    }
}

inline u32 func get_pixel(int x, int y)
{
    if (x >= 0 && x < WINDOW_WIDTH && y >= 0 && y < WINDOW_HEIGHT) {
        return *((static_cast<u32 *>(global_back_buffer.memory)) + (y * WINDOW_WIDTH + x));
    }
    
    return 0;
}

inline void func set_zbuf(int x, int y, float z)
{
    if (x >= 0 && x < WINDOW_WIDTH && y >= 0 && y < WINDOW_HEIGHT) {
        *((static_cast<float *>(global_back_buffer.zbuffer)) + (y * WINDOW_WIDTH + x)) = z;
    }
}

inline float func get_zbuf(int x, int y)
{
    if (x >= 0 && x < WINDOW_WIDTH && y >= 0 && y < WINDOW_HEIGHT) {
        return *((static_cast<float *>(global_back_buffer.zbuffer)) + (y * WINDOW_WIDTH + x));
    }
    return 0.0f;
}

inline float func scr_x(float x)
{
    return (x + 1.0) * WINDOW_WIDTH / 2;
}

inline float func scr_rev_x(float x)
{
    return x * 2 / WINDOW_WIDTH - 1.0;
}

inline float func scr_y(float y)
{
    return (y + 1.0) * WINDOW_WIDTH / 2;
}

inline float func scr_rev_y(float y)
{
    return y * 2 / WINDOW_WIDTH - 1.0;
}

inline Vector3 func project(Vector3 v)
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

inline void func transform_x(Model *m, float x)
{
    m->x += x;
}

inline void func transform_y(Model *m, float y)
{
    m->y += y;
}

inline void func transform_z(Model *m, float z)
{
    m->z += z;
}

inline float func map_z(float z, float zmin, float zmax)
{
    // return 1.0 - ((z - zmax) / (zmin - zmax)) + 1.0;
    return (((z - zmax) / (zmin - zmax)));
}

inline Vector3 func max_vec3(Vector3 a, Vector3 b) {
    Vector3 r = {
        max(a.x, b.x),
        max(a.y, b.y),
        max(a.z, b.z)
    };
    return r;
}

inline Vector3 func min_vec3(Vector3 a, Vector3 b) {
    Vector3 r = {
        min(a.x, b.x),
        min(a.y, b.y),
        min(a.z, b.z)
    };
    return r;
}

void func draw_2dline(Model *m, Vector2 start, Vector2 end)
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

void func draw_line(float x1, float y1, float x2, float y2, float z1, float z2, u32 color = RGB_COLOR(255, 255, 255), bool with_zbuf = false)
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
    
    if (z1 < z2) {
        swap(&z1, &z2);
    }
    float zdelta = z2 - z1;
    
    for (float x_scr = xr1; x_scr < xr2; x_scr += 1.0f) {
        float t = (x_scr - xr1) / delta;
        float y = y1*(1.0f-t) + (y2*t);
        auto y_scr = scr_y(y);

        if (with_zbuf) {
            t = (y - y1) / (y2 - y1);
            float z = z1*(1.0f-t) + (z2*t);
            
            auto zbp = get_zbuf(x_scr, y_scr);
            if (!zbp || (z > -1.0f && z > zbp)) {
                set_zbuf(x_scr, y_scr, z);
            } else {
                continue;
            }
        }
        
        if (steep) {
            set_pixel(y_scr, x_scr, color);
        } else {
            set_pixel(x_scr, y_scr, color);
        }
    }
}

inline void func draw_line(Vector3 start, Vector3 end, u32 color = RGB_COLOR(255, 255, 255), bool with_zbuf = false)
{
    draw_line(start.x, start.y, end.x, end.y, start.z, end.z, color, with_zbuf);
}

#define MAX3(_a, _b, _c) (max(_c, max(_a, _b)))
#define MIN3(_a, _b, _c) (min(_c, min(_a, _b)))

#define OLIVEC_SIGN(T, x) ((T)((x) > 0) - (T)((x) < 0))
inline bool func olivec_barycentric(float x1, float y1, float x2, float y2, float x3, float y3, float xp, float yp, float *u1, float *u2, float *det)
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

inline void func draw_triangle(Vector3 v1, Vector3 v2, Vector3 v3, u32 color = RGB_COLOR(255, 255, 255))
{
    draw_line(v1, v2, color);
    draw_line(v2, v3, color);
    draw_line(v3, v1, color);
}


inline void func transform(Vector3 *v, float x, float y, float z)
{
    v->x += x;
    v->y += y;
    v->z += z;
}

inline void func transform(Vector3 *v, Vector3 trans)
{
    // @Todo: Vector ptr overloads
    v->x += trans.x;
    v->y += trans.y;
    v->z += trans.z;
}

inline void func transform(Vector3 *v, Model *m)
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

void func fill_triangle(Vector3 v1, Vector3 v2, Vector3 v3, u32 color, Vector3 *algv = nullptr)
{
    Vector3 normal = vec_normal(v1, v2, v3);
    
    // constexpr const Vector3 view = {0, 0, -1};
    // if (dot_product(normal, view) <= 0) {
    //     continue;
    // }
    
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
    
    for (int x = xmin; x <= xmax; x++) {
        for (int y = ymin; y <= ymax; y++) {
            float u1, u2, det = 0;
            if (olivec_barycentric(A.x, A.y, B.x, B.y, C.x, C.y, x, y, &u1, &u2, &det)) {
                float u3 = det - u1 - u2;
                // z index triangle interpolation - therefore the det is int we have to divide with it
                float z = 1/v1.z*u1/det + 1/v2.z*u2/det + 1/v3.z*u3/det;
                
                if (z < 0.1) continue; // near clipping plane
                                    
                Vector3 bar = {u1/det, u2/det, u3/det};
                float intensity = algv ? dot_product(*algv, bar) : .45;
                if (intensity>.85) intensity = 1;
                else if (intensity>.60) intensity = .80;
                else if (intensity>.45) intensity = .60;
                else if (intensity>.30) intensity = .45;
                else if (intensity>.15) intensity = .30;
                else intensity = 0;

                auto vc = rgb_unpack(color);
                u32 color = RGB_COLOR(
                    (u32)(vc.m[0] * intensity * z),
                    (u32)(vc.m[1] * intensity * z),
                    (u32)(vc.m[2] * intensity * z)
                );
                
                if (z > get_zbuf(x, y)) {
                    set_pixel(x, y, color);
                    set_zbuf(x, y, z);
                }
            }
        }
    }
}


void func draw_mesh(Model *m)
{
    // for mouse hover check
    float x_min = -0.1;
    float y_min = -0.1;
    float x_max = 0.0f;
    float y_max = 0.0f;

    int normals_count = 0;

    for (Face f : m->faces) {
        if (f.v1 >= m->vectors.count || f.v2 >= m->vectors.count || f.v3 >= m->vectors.count) return;
        Vector3 v1 = m->vectors[f.v1];
        Vector3 v2 = m->vectors[f.v2];
        Vector3 v3 = m->vectors[f.v3];

        if (m->name == "teddy") {
            rotate_y(&v1, light_rot);
            rotate_x(&v1, -light_rot);
            rotate_y(&v2, light_rot);
            rotate_x(&v2, -light_rot);
            rotate_y(&v3, light_rot);
            rotate_x(&v3, -light_rot);
        }

        rotate(&v1, cam.rot);
        rotate(&v2, cam.rot);
        rotate(&v3, cam.rot);

        // LOCAL
        scale(&v1, m->scale);
        scale(&v2, m->scale);
        scale(&v3, m->scale);
        
        rotate(&v1, m);
        rotate(&v2, m);
        rotate(&v3, m);
        
        transform(&v1, m);
        transform(&v2, m);
        transform(&v3, m);

        // CAMERA
        v1 = (v1 + cam.pos) * cam.zoom;
        v2 = (v2 + cam.pos) * cam.zoom;
        v3 = (v3 + cam.pos) * cam.zoom;
        
        Vector3 normal = vec_normal(v1, v2, v3);
        normals_count++;

        if (m->show_normals && normals_count % m->normals_intensity == 0) {
            //float i = max(0.0f, dot_product(normal, {-light_dir.x, -light_dir.y, -light_dir.z}));
            float i = max(0.0f, dot_product(normal, {light_dir.x, light_dir.y, light_dir.z}));
            auto r = ((u32)(m->r*i));
            auto g = ((u32)(m->g*i));
            auto b = ((u32)(m->b*i));
            u32 color = RGB_COLOR(
                clamp<u32>( 0, 255, r ),
                clamp<u32>( 0, 255, g ),
                clamp<u32>( 0, 255, b )
            );
            // fill_triangle(v1, v2, v3, color);
            draw_line(v1, normal*0.5f, color, false);
        }

        // Temporary back face culling
        constexpr const Vector3 view = {0.0f, 0, -1.0f};
        if (dot_product(normal, view) <= 0) {
            continue;
        }

        Vector3 varying_intensity = {0};
        if (m->normals.count) {
            Vector3 norm = m->normals[f.vn1];
            rotate(&norm, m); // only if the obj is rotating
            varying_intensity.m[0] = max(0.0f, dot_product(norm, light_dir));

            norm = m->normals[f.vn2];
            rotate(&norm, m); // only if the obj is rotating
            varying_intensity.m[1] = max(0.0f, dot_product(norm, light_dir));

            norm = m->normals[f.vn3];
            rotate(&norm, m); // only if the obj is rotating
            varying_intensity.m[2] = max(0.0f, dot_product(norm, light_dir));
        }
        else {
            auto i = max(0.0f, dot_product(normal, light_dir));
            varying_intensity = { i, i, i };
        }
        
        // Projection
        // v1 = project(v1);
        // v2 = project(v2);
        // v3 = project(v3);
        
        // Scaling
        // @Todo: put the scaling here
        // draw_triangle(v1, v2, v3);
        
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

        // for mouse hover check
        if (xmax > x_max) x_max = xmax;
        if (xmin < x_min || x_min < 0.0f) x_min = xmin; 
        if (ymax > y_max) y_max = ymax;
        if (ymin < y_min || y_min < 0.0f) y_min = ymin; 

        for (int x = xmin; x <= xmax; x++) {
            for (int y = ymin; y <= ymax; y++) {
                float u1, u2, det = 0;
                if (olivec_barycentric(A.x, A.y, B.x, B.y, C.x, C.y, x, y, &u1, &u2, &det)) {
                    float u3 = det - u1 - u2;
                    float z = 1/v1.z*u1/det + 1/v2.z*u2/det + 1/v3.z*u3/det;
                    
                    Vector3 bar = {u1/det, u2/det, u3/det};
                    
                    float intensity = dot_product(varying_intensity, bar);
                    // if (intensity>.85) intensity = 1;
                    // else if (intensity>.60) intensity = .80;
                    // else if (intensity>.45) intensity = .60;
                    // else if (intensity>.30) intensity = .45;
                    // else if (intensity>.15) intensity = .30;
                    // else intensity = 0;

                    u32 r = clamp<u32>( 5, 255, static_cast<u8>(255 * intensity * z) );
                    u32 g = clamp<u32>( 5, 255, static_cast<u8>(255 * intensity * z) );
                    u32 b = clamp<u32>( 5, 255, static_cast<u8>(255 * intensity * z) );
                    u32 color = RGB_COLOR(r, g, b);
                    
                    if (z > get_zbuf(x, y)) {
                        set_pixel(x, y, color);
                        set_zbuf(x, y, z);
                    }
                }
            }
        }

        // if (normals_count % 60 == 0) {
        //     float i = max(0.0f, dot_product(normal, {0, 0, -1}));
        //     auto r = ((u32)(0*i));
        //     auto g = ((u32)(50*i));
        //     auto b = ((u32)(50*i));
        //     u32 color = RGB_COLOR(
        //         clamp<u32>( 0, 255, r ),
        //         clamp<u32>( 0, 255, g ),
        //         clamp<u32>( 0, 255, b )
        //     );
        //     fill_triangle(v1, v2, v3, color);
        //     draw_line(v1, normal, color, false);
        // }

    }
    
    // for mouse hover check
    m->sx = x_min;
    m->sw = x_max - x_min;
    m->sy = y_min;
    m->sh = y_max - y_min;
    
    // @Incomplete: Z-index check?
    if (mouse_x >= x_min && mouse_x <= x_max 
        && abs(mouse_y-WINDOW_HEIGHT) >= m->sy && abs(mouse_y-WINDOW_HEIGHT) <= m->sy + m->sh) {    
        hovered_model = m;
    }
    
}

void func draw_vec_head(Vector3 s, Vector3 e, u32 color = RGB(0,150,150), float rr = 0.0f)
{
    // Vector3 b = {e.x, e.y-0.05f, e.z};
    // draw_line(s, e, color);
    Vector3 c = normalize({-e.x, e.y, e.z});
    Vector3 k = normalize({e.x, -e.y, e.z});
    auto i = normalize(substract(c, e));
    // draw_triangle(s, e, c, color);
    // draw_line(e, c, RGB(90, 255, 0));
    // draw_line(e, k, RGB(255, 120, 0));
    auto en = normalize(e);
    // draw_line(en, e, RGB(0, 0, 255));
    auto sn = normalize(e);
    // draw_line(sn, s, RGB(0, 255, 0));

    fill_triangle(s, e, sn, color);
    
    auto cr = normalize(cross_product(s, e));
    draw_line(sn, cr, RGB(255, 255, 0));
    
    rotate_x(&en, rr);
    rotate_x(&sn, rr);
    rotate_y(&en, -rr);
    rotate_y(&sn, -rr);
    // draw_triangle(e, b, c, color);
//     fill_triangle(e, b, c, color);
}

int CALLBACK
WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR command_line, int show_code)
{ 
    assert(AllocConsole()); // @temporary
    Win32LoadXInput();

    std::srand(time(0));

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
                m->x = -1.0f;
                m->y = 0.0f;
                m->z = 0.5;
                m->scale = 1.0f;
                // scale(m, 0.8f);
                m->r = 25; m->g = 25; m->b = 200;
                
                m->ry = 10.0f;
                
                array_add(&models, m);
                
                TGAImage uvtex(1024, 1024, TGAImage::RGB);
                assert(uvtex.read_tga_file("./obj/boogie/body_diffuse.tga"));
                uvtex.scale(1024/2, 1024/2);
                uvtex.flip_vertically();
                m->texture = uvtex;
                
                m->show_normals = false;
            }
            
            {
                Model *m = parse_obj_file((".\\obj\\boogie\\head.obj"));
                m->name = "boogie_head";
                m->x = -1.0f;
                m->y = 0.0f;
                m->z = 0.5f;
                m->scale = 1.0f;
                m->r = 25; m->g = 25; m->b = 200;
                
                m->ry = 10.0f;
                
                array_add(&models, m);
                
                TGAImage uvtex(1024, 1024, TGAImage::RGB);
                assert(uvtex.read_tga_file("./obj/boogie/head_diffuse.tga"));
                uvtex.scale(1024/2, 1024/2);
                uvtex.flip_vertically();
                m->texture = uvtex;
                
                m->show_normals = false;
            }
            
            {
                Model *m = parse_obj_file((".\\obj\\cow.obj"));
                m->name = "cow";
                m->x = 0.50000f;
                m->y = -0.20000f;
                m->z = 2.500000f;
                m->scale = 0.15f;
                m->r = 200; m->g = 150; m->b = 0;
                m->ry = -4.5f;
                
                array_add(&models, m);
            }
            
            {
                Model *m = parse_obj_file((".\\obj\\lamp.obj"));
                m->name = "lamp1";
                m->x = -0.2000f;
                m->y = 0.000000f;
                m->z = 2.500000f;
                m->scale = 0.1f;
                m->r = 10; m->g = 150; m->b = 100;
                m->ry = 10.0f;
                
                array_add(&models, m);
            }
            
            {
                Model *m = parse_obj_file((".\\obj\\teddy.obj"));
                m->name = "teddy";
                m->x = 1.50000f;
                m->y = 1.300000f;
                m->z = 2.500000f;
                m->scale = 0.015f;
                m->r = 10; m->g = 150; m->b = 100;
                m->ry = 10.0f;
                
                array_add(&models, m);
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

            TGAImage bulb_tga(1024, 1024, TGAImage::RGB);
            assert(bulb_tga.read_tga_file("./img/bulb.tga"));
            bulb_tga.scale(404, 617);
            bulb_tga.flip_vertically();

            auto rr = 0.0;

            Vector3 start = {0, 0.0f, 1.0f};
            Vector3 end = {-0.2f, 0.2f, 1.0f};
            scale(&start, 0.2);
            scale(&end, 0.2);

            cam.pos  = {0, 0, 1.0f};
            cam.zoom = 0.5;

            // Start main loop
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

                // rotate_x(&end, -rr);
                // rotate_x(&start, -rr);
                // // rotate_y(&end, -rr);
                // // rotate_y(&start, -rr);
                
                // rr += 0.00000001;
                
                // draw_vec_head(start, end, RGB(0,150,150), rr);
                
                // draw_line(start, end1);
                // draw_line(start, end2, RGB_COLOR(200, 100, 0));

                // CLOG1(CLOG_D(vk_key_pressed));
                
                rotate_y(&light_dir, light_speed);
                rotate_x(&light_dir, -light_speed);
                light_rot += light_speed;
                
                if (vk_key_pressed == 37) { // arrow left
                    if (vk_alt_was_down) {
                        cam.rot.y += 0.05f;
                    } else {
                        cam.pos.x -= 0.01f;
                    }
                }
                if (vk_key_pressed == 39) { // arrow right
                    if (vk_alt_was_down) {
                        cam.rot.y -= 0.05f;
                    } else {
                        cam.pos.x += 0.01f;
                    }
                }
                if (vk_key_pressed == 40) { // arrow down
                    if (vk_alt_was_down) {
                        cam.zoom += 0.01f;
                    } else {
                        cam.pos.y -= 0.01f;
                    }
                }
                if (vk_key_pressed == 38) { // arrow up
                    if (vk_alt_was_down) {
                        cam.zoom -= 0.01f;
                    } else {
                        cam.pos.y += 0.01f;
                    }
                }
                
                For_Index (models) {
                    Model *m = models[it_index];
                    
                    if (vk_key_pressed == 71) { // g
                        CLOG_START();
                            CLOG_S(m->name);
                            CLOG_F(m->x);
                            CLOG_F(m->y);
                            CLOG_F(m->z);
                            CLOG_F(m->scale);
                        CLOG_END();
                    }
                    
                    // m->ry = (((float)mouse_x) / 500)*-1 + 10.0f;
                    // m->ry += 0.01;
                    
                    // if (m->name == "teddy") {
                    //     m->x = light_dir.x;
                    //     m->y = light_dir.y;
                    //     m->z = 2.0f;
                    // }
                    
                    draw_mesh(m);
                }

                // (CLOG_D(mouse_x));
                // clog("\n");
                // (CLOG_D(mouse_y));
                // clog("\n");

                if (mouse_events & MOUSE_DOWN) {
                    if (hovered_model) {
                        selected_model = hovered_model;
                    } else {
                        selected_model = nullptr;
                    }
                }
                
                if (selected_model) {
                    // auto m = selected_model;
                    // Vector3 mouse = { scr_rev_x(mouse_x), scr_rev_y(abs(mouse_y-WINDOW_HEIGHT)), m->z };
                    // m->x = mouse.x * m->z;
                    // m->y = mouse.y * m->z;
                }
                
                if (hovered_model) {
                    Model *m = hovered_model;
                    u32 border_color = RGB_COLOR(255, 255, 255);

                    float m_cy = (m->sy + m->sh / 2);
                    float m_cx = (m->sx + m->sw / 2);

                    // Selected entity x, y axis
                    for (float x = m_cx; x <= m->sx + m->sw; x++) {
                        set_pixel(x, m_cy, RGB_COLOR(255, 80, 80));
                    }
                    for (float y = m_cy; y <= m->sy + m->sh; y++) {
                        set_pixel(m_cx, y, RGB_COLOR(80, 255, 80));
                    }
                    
                    // Selected entity bounding box with 10px padding
                    for (float x = m->sx-10; x <= m->sx + m->sw + 10; x++) {
                        set_pixel(x, m->sy + m->sh + 10, border_color);  // top
                        set_pixel(x, m->sy - 10, border_color);          // bottom
                    }
                    for (float y = m->sy - 10; y <= m->sy + m->sh + 10; y++) {
                        set_pixel(m->sx - 10, y, border_color);          // left
                        set_pixel(m->sx + m->sw + 10, y, border_color);  // right
                    }                   
                }
                
                if (selected_model) {
                    Model *m = selected_model;
                    u32 border_color = RGB_COLOR(200, 21, 21);

                    float m_cy = (m->sy + m->sh / 2);
                    float m_cx = (m->sx + m->sw / 2);

                    // Selected entity x, y axis
                    for (float x = m_cx; x <= m->sx + m->sw; x++) {
                        set_pixel(x, m_cy, RGB_COLOR(255, 80, 80));
                    }
                    for (float y = m_cy; y <= m->sy + m->sh; y++) {
                        set_pixel(m_cx, y, RGB_COLOR(80, 255, 80));
                    }
                    
                    // Selected entity bounding box with 10px padding
                    for (float x = m->sx-10; x <= m->sx + m->sw + 10; x++) {
                        set_pixel(x, m->sy + m->sh + 10, border_color);  // top
                        set_pixel(x, m->sy - 10, border_color);          // bottom
                    }
                    for (float y = m->sy - 10; y <= m->sy + m->sh + 10; y++) {
                        set_pixel(m->sx - 10, y, border_color);          // left
                        set_pixel(m->sx + m->sw + 10, y, border_color);  // right
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