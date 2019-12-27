#include "core.h"

#include <math.h>

float radians(float deg) {
    return M_PI * deg / 180.0f;
}

void copy(Mat4 from, Mat4 to) {
    for (int i = 0; i < 16; i++) {
        to[i%4][i/4] = from[i%4][i/4];
    }
}

void zero(Mat4 result) {
    for (int i = 0; i < 16; i++) {
        result[i%4][i/4] = 0.0f;
    }
}

void identity(Mat4 result) {
    for (int i = 0; i < 16; i++) {
        result[i%4][i/4] = ((i/4) == (i%4)) ? 1.0f : 0.0f;
    }
}

void rotation(Mat4 result, float deg) {
    identity(result);
    result[0][0] = cos(M_PI * deg / 180);
    result[0][1] = sin(M_PI * deg / 180);
    result[1][0] = -sin(M_PI * deg / 180);
    result[1][1] = cos(M_PI * deg / 180);
}

void translation(Mat4 result, float x, float y, float z) {
    identity(result);
    result[3][0] = x;
    result[3][1] = y;
    result[3][2] = z;
}

void multiply(Mat4 result, Mat4 A, Mat4 B) {
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            result[j][i] = A[0][i]*B[j][0] + A[1][i]*B[j][1] + A[2][i]*B[j][2] + A[3][i]*B[j][3];
}

void multiply(Mat4 A, Mat4 B) {
    Mat4 result;
    multiply(result, A, B);

    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            A[j][i] = result[j][i];
}

Vec4 multiply(Mat4 A, Vec4 v) {
    Vec4 result;
    result.x = (A[0][0] * v.x) + 
               (A[1][0] * v.y) + 
               (A[2][0] * v.z) + 
               (A[3][0] * v.w);

    result.y = (A[0][1] * v.x) + 
               (A[1][1] * v.y) + 
               (A[2][1] * v.z) + 
               (A[3][1] * v.w);

    result.z = (A[0][2] * v.x) + 
               (A[1][2] * v.y) + 
               (A[2][2] * v.z) + 
               (A[3][2] * v.w);

    result.w = (A[0][3] * v.x) + 
               (A[1][3] * v.y) + 
               (A[2][3] * v.z) + 
               (A[3][3] * v.w);
    return result;
}

void translate(Mat4 result, float x, float y, float z) {
    Mat4 m;
    translation(m, x, y, z);
    Mat4 tmp;
    copy(result, tmp);
    multiply(result, m, tmp);
}

void translate(Mat4 result, Vec3 by) {
    translate(result, by.x, by.y, by.z);
}

void rotate(Mat4 result, float deg) {
    Mat4 m;
    rotation(m, deg);
    Mat4 tmp;
    copy(result, tmp);
    multiply(result, m, tmp);
}

void scale(Mat4 result, Vec3 s) {
    Mat4 sm;
    identity(sm);
    sm[0][0] *= s.x;
    sm[1][1] *= s.y;
    sm[2][2] *= s.z;
    Mat4 tmp;
    copy(result, tmp);
    multiply(result, sm, tmp);
}

void scale(Mat4 result, Vec2 s) {
    scale(result, vec3(s.x, s.y, 1.0f));
}

void scale(Mat4 result, float s) {
    scale(result, vec3(s, s, s));
}

void rotation(Mat4 result, float rads, Vec3 axis) {
    float rcos = cos(rads);
    float rsin = sin(rads);
    float ircos = 1.0f - rcos;

    float x = axis.x;
    float y = axis.y;
    float z = axis.z;

    result[0][0] = rcos + x*x*ircos;
    result[1][0] = x*y*ircos - z*z*rsin;
    result[2][0] = x*z*ircos + y*rsin;
    result[3][0] = 0.0f;

    result[0][1] = y*x*ircos + z*rsin;
    result[1][1] = rcos + y*y*ircos;
    result[2][1] = y*z*ircos - x*rsin;
    result[3][1] = 0.0f;

    result[0][2] = z*x*ircos - y*rsin;
    result[1][2] = z*y*ircos + x*rsin;
    result[2][2] = rcos + z*z*ircos;;
    result[3][2] = 0.0f;

    result[0][3] = 0.0f;
    result[1][3] = 0.0f;
    result[2][3] = 0.0f;
    result[3][3] = 1.0f;
}

void rotate(Mat4 result, float rads, Vec3 axis) {
    Mat4 r;
    rotation(r, rads, axis);
    Mat4 tmp;
    copy(result, tmp);
    multiply(result, r, tmp);
}

void rotateAroundOffset(Mat4 result, float rads, Vec3 axis, Vec3 offset) {
    translate(result, scaled(offset, -1));
    rotate(result, rads, axis);
    translate(result, offset);
}

void lookAt(Mat4 result, Vec3 eye, Vec3 center, Vec3 up) {
    Vec3 f = normalized(sub(center, eye));
    Vec3 u = up;
    Vec3 s = normalized(cross(f, u));
    u = cross(s, f);

    result[0][0] = s.x;
    result[1][0] = s.y;
    result[2][0] = s.z;
    result[3][0] = 0.0f;

    result[0][1] = u.x;
    result[1][1] = u.y;
    result[2][1] = u.z;
    result[3][1] = 0.0f;

    result[0][2] = -f.x;
    result[1][2] = -f.y;
    result[2][2] = -f.z;
    result[3][2] = 0.0f;

    result[3][0] = -dot(s, eye);
    result[3][1] = -dot(u, eye);
    result[3][2] = dot(f, eye);
    result[3][3] = 1.0f;
}

void ortho(Mat4 result, float left, float right, float top, float bottom, float near, float far) {
    zero(result);
    result[0][0] = 2.0f / (right - left);
    result[1][1] = 2.0f / (top - bottom);
    result[2][2] = -2.0f / (far - near);
    result[3][0] = -(right + left) / (right - left);
    result[3][1] = -(top + bottom) / (top - bottom);
    result[3][2] = -(far + near) / (far - near);
    result[3][3] = 1;
}

void perspective(Mat4 result, float fovy_rads, float aspect, float z_near, float z_far) {
    float tan_half_fovy = tan(fovy_rads / float(2));

    result[0][0] = 1.0f/(aspect * tan_half_fovy);
    result[0][1] = 0.0f;
    result[0][2] = 0.0f;
    result[0][3] = 0.0f;

    result[1][0] = 0.0f;
    result[1][1] = 1.0f/tan_half_fovy;
    result[1][2] = 0.0f;
    result[1][3] = 0.0f;

    result[2][0] = 0.0f;
    result[2][1] = 0.0f;
    result[2][2] = -(z_far + z_near) / (z_far - z_near);
    result[2][3] = -1.0f;

    result[3][0] = 0.0f;
    result[3][1] = 0.0f;
    result[3][2] = -(2.0f * z_far * z_near) / (z_far - z_near);
    result[3][3] = 0.0f;
}
