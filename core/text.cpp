#include "core.h"

void loadFontAtlas(FontAtlas *font, const unsigned char *font_data, float size) {
    stbtt_InitFont(&font->info, font_data, stbtt_GetFontOffsetForIndex(font_data,0));

    rebakeFont(font, font_data, size);
}

void rebakeFont(FontAtlas *font, const unsigned char *font_data, float size) {
    unsigned char temp_bitmap[512*512];
    stbtt_BakeFontBitmap(font_data, 0, size, temp_bitmap, 512, 512, 32, ATLAS_CHAR_COUNT, font->cdata); // no guarantee this fits!

    unsigned char expanded[4*512*512];
    for (int i = 0; i < 512*512; i++) {
        expanded[4*i+0] = temp_bitmap[i];
        expanded[4*i+1] = temp_bitmap[i];
        expanded[4*i+2] = temp_bitmap[i];
        expanded[4*i+3] = temp_bitmap[i];
    }

    glGenTextures(1, &font->tex);
    glBindTexture(GL_TEXTURE_2D, font->tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(
        GL_TEXTURE_2D,      // target texture
        0,                  // level of detail (0=base)
        GL_RGBA8,           // internal format (color component count)
        512, 512,           // width * height
        0,                  // border... must be 0
        GL_RGBA,            // format of pixel data
        GL_UNSIGNED_BYTE,   // data type of each pixel
        expanded            // pointer to data
    );
    glBindTexture(GL_TEXTURE_2D, 0);

    font->size = size;

    font->scale = stbtt_ScaleForPixelHeight(&font->info, size);
    int asc, desc, gap;
    stbtt_GetFontVMetrics(&font->info, &asc, &desc, &gap);
    font->lineHeight = font->scale * (float)(asc - desc + gap);
}

Vec2 getTextDimensions(const char *text, FontAtlas *font) {
    float w = 0;
    float h = 0;
    float maxwidth = 0;
    unsigned char u;

    while ((u = *text)) {
        text++;
        if (u == '\n') {
            h += font->lineHeight * font->lineHeightScale;
            w = 0;
        } else {
            stbtt_aligned_quad q;
            float xadv = 0;
            float yadv = 0;
            stbtt_GetBakedQuad(font->cdata, 512,512, u-32, &xadv,&yadv,&q,1);//1=opengl & d3d10+,0=d3d9
            w += xadv;
            if (w > maxwidth) {
                maxwidth = w;
            }
        }
    }
    h += font->lineHeight * font->lineHeightScale;
    return vec2(maxwidth, h);
}

Vec2 getTextCenterOffset(const char *text, FontAtlas *font) {
    Vec2 v = getTextDimensions(text, font);
    v.x /= 2;
    v.y /= -2;
    return v;
}
