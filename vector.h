#pragma once

struct Vector2 {
    union {
        struct {
            float x, y;
        };
		float m[2] = {0};
    };
    
    s64 index;
};

struct Vector3 {
    union {
        struct {
            float x, y, z;
        };
		float m[3] = {0};
    };
    
    s64 index;
};

struct Vector4 {
    union {
        struct {
            float x, y, z, w;
        };
		float m[4] = {0};
    };
};


inline float dot_product(Vector3 a, Vector3 b)
{
    return (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
}

inline Vector3 cross_product(Vector3 a, Vector3 b)
{
    Vector3 r;
    r.x = (a.y*b.z) - (a.z*b.y);
    r.y = (a.z*b.x) - (a.x*b.z);
    r.z = (a.x*b.y) - (a.y*b.x);
    
    return r;
}

inline float magnitude(Vector3 v) 
{
    return sqrtf( (v.x * v.x) + (v.y * v.y) + (v.z * v.z) );
}

inline float vector_angle(Vector3 a, Vector3 b) 
{
    auto p   = dot_product(a, b);
    auto am  = magnitude(a);
    auto bm  = magnitude(b);
    float rad = acosf(p/(am*bm));
    
    inline float rad_to_deg(float rad);
    return rad_to_deg(rad);
}   

inline Vector3 normalize(Vector3 v)
{
    float m = magnitude(v);
    v.x /= m;    
    v.y /= m;    
    v.z /= m;    
    
    return v;
}

inline Vector3 vec_normal(Vector3 v1, Vector3 v2, Vector3 v3)
{
    Vector3 line1, line2 = {0};
    
    line1.x = v2.x - v1.x;
    line1.y = v2.y - v1.y;
    line1.z = v2.z - v1.z;
    
    line2.x = v3.x - v1.x;
    line2.y = v3.y - v1.y;
    line2.z = v3.z - v1.z;
    
    return normalize(cross_product(line1, line2));
}

inline Vector3 make_vector3(float x, float y, float z)
{
    return {x, y, z};
}

// Overloads
// @Todo: Vector ptr overloads

inline Vector3 operator+(Vector3 &lhs, float add) {
    Vector3 r = {
        lhs.x + add,
        lhs.y + add,
        lhs.z + add
    };
    return r;
}

inline Vector3 operator-(Vector3 &lhs, float sub) {
    Vector3 r = {
        lhs.x - sub,
        lhs.y - sub,
        lhs.z - sub
    };
    return r;
}

inline Vector3 operator*(Vector3 &lhs, float scalar) {
    Vector3 r = {
        lhs.x * scalar,
        lhs.y * scalar,
        lhs.z * scalar
    };
    return r;
}

inline Vector3 operator/(Vector3 &lhs, float scalar) {
    Vector3 r = {
        lhs.x / scalar,
        lhs.y / scalar,
        lhs.z / scalar
    };
    return r;
}

inline Vector3 operator+(Vector3 &lhs, Vector3 &rhs) {
    Vector3 r = {
        lhs.x + rhs.x,
        lhs.y + rhs.y,
        lhs.z + rhs.z,
    };
    return r;
}

inline Vector3 operator+=(Vector3 &lhs, Vector3 &rhs) {
    lhs.x += rhs.x;
    lhs.y += rhs.y;
    lhs.z += rhs.z;
    return lhs;
}

inline Vector3 operator-(Vector3 &lhs, Vector3 &rhs) {
    Vector3 r = {
        lhs.x - rhs.x,
        lhs.y - rhs.y,
        lhs.z - rhs.z,
    };
    return r;
}

inline Vector3 operator*(Vector3 &lhs, Vector3 &rhs) {
    Vector3 r = {
        lhs.x * rhs.x,
        lhs.y * rhs.y,
        lhs.z * rhs.z,
    };
    return r;
}

inline Vector3 operator/(Vector3 &lhs, Vector3 &rhs) {
    Vector3 r = {
        lhs.x / rhs.x,
        lhs.y / rhs.y,
        lhs.z / rhs.z,
    };
    return r;
}