#include "core.h"

void loadFontAtlas(FontAtlas *font, const unsigned char *font_data, float size) {
    stbtt_InitFont(&font->info, font_data, stbtt_GetFontOffsetForIndex(font_data,0));

    unsigned char temp_bitmap[512*512];
    stbtt_BakeFontBitmap(font_data, 0, size, temp_bitmap, 512, 512, 32, ATLAS_CHAR_COUNT, font->cdata); // no guarantee this fits!

    glGenTextures(1, &font->tex);
    glBindTexture(GL_TEXTURE_2D, font->tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    GLint swizzleMask[] = { GL_RED, GL_RED, GL_RED, GL_RED };
    glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG8, 512,512, 0, GL_RED, GL_UNSIGNED_BYTE, temp_bitmap);
}

Vec2 getTextDimensions(const char *text, FontAtlas *font) {
    float w = 0;
    float h = 0;
    float linemaxheight = 0;
    float maxwidth = 0;
    unsigned char u;

    while ((u = *text)) {
        text++;
        if (u == '\n') {
            h += linemaxheight;
            w = 0;
            linemaxheight = 0;
        } else {
            stbtt_aligned_quad q;
            float xadv = 0;
            float yadv = 0;
            stbtt_GetBakedQuad(font->cdata, 512,512, u-32, &xadv,&yadv,&q,1);//1=opengl & d3d10+,0=d3d9
            w += xadv;
            if ((q.y1-q.y0+yadv) > linemaxheight) {
                linemaxheight = (q.y1-q.y0+yadv);
            }
            if (w > maxwidth) {
                maxwidth = w;
            }
        }
    }
    h += linemaxheight;
    return vec2(maxwidth, h);
}

Vec2 getTextCenterOffset(const char *text, FontAtlas *font) {
    Vec2 v = getTextDimensions(text, font);
    v.x /= 2;
    v.y /= -2;
    return v;
}
