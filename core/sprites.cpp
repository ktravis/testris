#include "core.h"

#include <cstdarg>

SpriteSheet *loadSpriteSheet(const char *fname, int tile_w, int tile_h) {
    int tex_w, tex_h;
    TextureHandle t;
    if (!loadTextureFile(fname, &t, &tex_w, &tex_h)) {
        return NULL;
    }
    if (tex_w % tile_w != 0 || tex_h % tile_h != 0) {
        fprintf(stderr, (const char *)"sprite sheet size (%d x %d) is not an even multiple of tile size (%d x %d)\n", tex_w, tex_h, tile_w, tile_h);
        return NULL;
    }
    SpriteSheet *sheet = (SpriteSheet*)malloc(sizeof(SpriteSheet));
    sheet->t = t;
    sheet->sheet_width = tex_w;
    sheet->sheet_height = tex_h;
    sheet->tile_w = tile_w;
    sheet->tile_h = tile_h;
    sheet->nrows = sheet->sheet_height / sheet->tile_h;
    sheet->ncols = sheet->sheet_width / sheet->tile_w;
    sheet->norm_tile_w = ((float)tile_w) / ((float)tex_w);
    sheet->norm_tile_h = ((float)tile_h) / ((float)tex_h);
    return sheet;
}

void init(Animation *a, SpriteSheet *s, double delay, ...) {
    va_list indices;
    va_start(indices, delay);

    a->sheet = s;
    a->indices = NULL;
    a->delay = delay;

    int idx;
    while ((idx = va_arg(indices, int)) != -1) {
        array_push(a->indices, idx);
    }
    a->count = array_len(a->indices);

    va_end(indices);
}

void drawSpriteFlippedX(Renderer *r, Sprite s, Vec2 pos, float w, float h) {
    pos.x += w;
    drawSprite(r, s, pos, -w, h);
}

void drawSpriteFlippedY(Renderer *r, Sprite s, Vec2 pos, float w, float h) {
    pos.y += h;
    drawSprite(r, s, pos, w, -h);
}

void drawSprite(Renderer *r, Sprite s, Vec2 pos, float w, float h) {
    Rect st;
    float ntw = s.sheet->norm_tile_w;
    float nth = s.sheet->norm_tile_h;
    st.x = (float)(s.index % s.sheet->ncols) * ntw;
    st.y = (s.index / s.sheet->ncols) * nth;
    st.w = ntw;
    st.h = nth;

    drawTexturedQuad(r, pos, w, h, s.sheet->t, st); 
}

int addFrame(Animation *a, int i) {
    array_push(a->indices, i);
    return ++a->count;
}

Sprite step(AnimState *s, float dt) {
    Animation *a = s->a;
    s->t += dt;
    if (s->t > a->delay) {
        s->t -= a->delay * (int)(s->t / a->delay);
        s->curr = (s->curr + 1) % a->count;
    }
    return currFrame(s);
}

Sprite currFrame(AnimState *s) {
    Sprite sp = {
        .sheet = s->a->sheet,
        .index = s->a->indices[s->curr],
    };
    return sp;
}

Sprite currFrame(Animation *a, float t) {
    int n = (int)(t / a->delay);
    Sprite sp = {
        .sheet = a->sheet,
        .index = a->indices[n % a->count],
    };
    return sp;
}

void freeAnimation(Animation *a) {
    array_free(a->indices);
    free(a);
}
