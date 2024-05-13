#ifndef H_RR3DCPP
#define H_RR3DCPP

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>

#include "tgaimage.h"

typedef char s8;
typedef short s16;
typedef int s32;
typedef long s64;

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long u64;

#define COLOR_DEFAULT "\033[0m"
#define COLOR_RED "\033[0;31m"
#define COLOR_GREEN "\033[0;32m"
#define COLOR_YELLOW "\033[0;33m"
#define COLOR_BLUE "\033[0;34m"
#define COLOR_PURPLE "\033[0;35m"
#define COLOR_CYAN "\033[0;36m"

void clog(const char *__fmt_msg, ...);

#define ASSERT(__cond, __fmt_msg, ...) { \
    if (!(__cond)) { \
        clog(__fmt_msg"\n", ##__VA_ARGS__); \
        assert(__cond); \
    } \
}

#define ZERO_MEMORY(dest, len) (memset(((u8 *)dest), 0, (len)))
#define NEW(_type) ((_type *)malloc(sizeof(_type)))

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr)[0])
// #define CSTR_EQUAL(str1, str2) (strcmp(str1, str2) == 0)
#define CSTR_LEN(x) (x != NULL ? strlen(x) : 0)
#define XSTR(x) #x
#define STRUCT_COPY(dest, src) (memcpy(&dest, &src, sizeof(dest)))

#define func 

#define M_PI 3.14159265358979323846

#include <string>
#include "new_string.h"
#include "array.h"
#include "window.h"
#include "vector.h"
#include <ctime>
#include <cstdlib>
#include <iostream>

char *func get_project_dir_cstr() 
{
    static char buffer[MAX_PATH] = {0};
    GetModuleFileName( NULL, buffer, MAX_PATH );
    auto b = std::string(buffer);
    auto pos = b.find_last_of( "\\/" );
    assert(pos != std::string::npos);
    buffer[pos] = '\0';
    return buffer;
}

template <typename T>
T func clamp(T min, T max, T value)
{
    if      (value < min) return min;
    else if (value > max) return max;
    return value;
}

float func lerp(float a, float b, float f)
{
    return a * (1.0 - f) + (b * f);
}

struct Matrix3 {
    float _00, _01, _02,
          _10, _11, _12,
          _20, _21, _22 = 0.0f;
};

struct Matrix4 {
    float _00, _01, _02, _03,
          _10, _11, _12, _13,
          _20, _21, _22, _23,
          _30, _31, _32, _33 = 0.0f;    
};

struct Face {
    int v1, v2, v3;
    int vt1, vt2, vt3;
    int vn1, vn2, vn3;
};

struct Model {
    String name;

    u8 r, g, b = 254;

    float x, y, z = 0.0f;
    float w, h, zh = 0.0f;
    
    float scale = 1.0f;
    float rx = 0.0f;
    float ry = 0.0f;
    float rz = 0.0f;
    
    float zfar  = 0.0f;
    float znear = 0.0f;
    
    float sx, sy, sw, sh = 0.0f;
    
    Array<Vector3>  vectors;
    Array<Vector2>  uvs;
    Array<Vector3>  normals;
    Array<Face>     faces;
    
    TGAImage texture;
    
    // DEBUG
    bool show_normals = false;
    int  normals_intensity = 60; // show every 60th normal
    
    void func recalc_bounds() {
        auto m = this;
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
        
        // CLOG1((
        //     CLOG_S(m->name),
        //     CLOG_F(m->z),
        //     CLOG_F(zmin),
        //     CLOG_F(zmax)
        // ));
        
        m->w = xmax - xmin;
        m->h = ymax - ymin;
        m->z = ( (zmin - zmax) / (zmin - zmax) ) + m->z;
        m->zh = zmax - zmin;
        
        m->zfar = zmax;
        m->znear = zmin;
        // CLOG1(CLOG_F(m->z));
    }
    
    
};

struct Camera {
    Vector3 pos = {0};
    Vector3 rot = {0};    // rotation
    Vector3 dir = {0};
    
    float   zoom = 1.0f;
};

String func read_entire_file(char *filename)
{
    FILE *fp = fopen(filename, "rb");
    ASSERT(fp, "Failed to open file! Filename: %s\n", filename);
    
    fseek(fp, 0, SEEK_END);
    u32 fsize = ftell(fp);
    rewind(fp);
    
    String s = string_make_alloc(fsize+1); // +1, because we'll insert a line break to deal with the EOF easier
    s.data[s.count-1] = '\n';
    
    fread(s.data, fsize, 1, fp);
    fclose(fp);
    
    return s;
}

void clog(const char *__fmt_msg, ...)
{
    va_list args;
    static char buf[4096] = {0};
    
    va_start(args, __fmt_msg);
    _vstprintf_s(buf, __fmt_msg, args);
    va_end(args);
    
    buf[4096-1] = 0;
    
    WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), buf, strlen(buf), NULL, NULL);
}

inline void func swap(float *a, float *b)
{
    float t = *a;
    *a = *b;
    *b = t;
}

#define CLOG_VEC2(_v) clog("{ x: %f ; y: %f }\n", (_v).x, (_v).y)
#define CLOG_VEC3(_v) clog("{ x: %f ; y: %f ; z: %f }\n", (_v).x, (_v).y, (_v).z)

