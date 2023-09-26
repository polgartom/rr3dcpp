#ifndef H_NEW_STRING
#define H_NEW_STRING

#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <cstdlib>

#define IS_ALPHA(c) ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
#define IS_DIGIT(c) (c >= '0' && c <= '9')
#define IS_ALNUM(c) (IS_ALPHA(c) || IS_DIGIT(c) || c == '_')
#define IS_SPACE(c) ((c) == ' ' || (c) == '\t' || (c) == '\r' || (c) == '\n')

typedef struct {
    char *data;
    char *alloc_location; // If allocated on the heap
    int count;
} String;

#define SFMT "%.*s"
#define SARG(__s) (int) (__s).count, (__s).data 
// Usage: printf("This is an example: " SFMT "\n", SARG(value));

String string_create(char *data)
{
    String s;
    s.data  = data;
    s.count = strlen(data);
    
    return s;
}

String string_make_alloc(unsigned int size)
{
    String s;
    s.alloc_location = (char *)malloc(size+1);
    s.data           = (char *)memset(s.alloc_location, 0, (size+1));
    s.count          = size;
    
    return s;
}

String string_advance(String s, unsigned int step = 1)
{
    assert(s.count >= step && step >= 0);

    String new_s = s;
    new_s.data   = s.data + step; 
    new_s.count -= step;
        
    return new_s;
}
inline void string_advance(String *s, unsigned int step = 1) 
{
    String r = string_advance(*s);
    memcpy(s, &r, sizeof(String));
}

char *string_to_cstr(String s)
{
    // @Leak:
    char *c_str = (char *)memset(((char *)malloc(s.count+1)), 0, (s.count+1));
    memcpy(c_str, s.data, s.count);
    return c_str;
}

bool string_equal(String a, String b)
{
    if (a.count != b.count) return false;
    int max_count = a.count > b.count ? a.count : b.count;
    for (int i = 0; i < max_count; i++) {
        if (a.data[i] != b.data[i]) return false;
    }
    
    return true;
}

// inline bool *string_equal_nocase(String a, String b)
// {
//     // @Todo
//     assert(0);
// }

inline bool string_equal_cstr(String a, char *b)
{
    return string_equal(a, string_create(b));
}

String string_trim_white(String s)
{
    // Trim left
    while (s.count && IS_SPACE(*s.data)) {
        s.data += 1;
        s.count -= 1;
    }
    
    // Trim Right
    while (s.count && IS_SPACE(s.data[s.count-1])) {
        s.count -= 1;
    }
    
    return s;
}

int string_to_int(String s, bool *failed)
{
    char *cstr = string_to_cstr(s);
    int num    = atoi(cstr);
    free(cstr);
    if (num == 0 && !string_equal_cstr(s, "0")) *failed = true;

    return num; 
}

int string_to_int(String s, bool *success, String *remained)
{
    assert(success != NULL);
    if (s.count == 0 || !s.data) {
        *success = false;
        return 0.0f;
    }

    char *o = remained->data;
    int i = strtol(s.data, &remained->data, 0);

    int bef = remained->count;    
    remained->count -= remained->data - o;

    *success = true;
    return i;
}

float string_to_float(String s, bool *success, String *remained)
{
    assert(success != NULL);
    if (s.count == 0 || !s.data) {
        *success = false;
        return 0.0f;
    }

    char *o = remained->data;
    float f = strtof(s.data, &remained->data);

    int bef = remained->count;    
    remained->count -= remained->data - o;

    *success = true;    
    return f;
}

#endif