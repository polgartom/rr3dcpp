#pragma once

inline Vector3 func multiply(Vector3 v, Matrix3 m)
{
    Vector3 r;
    r.x = (v.x * m._00) + (v.y * m._01) + (v.z * m._02);
    r.y = (v.x * m._10) + (v.y * m._11) + (v.z * m._12);
    r.z = (v.x * m._20) + (v.y * m._21) + (v.z * m._22);

    return r;
}

inline Vector3 func multiply(Vector3 v, Matrix4 m)
{
    Vector3 r;
    r.x = (v.x * m._00) + (v.y * m._10) + (v.z * m._20) + (m._30);
    r.y = (v.x * m._01) + (v.y * m._11) + (v.z * m._21) + (m._31);
    r.z = (v.x * m._02) + (v.y * m._12) + (v.z * m._22) + (m._32);
    float w = (v.x * m._03) + (v.y * m._13) + (v.z * m._23) + (m._33);

    if (w != 0.0f) {
        r.x /= w;
        r.y /= w;
        r.z /= w;
    }

    return r;
}

inline Vector4 func multiply(Vector4 v, Matrix4 m)
{
    Vector4 r;
    r.x = (v.x * m._00) + (v.y * m._01) + (v.z * m._02) + (v.w * m._03);
    r.y = (v.x * m._10) + (v.y * m._11) + (v.z * m._12) + (v.w * m._13);
    r.z = (v.x * m._20) + (v.y * m._21) + (v.z * m._22) + (v.w * m._23);
    r.w = (v.x * m._30) + (v.y * m._31) + (v.z * m._32) + (v.w * m._33);

    return r;
}

inline Matrix4 func multiply(Matrix4 a, Matrix4 b)
{
    Matrix4 r = {0};
    
    r._00 = a._00 * b._00 + a._01 * b._10 + a._02 * b._20 + a._03 * b._30;
    r._10 = a._10 * b._00 + a._11 * b._10 + a._12 * b._20 + a._13 * b._30;
    r._20 = a._20 * b._00 + a._21 * b._10 + a._22 * b._20 + a._23 * b._30;
    r._30 = a._30 * b._00 + a._31 * b._10 + a._32 * b._20 + a._33 * b._30;
    
    r._01 = a._00 * b._01 + a._01 * b._11 + a._02 * b._21 + a._03 * b._31;
    r._11 = a._10 * b._01 + a._11 * b._11 + a._12 * b._21 + a._13 * b._31;
    r._21 = a._20 * b._01 + a._21 * b._11 + a._22 * b._21 + a._23 * b._31;
    r._31 = a._30 * b._01 + a._31 * b._11 + a._32 * b._21 + a._33 * b._31;
    
    r._02 = a._00 * b._02 + a._01 * b._12 + a._02 * b._22 + a._03 * b._32;
    r._12 = a._10 * b._02 + a._11 * b._12 + a._12 * b._22 + a._13 * b._32;
    r._22 = a._20 * b._02 + a._21 * b._12 + a._22 * b._22 + a._23 * b._32;
    r._32 = a._30 * b._02 + a._31 * b._12 + a._32 * b._22 + a._33 * b._32;
    
    r._03 = a._00 * b._03 + a._01 * b._13 + a._02 * b._23 + a._03 * b._33;
    r._13 = a._10 * b._03 + a._11 * b._13 + a._12 * b._23 + a._13 * b._33;
    r._23 = a._20 * b._03 + a._21 * b._13 + a._22 * b._23 + a._23 * b._33;
    r._33 = a._30 * b._03 + a._31 * b._13 + a._32 * b._23 + a._33 * b._33;

    return r;
}

inline Vector3 func substract(Vector3 a, Vector3 b)
{
    Vector3 r = {
        a.x - b.x,
        a.y - b.y,
        a.z - b.z
    };
    return r;
}

inline func void func scale(Vector3 *v, float scale)
{
    v->x *= scale;
    v->y *= scale;
    v->z *= scale;
}

// inline void scale(Model *m, float scale)
// {
//     m->scale = scale;
//     for (int i = 0; i < m->vectors.count; i++) {
//         Vector3 *v = &m->vectors[i];
//         v->x *= scale;
//         v->y *= scale;
//         v->z *= scale;
//         // This is a fucking bug or what??????????? -> ..\main.cpp(127): error C2064: term does not evaluate to a function taking 2 arguments
//         // scale(v, scale);
//     }
// }

inline void func rotate_x(Vector3 *v, float theta)
{
    Matrix4 m = {0};
    ZERO_MEMORY(&m, sizeof(m));

    m._00 = 1;
    m._11 = cosf(theta * 0.5f);
    m._12 = sinf(theta * 0.5f);
    m._21 = -sinf(theta * 0.5f);
    m._22 = cosf(theta * 0.5f);
    m._33 = 1;

    Vector3 r = multiply(*v, m);

    v->x = r.x;
    v->y = r.y;
    v->z = r.z;
}

inline void func rotate_y(Vector3 *v, float theta)
{
    Matrix3 m = {
        cosf(theta), 0, sinf(theta),
        0, 1, 0,
        -sinf(theta), 0, cosf(theta),
    };

    Vector3 r = multiply(*v, m);

    v->x = r.x;
    v->y = r.y;
    v->z = r.z;
}

inline void func rotate_z(Vector3 *v, float theta)
{
    Matrix4 m = {0};
    ZERO_MEMORY(&m, sizeof(m));

    m._00 = cosf(theta);
    m._01 = sinf(theta);
    m._10 = -sinf(theta);
    m._11 = cosf(theta);
    m._22 = 1;
    m._33 = 1;

    Vector3 r = multiply(*v, m);

    v->x = r.x;
    v->y = r.y;
    v->z = r.z;
}

inline void func rotate_x(Model *m, float val)
{
    m->rx += val;

    // for (int i = 0; i < m->vectors.count; i++) {
    //     rotate_x(&m->vectors[i], val);
    // }
}

inline void func rotate_y(Model *m, float val)
{
    m->ry += val;

    // for (int i = 0; i < m->vectors.count; i++) {
    //     rotate_y(&m->vectors[i], val);
    // }
}

inline void func rotate_z(Model *m, float val)
{
    m->rz += val;

    // for (int i = 0; i < m->vectors.count; i++) {
    //     rotate_z(&m->vectors[i], val);
    // }
}

inline void func rotate(Vector3 *v, float x, float y, float z)
{
    rotate_x(v, x);
    rotate_y(v, y);
    rotate_z(v, z);
}

inline void func rotate(Vector3 *v, Vector3 rot)
{
    rotate_x(v, rot.x);
    rotate_y(v, rot.y);
    rotate_z(v, rot.z);
}

inline void func rotate(Vector3 *v, Model *m)
{
    rotate(v, m->rx, m->ry, m->rz);
}

inline float rad_to_deg(float rad)
{
    return rad * 180.0f / M_PI;
}

inline float deg_to_rad(float deg)
{
    return deg * M_PI / 180.0;
}