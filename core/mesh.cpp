#include "core.h"

static const Vec4 quadTopLeft = {0, 0, 0, 1};
static const Vec4 quadTopRight = (Vec4){1, 0, 0, 1};
static const Vec4 quadBottomLeft = (Vec4){0, 1, 0, 1};
static const Vec4 quadBottomRight = (Vec4){1, 1, 0, 1};

static const Vec2 texTopLeft = (Vec2){0, 0};
static const Vec2 texTopRight = (Vec2){1, 0};
static const Vec2 texBottomLeft = (Vec2){0, 1};
static const Vec2 texBottomRight = (Vec2){1, 1};

VertexData quadVerts[] = {
    (VertexData){quadTopLeft, texTopLeft},
    (VertexData){quadTopRight, texTopRight},
    (VertexData){quadBottomRight, texBottomRight},

    (VertexData){quadBottomRight, texBottomRight},
    (VertexData){quadBottomLeft, texBottomLeft},
    (VertexData){quadTopLeft, texTopLeft},
};

Mesh quadMesh = {
    .count = sizeof(quadVerts)/sizeof(VertexData), // should be 6!
    .data = quadVerts
};

// TODO: load mesh data

Mesh texturedQuadMesh(VertexData data[6], Rect v, Rect st) {
    data[0] = (VertexData){vec4(v.pos, 0, 1), st.pos};
    data[1] = (VertexData){vec4(v.x+v.w, v.y, 0, 1), vec2(st.x+st.w, st.y)};
    data[2] = (VertexData){vec4(add(v.pos, v.box), 0, 1), add(st.pos, st.box)};

    data[3] = (VertexData){vec4(add(v.pos, v.box), 0, 1), add(st.pos, st.box)};
    data[4] = (VertexData){vec4(v.x, v.y+v.h, 0, 1), vec2(st.x, st.y+st.h)};
    data[5] = (VertexData){vec4(v.pos, 0, 1), st.pos};
    return (Mesh){.count=6, .data=data};
}

Mesh texturedQuadMesh(VertexData data[6], Rect st) {
    return texturedQuadMesh(data, rect(0, 0, 1, 1), st);
}
