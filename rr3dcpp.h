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
    float x, y, z;
};
#define VEC_FMT "v.x: %f ; v.y: %f ; v.z: %f"
#define VEC_ARG(__v) __v.x, __v.y, __v.z

struct Matrix3 {
    float _00, _01, _02,
          _10, _11, _12,
          _20, _21, _22;
};

struct Face {
    int v1, v2, v3;
};

struct Model {
    float scale = 1.0f;

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

#define PRINT_VEC(_v) clog("{ x: %f ; y: %f ; z: %f }\n", (_v).x, (_v).y, (_v).z);

#include "math.h"

#endif 