#define CLOG_START()  clog("{ ")
#define CLOG_F(_f)    clog(XSTR(_f) ": %f ; ", _f)
#define CLOG_D(_d)    clog(XSTR(_d) ": %d ; ", _d)
#define CLOG_LD(_ld)  clog(XSTR(_ld) ": %ld ; ", _ld)
#define CLOG_CS(_cs)  clog(XSTR(_cs) ": %s ; ", _cs)
// new_string.h
#define CLOG_S(_s)    clog(XSTR(_s) ": " SFMT " ; ", SARG(_s))
#define CLOG_END()    clog(" }\n")
#define CLOG1(_CLOG)  { CLOG_START(); _CLOG; CLOG_END(); }

#include "math.h"

Model func *parse_obj_file(String obj_filename) 
{
    String obj = read_entire_file(obj_filename.data);
    
    Model *m = new Model();

    float r = 0.0f;
    int ri = 0;
    bool s = true;
    String rem = obj;

    int vc = 0;
    
    while (obj.count-1) {
        if (SCHAR(obj) == '#') {
            obj = string_eat_until(obj, '\n');
            advance(&obj, 1);
        }
        if (SCHAR(obj) == 's') {
            // @Incomplete
            obj = string_eat_until(obj, '\n');
            advance(&obj, 1);
        }
        
        if (SCHAR(obj) == 'v') {        
            advance(&obj, 1);
            
            if (SCHAR(obj) == 't') {
                // texture coordinates - uv
            
                advance(&obj, 1);
                while (obj.count && SCHAR(obj) == ' ') advance(&obj, 1);            
                if (SCHAR(obj) != '-' && !IS_DIGIT(SCHAR(obj))) continue;
                
                Vector2 uv = {0};

                r = string_to_float(obj, &s, &rem);
                assert(s);
                obj = rem;
                uv.x = r;
    
                r = string_to_float(obj, &s, &rem);
                assert(s);
                obj = rem;
                uv.y = r;
                
                array_add(&m->uvs, uv);
                
            } else if (SCHAR(obj) == 'n') {
                // normals
                advance(&obj, 1);
                while (obj.count && SCHAR(obj) == ' ') advance(&obj, 1);            
                if (SCHAR(obj) != '-' && !IS_DIGIT(SCHAR(obj))) continue;
                
                Vector3 normal = {0};

                r = string_to_float(obj, &s, &rem);
                assert(s);
                obj = rem;
                normal.x = r;
    
                r = string_to_float(obj, &s, &rem);
                assert(s);
                obj = rem;
                normal.y = r;
                
                r = string_to_float(obj, &s, &rem);
                assert(s);
                obj = rem;
                normal.z = r;
                
                array_add(&m->normals, normal);
                
            } else if (SCHAR(obj) == ' ') {
                // vector coordinates
            
                advance(&obj, 1);
                while (obj.count && SCHAR(obj) == ' ') advance(&obj, 1);
                if (SCHAR(obj) != '-' && !IS_DIGIT(SCHAR(obj))) continue;
            
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
            } else {
                obj = string_eat_until(obj, '\n');
            }
        }
        else if (SCHAR(obj) == 'f') {
            // faces
        
            advance(&obj, 2);
            while (obj.count && SCHAR(obj) == ' ') advance(&obj, 1);
            
            if (!(SCHAR(obj) >= '1' && SCHAR(obj) <= '9')) continue;

            Face f = {0};

            ri = string_to_int(obj, &s, &rem);
            assert(s);
            obj = rem;
            f.v1 = ri-1;
            
            if (SCHAR(obj) == '/') {
                advance(&obj, 1);
                ri = string_to_int(obj, &s, &rem);
                assert(s);
                obj = rem;
                f.vt1 = ri-1;                
            }
            
            if (SCHAR(obj) == '/') {
                advance(&obj, 1);
                ri = string_to_int(obj, &s, &rem);
                assert(s);
                obj = rem;
                f.vn1 = ri-1;                
            }
            
            // skip not handled stuffs, every vertex index start after ' ' (space character)
            obj = string_eat_until(obj, ' '); 
            
            ri = string_to_int(obj, &s, &rem);
            assert(s);
            obj = rem;
            f.v2 = ri-1;
            
            if (SCHAR(obj) == '/') {
                advance(&obj, 1);
                ri = string_to_int(obj, &s, &rem);
                assert(s);
                obj = rem;
                f.vt2 = ri-1;
            }
            
            if (SCHAR(obj) == '/') {
                advance(&obj, 1);
                ri = string_to_int(obj, &s, &rem);
                assert(s);
                obj = rem;
                f.vn2 = ri-1;                
            }
            
            // skip not handled stuffs, every vertex index start after ' ' (space character)
            obj = string_eat_until(obj, ' ');
            
            ri = string_to_int(obj, &s, &rem);
            assert(s);
            obj = rem;
            f.v3 = ri-1;
 
            if (SCHAR(obj) == '/') {
                advance(&obj, 1);
                ri = string_to_int(obj, &s, &rem);
                assert(s);
                obj = rem;
                f.vt3 = ri-1;
            }
            
            if (SCHAR(obj) == '/') {
                advance(&obj, 1);
                ri = string_to_int(obj, &s, &rem);
                assert(s);
                obj = rem;
                f.vn3 = ri-1;                
            }
            
            array_add(&m->faces, f);
        }
        
        if ((obj.count-1) == 0) {
            break;
        }
        advance(&obj);
    }

    string_free(&obj);

    clog("obj parsed: " SFMT "\n", SARG(obj_filename));
    clog("vertices: %d ; faces: %d ; uvs: %d\n", m->vectors.count, m->faces.count, m->uvs.count);
    
    return m;
}

#endif 