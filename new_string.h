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

#define SFMT "%.*s"
#define SARG(__s) (int) (__s).count, (__s).data 
#define SARGC(__s, __c) (int)__c, (__s).data 
// Usage: printf("This is an example: " SFMT "\n", SARG(value));

#define SCHAR(s) (*s.data)

constexpr const unsigned int string_byte_padding = 8;

struct String {
    char *data = nullptr;
    char *alloc_location = nullptr; // If allocated on the heap
    int count;
    int refcount;

    String () {}
    
    ~String() {
    }
    
    String (const char *s)
    {
        data  = (char *)s;
        count = strlen(s);
    }

    inline void operator=(const char *rhs)
    {
        String new_s = String(rhs);
        STRUCT_COPY(*this, new_s);
    }

};

inline char schar(String &s)
{
    return *s.data;
}

inline String string_create(const char *data)
{
    return String(data);
}

inline String string_make_alloc(unsigned int size)
{   
    auto padded_size = (string_byte_padding - (size % string_byte_padding)) + size;
    assert(!(padded_size % string_byte_padding));
    // clog("[string_make_alloc]: %ld ; size: %ld\n", padded_size, size);

    String s;
    s.alloc_location = (char *)malloc(padded_size);
    s.data           = (char *)memset(s.alloc_location, 0, (padded_size));
    s.count          = size;
    
    return s;
}

inline void string_free(String *s)
{
    // @Todo: Free heap allocations automatically, but we have to do a ref counting for that
    if (s->alloc_location) {
        free(s->alloc_location);
        s->alloc_location = nullptr;
    }
}

inline String advance(String s, unsigned int step = 1)
{
    // if (!(s.count >= step && step >= 0)) {
    //     assert(s.count >= step && step >= 0);    
    // }
    assert(s.count >= step && step >= 0);

    s.data   = s.data + step; 
    s.count -= step;
        
    return s;
}

inline void advance(String *s, unsigned int step = 1) 
{
    String r = advance(*s, step);
    s->data = r.data;
    s->count = r.count;
}

inline char *string_to_cstr(String s)
{
    // @Leak:
    char *c_str = (char *)memset(((char *)malloc(s.count+1)), 0, (s.count+1));
    memcpy(c_str, s.data, s.count);
    return c_str;
}

inline bool string_equal(String a, String b)
{
    if (a.count != b.count) return false;

#if 0

    for (int i = 0; i < a.count; i++) {
        if (a.data[i] != b.data[i]) return false;
    }
    
#else 

    int i = 0;
    int target = a.count / string_byte_padding;
    do {
        if (*(reinterpret_cast<unsigned long*>(a.data)+i) != *(reinterpret_cast<unsigned long*>(b.data)+i)) {
            return false;
        }
    } while (i++ != target);
    
#endif
    return true;
}

inline bool string_equal_cstr(String a, char *b)
{
    return string_equal(a, string_create(b));
}

inline String string_trim_white(String s)
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

inline void string_trim_white(String *s)
{
    String r = string_trim_white(*s);
    STRUCT_COPY(*s, r); // @Todo: remove this crap
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

inline String string_eat_until(String s, const char c)
{
    while (s.count != 0 && *s.data && *s.data != c) {
        advance(&s);
    }
    
    return s;
}

inline bool operator==(String &lhs, String &rhs)
{
    return string_equal(lhs, rhs);
}

inline bool operator==(String &lhs, char *rhs)
{
    return string_equal_cstr(lhs, rhs);
}

#endif