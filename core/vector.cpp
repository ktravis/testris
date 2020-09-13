#include "core.h"

float lerp(float a, float b, float step, float min) {
    if (b - a < min) {
        return b;
    }
    return a + (b - a)*step;
}

Vec2 vec2(float x, float y) {
    Vec2 v;
    v.x = x;
    v.y = y;
    return v;
}

float dsqr(Vec2 a) {
    return a.x * a.x + a.y * a.y;
}

Vec2 add(Vec2 a, Vec2 b) {
    a.x += b.x;
    a.y += b.y;
    return a;
}

Vec2 sub(Vec2 a, Vec2 b) {
    a.x -= b.x;
    a.y -= b.y;
    return a;
}

void scale(Vec2 *a, float s) {
    a->x *= s;
    a->y *= s;
}

void scale(Vec2 *a, Vec2 b) {
    a->x *= b.x;
    a->y *= b.y;
}

Vec2 scaled(Vec2 a, float s) {
    a.x *= s;
    a.y *= s;
    return a;
}

Vec2 scaled(Vec2 a, Vec2 b) {
    a.x *= b.x;
    a.y *= b.y;
    return a;
}

// this is not really a lerp but w/e
Vec2 lerp(Vec2 a, Vec2 b, float step, float min) {
    a.x = lerp(a.x, b.x, step, min);
    a.y = lerp(a.y, b.y, step, min);
    return a;
}

float dot(Vec2 a, Vec2 b) {
    return a.x * b.x + a.y * b.y;
}

void normalize(Vec2 *a) {
    float d = dsqr(*a);
    if (d == 0) {
        return;
    }
    a->x /= d;
    a->y /= d;
}

Vec2 normalized(Vec2 a) {
    normalize(&a);
    return a;
}

Vec3 vec3(float x, float y, float z) {
    Vec3 v;
    v.x = x;
    v.y = y;
    v.z = z;
    return v;
}

Vec3 vec3(Vec2 xy, float z) {
    Vec3 v;
    v.x = xy.x;
    v.y = xy.y;
    v.z = z;
    return v;
}

float dsqr(Vec3 a) {
    return a.x * a.x + a.y * a.y + a.z * a.z;
}

Vec3 add(Vec3 a, Vec3 b) {
    a.x += b.x;
    a.y += b.y;
    a.z += b.z;
    return a;
}

Vec3 sub(Vec3 a, Vec3 b) {
    a.x -= b.x;
    a.y -= b.y;
    a.z -= b.z;
    return a;
}

void scale(Vec3 *a, float s) {
    a->x *= s;
    a->y *= s;
    a->z *= s;
}

Vec3 scaled(Vec3 a, float s) {
    a.x *= s;
    a.y *= s;
    a.z *= s;
    return a;
}

float dot(Vec3 a, Vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vec3 cross(Vec3 u, Vec3 v) {
    Vec3 s;
    s.x = u.y*v.z - u.z*v.y;
    s.y = u.z*v.x - u.x*v.z;
    s.z = u.x*v.y - u.y*v.x;
    return s;
}

void normalize(Vec3 *a) {
    float d = sqrt(dsqr(*a));
    if (d == 0) {
        return;
    }
    a->x /= d;
    a->y /= d;
    a->z /= d;
}

Vec3 normalized(Vec3 a) {
    normalize(&a);
    return a;
}

inline Vec4 vec4(float x, float y, float z, float w) {
    return (Vec4){.x = x, .y = y, .z = z, .w = w};
}

inline Vec4 vec4(Vec2 xy, float z, float w) {
    return vec4(xy.x, xy.y, z, w);
}

inline Vec4 vec4(Vec3 xyz, float w) {
    return vec4(xyz.x, xyz.y, xyz.z, w);
}

inline Vec4 vec4() {
    return vec4(0, 0, 0, 1);
}

Vec4 add(Vec4 a, Vec4 b) {
    a.x += b.x;
    a.y += b.y;
    a.z += b.z;
    a.w += b.w;
    return a;
}
