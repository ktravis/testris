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

MenuLine *addMenuLine(MenuContext *ctx, char *label, MenuButton *btn) {
    MenuLine *l = addMenuLine(ctx, BUTTON, label);
    l->btn = btn;
    return l;
}

void menuLineText(MenuContext *ctx, char *buf, int n, MenuLine item) {
    int alignWidth = ctx->alignWidth;
    const char *pad = "................................................";
    switch (item.type) {
    case BUTTON: // fallthrough
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

Rect menuLineDimensions(MenuContext *ctx, MenuLine item, Vec2 pos) {
    Rect r;
    r.x = 0;
    r.y = 0;
    char s[255];
    menuLineText(ctx, s, sizeof(s), item);
    r.box = getTextDimensions(s, ctx->font);
    r.pos = pos;
    switch (ctx->alignment) {
    case TextAlignment::CENTER:
        r.x -= r.w/2;
        r.y -= r.h/2;
        break;
    case TextAlignment::TOPLEFT:
        //r.y -= r.h;
        break;
    }
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

MenuButton *menuInteract(MenuContext *ctx, MenuLine *item) {
    ctx->interaction = NONE;
    if (item) {
        switch (item->type) {
        case BUTTON:
            return item->btn;
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
    return 0;
}

DrawOpts2d defaultHotOpts(float t) {
    DrawOpts2d opts = {};
    float z = 1.05f + 0.05f * cos(t/200);
    opts.tint = red;
    opts.scale.x = z;
    opts.scale.y = z;
    return opts;
}

MenuButton *updateMenu(MenuContext *ctx, InputData in) {
    if (ctx->interaction == NONE && in.mouseMoved) {
        ctx->hotIndex = -1;
        Vec2 pos = ctx->topCenter;
        for (int i = 0; i < array_len(ctx->lines); i++) {
            pos.y = ctx->topCenter.y + i * ctx->lineHeight;
            MenuLine *item = &ctx->lines[i];
            if (item->type == HEADING) {
                continue;
            }
            Rect dim = menuLineDimensions(ctx, *item, pos);
            if (contains(dim, in.mouse)) {
                ctx->hotIndex = i;
                break;
            }
        }
    }

    MenuLine *item = 0;
    if (ctx->hotIndex != -1) item = &ctx->lines[ctx->hotIndex];

    if (item && in.lmb.down) {
        return menuInteract(ctx, item);
    }
    for (int i = 0; i < in.numKeyEvents; i++) {
        KeyEvent e = in.keys[(in.keyEventBufferOffset + i) % KEY_EVENT_BUFCOUNT];
        if (!e.state.down)
            continue;

        switch (ctx->interaction) {
        case KEYBINDING:
            {
                assert(item);
                if (e.key != SDLK_ESCAPE) {
                    *item->key.current = e.key;
                }
                ctx->interaction = NONE;
                item->key.waiting = false;
                return 0;
            } break;
        default: break;
        }

        switch (e.key) {
        case SDLK_UP:
            prevHotLine(ctx);
            break;
        case SDLK_DOWN:
            nextHotLine(ctx);
            break;
        case SDLK_RETURN:
            return menuInteract(ctx, item);
        }
    }
    return 0;
}

void drawMenu(Renderer *r, MenuContext *ctx, DrawOpts2d hotOpts) {
    DrawOpts2d opts = {};
    Vec2 pos = ctx->topCenter;
    int n = array_len(ctx->lines);
    for (int i = 0; i < n; i++) {
        bool hot = (i == (ctx->hotIndex % n));

        char s[255];
        menuLineText(ctx, s, sizeof(s), ctx->lines[i]);

        // debug box
        //Rect dim = menuLineDimensions(ctx, ctx->lines[i], pos);
        //drawRectOutline(r, dim, red);

        switch (ctx->alignment) {
        case TextAlignment::CENTER:
            drawTextCentered(r, ctx->font, pos.x, pos.y, s, hot ? hotOpts : opts);
            break;
        case TextAlignment::TOPLEFT:
            drawText(r, ctx->font, pos.x, pos.y, s, hot ? hotOpts : opts);
            break;
        }
        pos.y += ctx->lineHeight; 
    }
}
