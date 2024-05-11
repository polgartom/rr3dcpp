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
