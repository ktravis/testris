#include "core.h"

Rect rect(float x0, float y0, float x1, float y1) {
    Rect r;
    r.x = x0;
    r.y = y0;
    r.w = x1 - x0;
    r.h = y1 - y0;
    return r;
}

Rect rect(Vec2 pos, float w, float h) {
    return Rect{pos, vec2(w, h)};
}

Rect rect(Vec2 pos, Vec2 box) {
    return Rect{pos, box};
}

bool collide(Rect a, Rect b) {
    if (a.x < b.x + b.w &&
        a.x + a.w > b.x &&
        a.y < b.y + b.h &&
        a.y + a.h > b.y) {
        return true;
    }
    return false;
}

bool contains(Rect r, Vec2 v) {
    return (v.x > r.x && v.x < (r.x + r.w) &&
            v.y > r.y && v.y < (r.y + r.h));
}

bool wouldCollide(Rect a, Vec2 d, Rect b) {
    // TODO: return interpolated collision point
    a.pos = add(a.pos, d);
    return collide(a, b);
}
