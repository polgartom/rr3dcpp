#ifndef _H_MY_MATH_
#define _H_MY_MATH 1

inline Vector3 multiply(Vector3 v, Matrix3 m) 
{
    Vector3 r;
    r.x = (v.x * m._00) + (v.y * m._01) + (v.z * m._02);
    r.y = (v.x * m._10) + (v.y * m._11) + (v.z * m._12);
    r.z = (v.x * m._20) + (v.y * m._21) + (v.z * m._22);
    
    return r;
}

inline void scale(Vector3 *v, float scale)
{
    v->x *= scale;
    v->y *= scale;
    v->z *= scale;
}

inline void scale(Model *m, float scale)
{
    m->scale = scale;
    for (int i = 0; i < m->vectors.count; i++) {
        Vector3 *v = &m->vectors[i];
        v->x *= scale;
        v->y *= scale;
        v->z *= scale;
        // This is a fucking bug or what??????????? -> ..\main.cpp(127): error C2064: term does not evaluate to a function taking 2 arguments
        // scale(v, scale);
    }
}

inline void rotate_x(Vector3 *v, float val) 
{
    Matrix3 m = {
        1.0f, 0.0f, 0.0f,
        0.0f, (float)cos(val), (float)-sin(val),
        0.0f, (float)sin(val), (float)cos(val),
    };

    Vector3 r = multiply(*v, m);

    v->x = r.x;
    v->y = r.y;
    v->z = r.z;
}

inline void rotate_y(Vector3 *v, float val) 
{
    Matrix3 m = {
        (float)cos(val), 0, (float)sin(val),
        0, 1, 0,
        (float)-sin(val), 0, (float)cos(val),
    };

    Vector3 r = multiply(*v, m);

    v->x = r.x;
    v->y = r.y;
    v->z = r.z;
}

inline void rotate_z(Vector3 *v, float val) 
{
    Matrix3 m = {
        (float)cos(val), (float)-sin(val), 0,
        (float)sin(val), (float)cos(val), 0,
        1, 0, 1,
    };

    Vector3 r = multiply(*v, m);

    v->x = r.x;
    v->y = r.y;
    v->z = r.z;
}

inline void rotate_x(Model *m, float val)
{
    m->rx += val;

    for (int i = 0; i < m->vectors.count; i++) {
        rotate_x(&m->vectors[i], val);
    }
}

inline void rotate_y(Model *m, float val)
{
    m->ry += val;

    for (int i = 0; i < m->vectors.count; i++) {
        rotate_y(&m->vectors[i], val);
    }
}

inline void rotate_z(Model *m, float val)
{
    m->rz += val;

    for (int i = 0; i < m->vectors.count; i++) {
        rotate_z(&m->vectors[i], val);
    }
}

#endif
