#ifndef H_RR3DCPP
#define H_RR3DCPP

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>

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

#include "new_string.h"
#include "array.h"
#include "window.h"

struct Vector3 {
    float x, y, z = 0.0f;
};

struct Vector4 {
    float x, y, z, w = 0.0f;
};

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
};

struct Model {
    float x, y, z = 0.0f;
    float scale = 1.0f;
    float rx = 0.0f;
    float ry = 0.0f;
    float rz = 0.0f;
        
    float sx = 0.0f;
    float sy = 0.0f;

    Array<Vector3> vectors;
    Array<Face>    faces;
};

String read_entire_file(char *filename)
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
    char buf[4096] = {0};
    
    va_start(args, __fmt_msg);
    _vstprintf_s(buf, __fmt_msg, args);
    va_end(args);
    
    buf[4096-1] = 0;
    
    WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), buf, strlen(buf), NULL, NULL);
}

inline void swap(float *a, float *b)
{
    float t = *a;
    *a = *b;
    *b = t;
}

#define CLOG_VEC(_v) clog("{ x: %f ; y: %f ; z: %f }\n", (_v).x, (_v).y, (_v).z);

#define CLOG_START()  clog("{ ")
#define CLOG_F(_f)    clog(XSTR(_f) ": %f ; ", _f)
#define CLOG_D(_d)    clog(XSTR(_d) ": %d ; ", _d)
#define CLOG_LD(_ld)  clog(XSTR(_ld) ": %ld ; ", _ld)
#define CLOG_CS(_cs)  clog(XSTR(_cs) ": %s ; ", _cs)
// new_string.h
#define CLOG_S(_s)    clog(XSTR(_s) ": " SFMT " ; ", SARG(_s))
#define CLOG_END()    clog("}\n")
#define CLOG1(_CLOG)  { CLOG_START(); _CLOG; CLOG_END(); }

#include "math.h"


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
            obj = string_eat_until(obj, ' '); 
            
            ri = string_to_int(obj, &s, &rem);
            assert(s);
            obj = rem;
            f.v2 = ri-1;
            
            // skip not handled stuffs, every vertex index start after ' ' (space character)
            obj = string_eat_until(obj, ' ');
            
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

inline Vector3 make_vector3(float x, float y, float z)
{
    return {x, y, z};
}

#endif 