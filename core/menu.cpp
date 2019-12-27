#include "core.h"

MenuLine *addMenuLine(MenuContext *ctx, MenuItemType t, char *label) {
    MenuLine line;
    line.type = t;
    line.label = label;
    array_push(ctx->lines, line);
    return &array_last(ctx->lines);
}

MenuLine *addMenuLine(MenuContext *ctx, char *label) {
    return addMenuLine(ctx, HEADING, label);
}

MenuLine *addMenuLine(MenuContext *ctx, char *label, bool *b) {
    MenuLine *l = addMenuLine(ctx, TOGGLE_VALUE, label);
    l->toggle = b;
    return l;
}

MenuLine *addMenuLine(MenuContext *ctx, char *label, SDL_Keycode *key) {
    MenuLine *l = addMenuLine(ctx, KEYBINDING_VALUE, label);
    l->key.waiting = false;
    l->key.current= key;
    return l;
}

void menuLineText(MenuContext *ctx, char *buf, int n, MenuLine item) {
    int alignWidth = ctx->alignWidth;
    const char *pad = "................................................";
    switch (item.type) {
    case HEADING:
    {
        snprintf(buf, n, "%s", item.label);
    } break;
    case TOGGLE_VALUE:
    {
        char *v = (char*)(*item.toggle ? "on" : "off");
        int p = 0;
        int w = snprintf(NULL, 0, "%s: %s", item.label, v);
        if (alignWidth != -1 && w < alignWidth) {
            p = alignWidth - w;
        }
        snprintf(buf, n, "%s: %.*s%s", item.label, p, pad, v);
    } break;
    case KEYBINDING_VALUE:
    {
        char *v = (char*)(item.key.waiting ? "press a key" : SDL_GetKeyName(*item.key.current));
        int p = 0;
        int w = snprintf(NULL, 0, "%s: %s", item.label, v);
        if (alignWidth != -1 && w < alignWidth) {
            p = alignWidth - w;
        }
        snprintf(buf, n, "%s: %.*s %s", item.label, p, pad, v);
    } break;
    default:
        assert(false);
    }
}

Rect menuLineDimensions(MenuContext *ctx, MenuLine item) {
    Rect r;
    r.x = 0;
    r.y = 0;
    char s[255];
    menuLineText(ctx, s, sizeof(s), item);
    r.box = getTextDimensions(s, ctx->font);
    return r;
}

int prevHotLine(MenuContext *ctx) {
    if (ctx->interaction != NONE) return ctx->hotIndex;
    int n = array_len(ctx->lines);
    for (int i = 1; i < n; i++) {
        int j = (ctx->hotIndex-i+n) % n;
        if (ctx->lines[j].type != HEADING) {
            ctx->hotIndex = j;
            break;
        }
    }
    return ctx->hotIndex;
}

int nextHotLine(MenuContext *ctx) {
    if (ctx->interaction != NONE) return ctx->hotIndex;
    int n = array_len(ctx->lines);
    for (int i = 1; i < n; i++) {
        int j = (ctx->hotIndex+i) % n;
        if (ctx->lines[j].type != HEADING) {
            ctx->hotIndex = j;
            break;
        }
    }
    return ctx->hotIndex;
}

void menuInteract(MenuContext *ctx, MenuLine *item) {
    ctx->interaction = NONE;
    if (item) {
        switch (item->type) {
        case TOGGLE_VALUE:
            *item->toggle = !*item->toggle;
            break;
        case KEYBINDING_VALUE:
            ctx->interaction = KEYBINDING;
            item->key.waiting = true;
            break;
        default:
            assert(false);
        }
    }
}

void drawMenu(Renderer *r, MenuContext *ctx, DrawOpts2d hotOpts) {
    DrawOpts2d opts = {};
    Vec2 pos = ctx->topCenter;
    int n = array_len(ctx->lines);
    for (int i = 0; i < n; i++) {
        bool hot = (i == (ctx->hotIndex % n));

        char s[255];
        menuLineText(ctx, s, sizeof(s), ctx->lines[i]);

        Vec2 box = getTextDimensions(s, ctx->font);
        Rect dim;
        dim.box = box;
        dim.x = pos.x - dim.w/2;
        dim.y = pos.y - dim.h;
        drawRectOutline(r, dim, red);
        drawTextCentered(r, ctx->font, pos.x, pos.y, s, hot ? hotOpts : opts);
        pos.y += ctx->lineHeight; 
    }
}
