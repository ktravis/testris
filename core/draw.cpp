#include "core.h"

void drawLine(Renderer *r, Vec2 a, Vec2 b, Color c) {
    useShader(r, &r->defaultShader);

    VertexData points[] = {
        (VertexData){vec4(a, 0, 1), vec2(0, 0)},
        (VertexData){vec4(b, 0, 1), vec2(0, 0)}
    };

    Mesh m = {
        .count = sizeof(points)/sizeof(points[0]),
        .data = points
    };

    DrawCall call;
    call.mode = Lines;
    call.mesh = &m;
    call.texture = r->defaultTexture;
    identity(call.uniforms.model);
    call.uniforms.tint = c;

    draw(r, call);
}

void drawRectOutline(Renderer *rend, Rect r, Color c, float thickness) {
    useShader(rend, &rend->defaultShader);

    r.x += 1;
    r.y += 1;
    r.w -= 2;
    r.h -= 2;
    VertexData points[] = {
        (VertexData){vec4(r.pos, 0, 1), vec2(0, 0)},
        (VertexData){vec4(add(r.pos, vec2(0, r.h)), 0, 1), vec2(0, 0)},

        (VertexData){vec4(add(r.pos, vec2(0, r.h)), 0, 1), vec2(0, 0)},
        (VertexData){vec4(add(r.pos, r.box), 0, 1), vec2(0, 0)},

        (VertexData){vec4(add(r.pos, r.box), 0, 1), vec2(0, 0)},
        (VertexData){vec4(add(r.pos, vec2(r.w, 0)), 0, 1), vec2(0, 0)},

        (VertexData){vec4(add(r.pos, vec2(r.w, 0)), 0, 1), vec2(0, 0)},
        (VertexData){vec4(r.pos, 0, 1), vec2(0, 0)}
    };

    Mesh m = {
        .count = sizeof(points)/sizeof(points[0]),
        .data = points
    };

    DrawCall call;
    call.mode = Lines;
    call.mesh = &m;
    call.texture = rend->defaultTexture;
    identity(call.uniforms.model);
    call.uniforms.tint = c;
    call.uniforms.lineWidth = thickness;

    draw(rend, call);
}

void drawMesh(Renderer *r, Mesh *m, Vec2 pos, TextureHandle texture, DrawOpts2d opts) {
    DrawCall call;
    call.mode = Triangles;
    call.mesh = m;
    call.texture = texture;
    identity(call.uniforms.model);
    translate(call.uniforms.model, scaled(vec3(opts.origin, 0), -1));
    scale(call.uniforms.model, opts.scale);
    rotate(call.uniforms.model, opts.rotation);
    translate(call.uniforms.model, vec3(pos, 0));
    call.uniforms.tint = opts.tint;

    draw(r, call);
}

void drawMesh(Renderer *r, Mesh *m, Vec3 pos, TextureHandle texture, DrawOpts3d opts) {
    DrawCall call;
    call.mode = Triangles;
    call.mesh = m;
    call.texture = texture;
    identity(call.uniforms.model);
    translate(call.uniforms.model, scaled(opts.origin, -1));
    //rotate(call.uniforms.model, opts.rotation);
    scale(call.uniforms.model, opts.scale);
    translate(call.uniforms.model, pos);
    call.uniforms.tint = opts.tint;

    draw(r, call);
}

// TODO: this is slow as hell lol
void drawText(Renderer *r, FontAtlas *font, float x, float y, const char *text, DrawOpts2d opts) {
    Vec2 v = vec2(x, y);

    useShader(r, opts.shader ? opts.shader : &r->defaultShader);
    float xoff = 0;
    float yoff = 0;
    int vert_count = strlen(text) * 6;

    while (*text) {
        unsigned char u = *text;
        if (u == '\n') {
            xoff = 0;
            yoff += font->lineHeight;
        } else if (u >= 32 && u < 128) {
            float xadv = 0;
            float yadv = 0;
            stbtt_aligned_quad q;
            stbtt_GetBakedQuad(font->cdata, 512,512, u-32, &xadv, &yadv, &q, 1);//1=opengl & d3d10+,0=d3d9

            if (u != ' ' && u != '\t') {
                VertexData data[6];
                Mesh m = texturedQuadMesh(data, rect(xoff+q.x0, yoff+q.y0, xoff+q.x1, yoff+q.y1), rect(q.s0, q.t0, q.s1, q.t1));
                drawMesh(r, &m, v, font->tex, opts);
            }

            xoff += xadv;
            yoff += yadv;
        }
        ++text;
    }
}

void drawTextf(Renderer *r, FontAtlas *font, float x, float y, DrawOpts2d opts, const char *fmt...) {
    char buf[255];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    drawText(r, font, x, y, buf, opts);
}

void drawTextCentered(Renderer *r, FontAtlas *font, float x, float y, const char *text, DrawOpts2d opts) {
    Vec2 dim = getTextDimensions(text, font);
    // text draws from the bottom left, not the top left - so we need to shift
    // the origin accordingly
    opts.origin = vec2(0.5*dim.x, -0.5*dim.y);

    drawText(r, font, x, y, text, opts);
}

void drawTexturedQuad(Renderer *r, Rect quad, TextureHandle texture, Rect st, DrawOpts2d opts) {
    useShader(r, opts.shader ? opts.shader : &r->defaultShader);

    VertexData data[6];
    Mesh m = texturedQuadMesh(data, rect(vec2(0, 0), quad.box), st);
    drawMesh(r, &m, quad.pos, texture, opts);
}

void drawTexturedQuad(Renderer *r, Vec2 pos, float w, float h, TextureHandle texture, Rect st, DrawOpts2d opts) {
    drawTexturedQuad(r, rect(pos, w, h), texture, st, opts);
}

void drawRect(Renderer *rend, Rect r, DrawOpts2d opts) {
    drawTexturedQuad(rend, r, rend->defaultTexture, rect(0, 0, 1, 1), opts);
}

void drawRect(Renderer *rend, Rect r, Color c) {
    DrawOpts2d opts = {};
    opts.tint = c;
    drawTexturedQuad(rend, r, rend->defaultTexture, rect(0, 0, 1, 1), opts);
}
