#include "core.h"

Color white = {1.f, 1.f, 1.f, 1.f};
Color red = {1.f, 0.f, 0.f, 1.f};
Color green = {0.f, 1.f, 0.f, 1.f};
Color blue = {0.f, 0.f, 1.f, 1.f};
Color grey = {0.3f, 0.3f, 0.3f, 1.f};
Color black = {0.f, 0.f, 0.f, 1.f};

Color hex(uint32_t d) {
    Color c;
    c.r = (0xff & (d >> 24))/256.0f;
    c.g = (0xff & (d >> 16))/256.0f;
    c.b = (0xff & (d >> 8))/256.0f;
    c.a = (0xff & d)/256.0f;
    return c;
}

float clamp(float v, float min, float max) {
    return (v < min ? min : (v > max ? max : v));
}

Color multiply(Color c, float f) {
    c.r = clamp(c.r * f, 0, 1);
    c.g = clamp(c.g * f, 0, 1);
    c.b = clamp(c.b * f, 0, 1);
    c.a = clamp(c.a * f, 0, 1);
    return c;
}
