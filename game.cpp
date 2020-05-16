#include "game.h"

#include "pieces.cpp"

// TODO:
// - spawn delay
// - do scoring right
// - screen shake
// - score indicators
// - animated backgrounds
// - score animation
// - game over animation
// - save the high score somewhere
// - sounds
// - music

FontAtlas ubuntu_m16;
FontAtlas ubuntu_m32;
FontAtlas mono_m18;

int32_t sound_ready;

ShaderProgram titleShader;
ShaderProgram titleBGShader;

Piece faller;

bool inBounds(int x, int y) {
    return x >= 0 && x < BOARD_WIDTH && y < BOARD_HEIGHT; // y >= 0
}

int rotate(Piece *p, RotationDir dir) {
    switch (dir) {
    case CW:
        p->orientation = (p->orientation + 1) % NUM_ORIENTATIONS;
        break;
    case CCW:
        if (p->orientation == 0) {
            p->orientation = NUM_ORIENTATIONS;
        }
        p->orientation--;
        break;
    }
    // TODO: check orientation && pos against piece extent, shift as needed
    return p->orientation;
}

bool pieceFits(GameState *st, int type, int o, int x, int y, Vec2 *nudge) {
    for (int ix = 0; ix < 4; ix++) {
        int iy = 0;
        int i = ix + iy * 4;
        if (y + iy < 0) continue;
        if (!pieces[type][o][i]) continue;
        if (st->board[y+iy][x+ix].full) {
            return false;
        }
        if (!inBounds(x+ix, y+iy)) {
            if (nudge) {
                if (x+ix < 0) *nudge = vec2(-(x+ix), 0);
                else if (x+ix >= BOARD_WIDTH) *nudge = vec2(x+ix-BOARD_WIDTH-1, 0);
                //else if (y+iy < 0) *nudge = vec2(0, -(y+iy));
                else if (y+iy >= BOARD_HEIGHT) *nudge = vec2(0, y+iy-BOARD_HEIGHT-1);
            }
            return false;
        }
    }
    for (int ix = 0; ix < 4; ix++) {
        int iy = 3;
        int i = ix + iy * 4;
        if (y + iy < 0) continue;
        if (!pieces[type][o][i]) continue;
        if (st->board[y+iy][x+ix].full) {
            return false;
        }
        if (!inBounds(x+ix, y+iy)) {
            if (nudge) {
                if (x+ix < 0) *nudge = vec2(-(x+ix), 0);
                else if (x+ix >= BOARD_WIDTH) *nudge = vec2(BOARD_WIDTH-1-x-ix, 0);
                //else if (y+iy < 0) *nudge = vec2(0, -(y+iy));
                else if (y+iy >= BOARD_HEIGHT) *nudge = vec2(0, BOARD_HEIGHT-1-y-iy);
            }
            return false;
        }
    }
    for (int jx = 3; jx < 3+4; jx++) {
        int ix = jx % 4;
        for (int iy = 1; iy < 3; iy++) {
            if (y + iy < 0) continue;
            int i = ix + iy * 4;

            if (!pieces[type][o][i]) continue;
            if (st->board[y+iy][x+ix].full) {
                return false;
            }
            if (!inBounds(x+ix, y+iy)) {
                if (nudge) {
                    if (x+ix < 0) *nudge = vec2(-(x+ix), 0);
                    else if (x+ix >= BOARD_WIDTH) *nudge = vec2(BOARD_WIDTH-1-x-ix, 0);
                    //else if (y+iy < 0) *nudge = vec2(0, -(y+iy));
                    else if (y+iy >= BOARD_HEIGHT) *nudge = vec2(0, BOARD_HEIGHT-1-y-iy);
                }
                return false;
            }
        }
    }
    return true;
}

bool pieceFits(GameState *st, int type, int o, int x, int y) {
    return pieceFits(st, type, o, x, y, NULL);
}

bool pieceFits(GameState *st, Piece *p) {
    return pieceFits(st, p->type, p->orientation, p->x, p->y, NULL);
}

bool pieceFits(GameState *st, Piece *p, Vec2 *nudge) {
    return pieceFits(st, p->type, p->orientation, p->x, p->y, nudge);
}

bool tryMove(GameState *st, Piece *p, int dx, int dy) {
    if (!pieceFits(st, p->type, p->orientation, p->x + dx, p->y + dy))
        return false;
    p->x += dx;
    p->y += dy;
    return true;
}

bool tryRotate(GameState *st, Piece *p, RotationDir dir) {
    int o = p->orientation;
    switch (dir) {
    case CW:
        o = (p->orientation + 1) % NUM_ORIENTATIONS;
        break;
    case CCW:
        if (p->orientation == 0) {
            o = NUM_ORIENTATIONS;
        }
        o--;
        break;
    }
    Vec2 nudge = {};
    if (pieceFits(st, p->type, o, p->x, p->y, &nudge)) {
        p->orientation = o;
        return true;
    }
    if (nudge.x != 0 || nudge.y != 0) {
        if (!pieceFits(st, p->type, o, p->x+nudge.x, p->y+nudge.y)) {
            return false;
        }
        p->x += nudge.x;
        p->y += nudge.y;
    } else {
        Vec2 cwtries[] = {
            {-1, 0},
            {0, -1},
            {-2, 0},
            {0, -2},
            {1, 0},
            {0, 1},
            {2, 0},
            {0, 2},
        };
        Vec2 ccwtries[] = {
            {1, 0},
            {0, -1},
            {2, 0},
            {0, -2},
            {-1, 0},
            {0, 1},
            {-2, 0},
            {0, 2},
        };
        for (int i = 0; i < (int)(sizeof(cwtries)/sizeof(Vec2)); i++) {
             Vec2 v = (dir == CW ? cwtries : ccwtries)[i];
             if (pieceFits(st, p->type, o, p->x+v.x, p->y+v.y)) {
                 p->x += v.x;
                 p->y += v.y;
                 goto hooray;
             }
        }
        return false;
    }
hooray:
    p->orientation = o;
    return true;
}

DEFINE_BUTTON(MainMenu_StartGameButton);
DEFINE_BUTTON(MainMenu_OptionsButton);
DEFINE_BUTTON(MainMenu_HighScoresButton);
DEFINE_BUTTON(MainMenu_QuitButton);

void initTitleMenu(GameState *st) {
    MenuContext *menu = &st->titleMenu;
    menu->font = &ubuntu_m32;
    menu->lineHeight = 48;
    menu->lines = 0;
    menu->alignWidth = 24;
    menu->topCenter = vec2(st->width/2, st->height/2 + 140);
    addMenuLine(menu, (char *)"start a game", MainMenu_StartGameButton);
    addMenuLine(menu, (char *)"options", MainMenu_OptionsButton);
    addMenuLine(menu, (char *)"high scores", MainMenu_HighScoresButton);
    addMenuLine(menu, (char *)"quit", MainMenu_QuitButton);

    menu->hotIndex = 1;
}

void initOptionsMenu(GameState *st) {
    MenuContext *menu = &st->options;
    menu->font = &mono_m18;
    menu->lines = 0;
    menu->alignWidth = 24;
    menu->topCenter = vec2(st->width/2, 200);
    addMenuLine(menu, (char *)"[ keys ]");
    addMenuLine(menu, (char *)"left", &st->settings.controls.left);
    addMenuLine(menu, (char *)"right", &st->settings.controls.right);
    addMenuLine(menu, (char *)"down", &st->settings.controls.down);
    addMenuLine(menu, (char *)"drop", &st->settings.controls.drop);
    addMenuLine(menu, (char *)"store", &st->settings.controls.store);
    addMenuLine(menu, (char *)"rotate cw", &st->settings.controls.rotateCW);
    addMenuLine(menu, (char *)"rotate ccw", &st->settings.controls.rotateCCW);
    addMenuLine(menu, (char *)"pause", &st->settings.controls.pause);
    addMenuLine(menu, (char *)"mute", &st->settings.controls.mute);

    addMenuLine(menu, (char *)"");
    addMenuLine(menu, (char *)"");

    addMenuLine(menu, (char *)"[ settings ]");
    addMenuLine(menu, (char *)"muted", &st->settings.muted);
    addMenuLine(menu, (char *)"ghost", &st->settings.showGhost);

    menu->hotIndex = 1;
}

#define KEY_FIELD(key) FIELD_FN(controls.key, "%s", SDL_GetKeyName, SDL_GetKeyFromName)

DEFINE_SERDE(Settings,
    KEY_FIELD(left),
    KEY_FIELD(right),
    KEY_FIELD(down),
    KEY_FIELD(drop),
    KEY_FIELD(store),
    KEY_FIELD(rotateCW),
    KEY_FIELD(rotateCCW),
    KEY_FIELD(pause),
    KEY_FIELD(mute),
    BOOL_FIELD(muted),
    BOOL_FIELD(showGhost)
)

bool loadSettings(Settings *s, const char *filename) {
    uint8_t *buf;
    if (!(buf = readFile(filename))) {
        return false;
    }
    deserializeSettings(s, buf);
    free(buf);
    return true;
}

bool saveSettings(Settings *s, const char *filename) {
    uint8_t *buf = serializeSettings(s);
    bool result = writeFile(filename, buf);
    free(buf);
    return result;
}

bool compileExtraShaders(Renderer *r) {
    uint8_t *shaderBuf = readFile("assets/shaders/title.vs");
    if (!shaderBuf) {
        return false;
    }

    if (!compileShader(&titleShader.vert, shaderBuf, GL_VERTEX_SHADER)) {
        return false;
    }
    shaderBuf = readFile("assets/shaders/title.fs");
    if (!shaderBuf) {
        return false;
    }

    if (!compileShader(&titleShader.frag, shaderBuf, GL_FRAGMENT_SHADER)) {
        return false;
    }

    if (!compileShaderProgram(&titleShader)) {
        return false;
    }

    shaderBuf = readFile("assets/shaders/bg.fs");
    if (!shaderBuf) {
        return false;
    }

    titleBGShader.vert = r->defaultShader.vert;
    if (!compileShader(&titleBGShader.frag, shaderBuf, GL_FRAGMENT_SHADER)) {
        return false;
    }

    if (!compileShaderProgram(&titleBGShader)) {
        return false;
    }
    Vec2 dim = vec2(r->screenWidth, r->screenHeight);
    glUniform2fv(glGetUniformLocation(titleBGShader.handle, "dim"), 1, (GLfloat*)&dim);
    return true;
}

bool startGame(GameState *st, Renderer *r) {
    uint8_t *ttf_buffer;
    if (!(ttf_buffer = readFile("./assets/fonts/Ubuntu-M.ttf"))) {
        return false;
    }
    loadFontAtlas(&ubuntu_m32, ttf_buffer, 32.0f);
    loadFontAtlas(&ubuntu_m16, ttf_buffer, 16.0f);
    free(ttf_buffer);
    
    if (!(ttf_buffer = readFile("./assets/fonts/DejaVuSansMono-Bold.ttf"))) {
        return false;
    }
    loadFontAtlas(&mono_m18, ttf_buffer, 18.0f);
    free(ttf_buffer);

    sound_ready = loadAudioAndConvert("./assets/sounds/ready.wav");
    if (sound_ready == -1) { return false; }

    GameState init = {};
    // is this stupid? probably
    init.width = st->width;
    init.height = st->height;
    *st = init;

    if (!loadSettings(&st->settings, "~/.testris.conf")) {
        log("settings not found, saving defaults");
        if (!saveSettings(&st->settings, "~/.testris.conf")) {
            log("settings save failed");
        }
    }

    st->scenes[st->currentScene] = TITLE;

    //int32_t sound_hiscore = loadAudioAndConvert("assets/sounds/hiscore.wav");
    //if (sound_hiscore == -1) { return NULL; }
    
    initTitleMenu(st);
    initOptionsMenu(st);

    if (!compileExtraShaders(r))
        return false;

    return true;
}

int firstFilled(int type, int o) {
    for (int i = 0; i < PIECE_BLOCK_TOTAL; i++) {
        if (pieces[type][o][i])
            return i;
    }
    return -1;
}

int firstFilledRow(int type, int o) {
    return firstFilled(type, o)/4;
}

#define swap(a, b) { int _x = *a; *a = *b; *b = _x; }

bool spawnFaller(GameState *st) {
    faller.orientation = 0;
    faller.type = st->queue[0];
    faller.x = 3;
    faller.y = -(firstFilledRow(faller.type, faller.orientation));
    for (int i = 0; i < QUEUE_SIZE-1; i++)
        st->queue[i] = st->queue[i+1];
    st->queueRemaining--;
    if (st->queueRemaining <= 2*NUM_PIECES) {
        int base[NUM_PIECES];
        int n = NUM_PIECES;
        for (int i = 0; i < n; i++) {
            base[i] = i;
        }
        // fill the back half of the queue
        for (int x = 0; x < 2; x++) {
            // fisher-yates shuffle
            for (int i = n-1; i > 0; i--) { 
                int j = randn(i+1); 
                swap(&base[i], &base[j]);
            } 
            // copy in
            for (int i = 0; i < n; i++) {
                st->queue[st->queueRemaining++] = base[i];
            }
        }
    }
    if (!pieceFits(st, &faller)) {
        return false;
    }
    return true;
}

void setScene(GameState *st, Scene s) {
    st->scenes[st->currentScene] = s;
}

void pushScene(GameState *st, Scene s) {
    assert(st->currentScene+1 < (int)(sizeof(st->scenes)/sizeof(Scene)));
    st->scenes[++st->currentScene] = s;
}

void popScene(GameState *st) {
    assert(st->currentScene > 0);
    st->currentScene--;
}

Transition no_transition;

Transition *transition(GameState *st, Transition::Type tp, Scene to, float duration) {
    Transition t = {};
    t.type = tp;
    t.to = to;
    t.duration = duration;
    st->transition = t;
    return &st->transition;
}

void startRound(GameState *st) {
    st->score = 0;
    st->stored = -1;
    st->canStore = true;

    int board_width_px = (BOARD_WIDTH * BLOCK_SIDE + (BOARD_WIDTH + 1) * BLOCK_PAD);
    int board_height_px = (BOARD_HEIGHT * BLOCK_SIDE + (BOARD_HEIGHT + 1) * BLOCK_PAD);

    Vec2 ul_corner = {
        .x = st->width/2.0f - board_width_px / 2.0f,
        .y = st->height/2.0f - board_height_px / 2.0f,
    };

    st->border = Rect{
        .pos = ul_corner,
        .box = vec2(board_width_px, board_height_px), 
    };

    for (int y = 0; y < BOARD_HEIGHT; y++)
        for (int x = 0; x < BOARD_WIDTH; x++)
            st->board[y][x] = (Cell){.full = false};

    for (int i = 0; i < MAX_BLOCKS; i++)
        st->blocks[i].inUse = false;

    int base[NUM_PIECES];
    int n = NUM_PIECES;
    for (int i = 0; i < n; i++) {
        base[i] = i;
    }
    // fill the entire queue
    for (int x = 0; x < 4; x++) {
        // fisher-yates shuffle
        for (int i = n-1; i > 0; i--) { 
            int j = randn(i+1); 
            swap(&base[i], &base[j]);
        } 
        // copy in
        for (int i = 0; i < n; i++) {
            st->queue[st->queueRemaining++] = base[i];
        }
    }

    spawnFaller(st);
}

void transitionStartRound(GameState *st) {
    transition(st, Transition::CHECKER_IN_OUT, IN_ROUND, 1500);
    st->transition.fn = startRound;
}

float tickInterval = 1500.0f;

Vec2 gridBlockPos(GameState *st, int x, int y) {
    return vec2(
        (x + 0.5) * BLOCK_SIDE + (x + 1) * BLOCK_PAD + st->border.x,
        (y + 0.5) * BLOCK_SIDE + (y + 1) * BLOCK_PAD + st->border.y
    );
}

void tryClearingLine(GameState *st, int line) {
    Cell *row = st->board[line];
    for (int x = 0; x < BOARD_WIDTH; x++) {
        if (!row[x].full) {
            debug("U WUT");
        }
        row[x].full = false;
        Block *b = &st->blocks[row[x].blockIndex];

        // fly off board
        b->onBoard = false;
        b->timeLeft = 1200.0f;
        b->rotv = randf(8.0f) - 4.0f;
        b->vel.x = randf(9.0f) - 4.5f;
        b->vel.y = randf(-7.5f) - 0.5f;

        for (int y = line-1; y >= 0; y--) {
            st->board[y+1][x] = st->board[y][x];
            if (!st->board[y][x].full) continue;
            st->blocks[st->board[y+1][x].blockIndex].targetPos = gridBlockPos(st, x, y+1);
            st->blocks[st->board[y+1][x].blockIndex].tweenStep = 0;
        }
        st->board[0][x].full = false;
    }
}

void clearLines(GameState *st) {
    for (int y = 0; y < BOARD_HEIGHT; y++) {
        bool all = true;
        for (int x = 0; x < BOARD_WIDTH; x++) {
            if (!st->board[y][x].full) {
                all = false;
                break;
            }
        }
        if (!all) {
            continue;
        }
        tryClearingLine(st, y);
        st->score++;
        if (st->score > st->hiscore) {
            st->hiscore = st->score;
        }
    }
}

int placeBlock(GameState *st, Block b) {
    int idx;
    for (int i = 0; i < MAX_BLOCKS; i++) {
        idx = (i + st->lastBlockInUse) % MAX_BLOCKS;
        if (!st->blocks[idx].inUse)
            break;
        // fall through if we run out
    }
    st->blocks[idx] = b;
    st->lastBlockInUse = idx;
    return idx;
}

void placePiece(GameState *st, Piece *p) {
    for (int i = 0; i < PIECE_BLOCK_TOTAL; i++) {
        if (!pieces[p->type][p->orientation][i]) continue;
        Block b = {0};
        b.inUse = true; 
        b.onBoard = true; 
        b.color = colors[p->type];
        int x = p->x+i%4;
        int y = p->y+i/4;
        b.targetPos = gridBlockPos(st, x, y);
        b.pos = b.targetPos;
        st->board[y][x].full = true;
        st->board[y][x].blockIndex = placeBlock(st, b);
        //int idx = placeBlock(st, b);
        //st->board[y][x].block = &st->blocks[idx];
    }
    clearLines(st);
    st->canStore = true;
}

// old == false, new == true
bool updateTransition(Transition *t, InputData in) {
    t->t += in.dt;
    t->z = t->t/t->duration;
    // TODO:
    //t->z = t->easing(t->z);
    switch (t->type) {
    case Transition::FADE_IN_OUT:
    case Transition::ROWS_ACROSS:
    case Transition::CHECKER_IN_OUT:
    case Transition::CHECKER_IN_OUT_ROTATE:
        t->z = t->z;
        break;
    default:
        break;
    }
    if (t->z >= 1) 
        *t = no_transition;
    return t->z > 0.5f;
}

void updateGameState(GameState *st, InputData in) {
    // only update the fps once a second so it's (slightly) smoother
    static float acc = 0;
    acc += in.dt;
    if (acc > 1000.0f) {
        acc = 0;
        st->fps = 1000.0f/in.dt;
    }
    st->t += in.dt;

    if (in.quit) {
        st->exitNow = true;
        return;
    }

    Transition *t = &st->transition;

    if (t->type == Transition::NONE) {
        updateScene(st, in, st->scenes[st->currentScene]);
    } else {
        if (updateTransition(t, in)) {
            if (t->fn) {
                t->fn(st);
                t->fn = NULL;
            }
            setScene(st, t->to);
        }
    }
}

void renderBoard(Renderer *r, GameState *st) {
    for (int i = 0; i < MAX_BLOCKS; i++) {
        if (!st->blocks[i].inUse) continue;
        Block *b = &st->blocks[i];
        DrawOpts2d opts;
        Rect rx = rect(b->pos, BLOCK_SIDE, BLOCK_SIDE);
        opts.origin = scaled(rx.box, 0.5);
        opts.tint = b->color;
        opts.rotation = b->rotation;
        drawRect(r, rx, opts);
    }

    // faller
    for (int i = 0; i < PIECE_BLOCK_TOTAL; i++) {
        if (!pieces[faller.type][faller.orientation][i]) {
            continue;
        }
        int x = i % 4;
        int y = i / 4;
        if (!inBounds(faller.x+x, faller.y+y) || faller.y+y < 0) {
            continue;
        }
        
        Rect rx = rect(gridBlockPos(st, faller.x+x, faller.y+y), BLOCK_SIDE, BLOCK_SIDE);
        rx.pos.x -= BLOCK_SIDE/2;
        rx.pos.y -= BLOCK_SIDE/2;
        drawRect(r, rx, colors[faller.type]);
    }

    if (st->stored != -1) {
        bool anyInRow = false;

        float side = BLOCK_SIDE * 0.7f;
        float pad = BLOCK_PAD * 0.7f;
        Vec2 v = vec2(16, 200);
        Vec2 box = vec2(side, side);
        int y = 0;
        for (int i = 0; i < PIECE_BLOCK_TOTAL; i++) {
            int x = i % 4;
            if (x == 0 && anyInRow) {
                y++;
            }
            if (!pieces[st->stored][0][i]) {
                continue;
            }
            anyInRow = true;
            
            Vec2 p = vec2(
                (x * (side + pad) + v.x),
                (y * (side + pad) + v.y)
            );
            drawRect(r, rect(p, box), colors[st->stored]);
        }
    }
}

void renderGhost(Renderer *r, GameState *st) {
    Piece cp = faller; 
    //Color c = multiply(colors[cp.type], 1.70);
    Color c = colors[cp.type];
    while (tryMove(st, &cp, 0, 1));
    for (int i = 0; i < PIECE_BLOCK_TOTAL; i++) {
        if (!pieces[cp.type][cp.orientation][i]) {
            continue;
        }
        int x = i % 4;
        int y = i / 4;
        if (!inBounds(cp.x+x, cp.y+y) || cp.y+y < 0) {
            continue;
        }
        
        Rect rx = rect(gridBlockPos(st, cp.x+x, cp.y+y), BLOCK_SIDE, BLOCK_SIDE);
        rx.pos.x -= BLOCK_SIDE/2;
        rx.pos.y -= BLOCK_SIDE/2;
        drawRectOutline(r, rx, c, 2.0f);
    }
}

void renderGameOver(Renderer *r, GameState *st) {
    drawTextCentered(r, &ubuntu_m32, st->width/2, st->height/2, "OOPS");

    drawTextCentered(r, &ubuntu_m16, st->width/2, st->height/2+128, "( r to restart )");
}

void renderScore(Renderer *r, GameState *st) {
    drawText(r, &ubuntu_m32, 16, 48, "score:");
    char buf[5];
    snprintf(buf, sizeof(buf), "%04d", st->score);
    drawText(r, &ubuntu_m32, 16, 84, buf);

    drawText(r, &ubuntu_m32, 16, st->height-84, "hi:");
    snprintf(buf, sizeof(buf), "%04d", st->hiscore);
    drawText(r, &ubuntu_m32, 16, st->height-48, buf);
}

void renderQueue(Renderer *r, GameState *st) {
    const int VISIBLE_QUEUE_BLOCKS = 5;

    float side = BLOCK_SIDE * 0.7f;
    float pad = BLOCK_PAD * 0.7f;

    Vec2 v = vec2(st->width - 110, 80);
    Vec2 box = vec2(side, side);
    int y = 0;
    for (int q = 0; q < VISIBLE_QUEUE_BLOCKS; q++) {
        bool anyInRow = false;
        for (int i = 0; i < PIECE_BLOCK_TOTAL; i++) {
            int t = st->queue[q];
            int x = i % 4;
            if (x == 0 && anyInRow) {
                y++;
            }
            if (!pieces[t][0][i]) {
                continue;
            }
            anyInRow = true;
            
            Vec2 p = vec2(
                (x * (side + pad) + v.x),
                (y * (side + pad) + v.y)
            );
            drawRect(r, rect(p, box), colors[t]);
        }
        y++;
    }
}

bool opaque(Scene s) {
    switch (s) {
    // add more here
    default:
        return true;
    }
}

void renderGameState(Renderer *r, GameState *st) {
    int i = st->currentScene;
    while (i > 0 && !opaque(st->scenes[i])) {
        i--;
    }
    for (; i <= st->currentScene; i++) {
        renderScene(r, st, st->scenes[i]);
    }

    if (st->transition.type != Transition::NONE) {
        renderTransition(r, st);
    }

#ifdef DEBUG
    char buf[6];
    snprintf(buf, sizeof(buf), "%02.f", st->fps);
    drawTextCentered(r, &mono_m18, r->screenWidth - 50.0f, 24, buf);
#endif
}

void updateTitle(GameState *st, InputData in) {
    MenuButton *btn = updateMenu(&st->titleMenu, in);
    if (btn == MainMenu_StartGameButton) {
        transitionStartRound(st);
    } else if (btn == MainMenu_OptionsButton) {
        // TODO(ktravis): would like to do this instead, but it doesn't "push
        // scene", so escaping from the options menu crashes
        //transition(st, Transition::CHECKER_IN_OUT, OPTIONS, 1500);
        pushScene(st, OPTIONS);
    } else if (btn == MainMenu_HighScoresButton) {
    } else if (btn == MainMenu_QuitButton) {
        st->exitNow = true;
    }
}

void renderTitle(Renderer *r, GameState *st) {
    const char title[] = 
        "oM                        oM                          \n"
        "M@                        M@        .     M@:         \n"
        "M@iii     :o:      .ioi   M@iii    :ioi    .     .ioi \n"
        "M@@@@    M@@@M:   o@@@@.  M@@@@   M@@@@.  :i    o@@@@.\n"
        "M@:..   V@o :@@   @M      M@:..   M@: .   V@.   @M    \n"
        "M@      @@   V@.  @@:     M@      M@.     V@.   @@:   \n"
        "M@     .@@@@@@@:  o@@M:   M@      M@.     V@.   o@@M: \n"
        "M@     .@@iiiii.   .V@@i  M@      M@.     V@.    .V@@i\n"
        "M@:     @@:          :@V  M@:     M@.     V@.      :@V\n"
        "o@MoV.  i@@Vooi  .ViiV@i  o@MoV.  M@.     V@.  .ViiV@i\n"
        " M@@@:   i@@@@V  :@@@@V    M@@@:  M@.     V@.  :@@@@V \n"
        "                    ..                           ..   ";

    DrawOpts2d bgOpts = {};
    bgOpts.shader = &titleBGShader;
    drawRect(r, rect(0, 0, r->screenWidth, r->screenHeight), bgOpts);

    const int N = 6 * sizeof(title)/sizeof(char);
    VertexData vbuf[N];

    DrawOpts2d opts = {};
    opts.meshBuffer = Mesh{.count = N, .data = vbuf};
    opts.shader = &titleShader;

    drawTextCentered(r, &mono_m18, r->screenWidth/2, r->screenHeight/2 - 150, title, opts);
    //Vec2 dim = getTextDimensions(title, &mono_m18);
    //opts.origin.x = dim.x / 2;
    //opts.origin.y = -dim.y / 2;
    //drawText(r, &mono_m18, r->screenWidth/2, 0, title, opts);
    drawMenu(r, &st->titleMenu, defaultHotOpts(st->t));
}

float moveRate = 12.0f;
float maxMoveDelayMillis = 1000.0f/moveRate; 
float moveStartDelayMillis = 225.0f;

void gameOver(GameState *st) {
    transition(st, Transition::ROWS_ACROSS, GAME_OVER, 1500);
}

void updateInRound(GameState *st, InputData in) {
    for (int i = 0; i < in.numKeyEvents; i++) {
        KeyEvent e = keyEvent(&in, i);
        if (e.state.down) {
            if (e.key == st->settings.controls.mute) {
                toggleMute();
            } else if (e.key == st->settings.controls.pause) {
                st->paused = !st->paused;
            } else {
                switch (e.key) {
                case SDLK_1:
                    //playSound(sound_ready);
                    break;
                case SDLK_o:
                    pushScene(st, OPTIONS);
                    break;
                case SDLK_q:
                    gameOver(st);
                    return;
                case SDLK_r:
                    // TODO: only if gameOver
                    transitionStartRound(st);
                    return;
                case SDLK_l:
                    faller.type = (faller.type + 1) % NUM_PIECES;
                    break;
                case SDLK_ESCAPE:
                    st->paused = false;
                    break;
                }
            }
            if (st->paused) continue;

            if (e.key == st->settings.controls.rotateCCW) {
                tryRotate(st, &faller, CCW);
            } else if (e.key == st->settings.controls.rotateCW) {
                tryRotate(st, &faller, CW);
            } else if (e.key == st->settings.controls.store) {
                if (st->canStore) {
                    int t = faller.type;
                    if (st->stored == -1) {
                        st->stored = t;
                        if (!spawnFaller(st)) {
                            return gameOver(st);
                        }
                    } else {
                        // TODO: make this "resetPiece" or something
                        faller.orientation = 0;
                        faller.type = st->stored;
                        faller.x = 3;
                        faller.y = -(firstFilledRow(faller.type, faller.orientation));
                        st->stored = t;
                    }
                    st->canStore = false;
                }
            } else if (e.key == st->settings.controls.drop) {
                while (tryMove(st, &faller, 0, 1));
                placePiece(st, &faller);
                if (!spawnFaller(st)) {
                    return gameOver(st);
                }
            } else if (e.key == st->settings.controls.down) {
                st->held.down = true;
            } else if (e.key == st->settings.controls.left) {
                st->held.left = true;
            } else if (e.key == st->settings.controls.right) {
                st->held.right = true;
            }
        } else if (e.state.up) {
            if (e.key == st->settings.controls.down) {
                st->held.down = false;
            } else if (e.key == st->settings.controls.left) {
                st->held.left = false;
            } else if (e.key == st->settings.controls.right) {
                st->held.right = false;
            }
        }
    }

    if (st->paused) return;

    // TODO: tweak accel
    if (st->moveDelayMillis <= 0.0f) {
        if (st->held.down) {
            tryMove(st, &faller, 0, 1);
            st->dropAccel += st->dropAccel + 5;
            st->moveDelayMillis = maxMoveDelayMillis;
        } else if (st->held.left) {
            tryMove(st, &faller, -1, 0);
            st->moveDelayMillis = maxMoveDelayMillis;
        } else if (st->held.right) {
            tryMove(st, &faller, 1, 0);
            st->moveDelayMillis = maxMoveDelayMillis;
        }
    }
    if (st->held.left || st->held.right || st->held.down) {
        st->moveDelayMillis -= in.dt;
        if (!st->moving) {
            st->moveDelayMillis = moveStartDelayMillis;
        }
        st->moving = true;
    } else {
        st->moving = false;
        st->dropAccel = 0;
        st->moveDelayMillis = 0;
    }

    st->droptick += in.dt;
    float tick = tickInterval - st->score*20;
    if (tick < 200) {
        tick = 200;
    }
    if (st->droptick > tick) {
        st->droptick = 0;
        if (!tryMove(st, &faller, 0, 1)) {
            placePiece(st, &faller);
            spawnFaller(st);
        }
    }
    for (int i = 0; i < MAX_BLOCKS; i++) {
        if (!st->blocks[i].inUse) continue;
        Block *b = &st->blocks[i];
        if (b->onBoard) {
            if (b->tweenStep < 1) {
                b->tweenStep += MIN(in.dt / 500.0f, 1.0f);
                b->pos = LERP(b->pos, b->targetPos, SQUARED(b->tweenStep));
            }
        } else {
            b->timeLeft -= in.dt;
            if (b->timeLeft > 0) {
                b->rotation += b->rotv;
                b->pos = add(b->pos, b->vel);
                b->vel.y += .25f;
            } else {
                b->inUse = false;
            }
        }
    }
    return;
}

void renderInRound(Renderer *r, GameState *st) {
    if (st->paused) {
        drawTextCentered(r, &ubuntu_m32, r->screenWidth/2, r->screenHeight/2, "PAUSED");
        return;
    }

    drawRectOutline(r, st->border, white);
    if (st->settings.showGhost)
        renderGhost(r, st);
    renderBoard(r, st);
    renderScore(r, st);
    renderQueue(r, st);
}

void updateGameOver(GameState *st, InputData in) {
    for (int i = 0; i < in.numKeyEvents; i++) {
        KeyEvent e = keyEvent(&in, i);
        if (!e.state.down)
            continue;
        switch (e.key) {
        case SDLK_r:
            transitionStartRound(st);
            break;
        }
    }
}

void updateOptions(GameState *st, InputData in) {
    if (st->options.interaction != KEYBINDING) {
        for (int i = 0; i < in.numKeyEvents; i++) {
            KeyEvent e = keyEvent(&in, i);
            if (!e.state.down)
                continue;

            switch (e.key) {
            case SDLK_ESCAPE:
                popScene(st);
                return;
            }
        }
    }
    updateMenu(&st->options, in);
    return;
}

void renderOptions(Renderer *r, GameState *st) {
    drawMenu(r, &st->options, defaultHotOpts(st->t));

    // draw help text
    DrawOpts2d opts = {};
    opts.tint.r = opts.tint.g = opts.tint.b = 0.65;
    drawTextCentered(r, st->options.font, r->screenWidth/2, r->screenHeight - 2*st->options.lineHeight, "arrow keys + enter / mouse + click", opts);
}

void renderTransition(Renderer *r, GameState *st) {
    Transition t = st->transition;
    t.opts.tint = hex(0xcdcdcdff);
    switch (t.type) {
    case Transition::NONE:
        break;
    case Transition::FADE_IN_OUT:
        {
            t.opts.tint = white;
            t.opts.tint.a = (t.z <= 0.5f ? 2*t.z : 2*(1 - t.z));
            drawRect(r, rect(0, 0, r->screenWidth, r->screenHeight), t.opts);
            break;
        }
    case Transition::CHECKER_IN_OUT:
        {
            int side = 50;
            int nx = 1 + r->screenWidth / side;
            int ny = 1 + r->screenHeight / side;
            float totalDelay = 0.5f;
            float hesitation = 1.8;
            for (int i = 0; i < nx*ny; i++) {
                float d = totalDelay * (0.3*(i%nx) / nx + 0.7*(i/nx)/ny);
                float z = ((1+totalDelay)*t.z - d);
                if (z < 0) z = 0;
                else if (z > 1) z = 1;

                float z1 = hesitation * (z > 0.5 ? 1 - z : z);
                if (z1 > 0.5) z1 = 0.5;

                DrawOpts2d opts = t.opts;
                opts.origin = vec2(side/2, side/2);
                opts.rotation = 0;
                opts.scale.x = 2 * z1;
                opts.scale.y = 2 * z1;

                Rect rx;
                rx.x = ((i % nx) + 0.5) * side;
                rx.y = ((i / nx) + 0.5) * side;
                rx.w = side;
                rx.h = side;

                drawRect(r, rx, opts);
            }
            break;
        }
    case Transition::CHECKER_IN_OUT_ROTATE:
        {
            int side = 30;
            int nx = 1 + r->screenWidth / side;
            int ny = 1 + r->screenHeight / side;
            float totalDelay = 0.5f;
            for (int i = 0; i < nx*ny; i++) {
                float d = totalDelay * (0.3*(i%nx) / nx + 0.7*(i/nx)/ny);
                float z = ((1+totalDelay)*t.z - d);
                if (z < 0) z = 0;
                else if (z > 1) z = 1;
                //if (z < 0.5) z = 1 - z;
                //if (z < 0.5) opts.rotation *= -1;
                t.opts.rotation = -45 * 2 * (z > 0.5 ? z - 0.5 : 0.5 - z);
                Rect rx;
                rx.w = 2*(z > 0.5 ? 1 - z : z) * side;
                rx.h = 2*(z > 0.5 ? 1 - z : z) * side;
                rx.x = ((i % nx) + (z > 0.5 ? z : 1 - z) - 0.5) * side;
                rx.y = ((i / nx) + (z > 0.5 ? z : 1 - z) - 0.5) * side;

                drawRect(r, rx, t.opts);
            }
            break;
        }
    case Transition::ROWS_ACROSS:
        {
            int side = 75;
            int ny = 1 + r->screenHeight / side;
            float rw = r->screenWidth*2;
            float offset = r->screenWidth;

            //float totalDelay = 0.5f;
            for (int i = 0; i < ny; i++) {
                float z = t.z;
                if (z < 0) z = 0;
                else if (z > 1) z = 1;
                Rect rx;
                rx.w = rw;
                rx.h = side;
                float so = offset*((float)i+1)/ny;
                float eo = offset*(1 - ((float)i+1)/ny);
                float startx = -(rw+so);
                float endx = r->screenWidth+eo;
                rx.x = startx + (endx-startx)*z;
                rx.y = side * i;

                drawRect(r, rx, t.opts);
            }
            break;
        }
    }
}

void updateScene(GameState *st, InputData in, Scene s) {
    st->lastMousePos = in.mouse;
    switch (s) {
    case IN_ROUND:
        updateInRound(st, in);
        break;
    case TITLE:
        updateTitle(st, in);
        break;
    case OPTIONS:
        updateOptions(st, in);
        break;
    case GAME_OVER:
        updateGameOver(st, in);
        break;
    }
}

void renderScene(Renderer *r, GameState *st, Scene s) {
    // I don't like this, the time should be in the renderer so it can be
    // updated all at once (once the shader is bound, also);
    // really would like to refactor the uniforms generally as well
    useShader(r, &titleShader);
    updateShader(&titleShader, st->t, 0, &r->proj, 0, 0, &st->lastMousePos);

    useShader(r, &titleBGShader);
    updateShader(&titleBGShader, st->t, 0, &r->proj, 0, 0, 0);

    useShader(r, &r->defaultShader);
    updateShader(r->currentShader, st->t, 0, &r->proj, 0, 0, &st->lastMousePos);

    switch (s) {
    case IN_ROUND:
        renderInRound(r, st);
        break;
    case TITLE:
        renderTitle(r, st);
        break;
    case OPTIONS:
        renderOptions(r, st);
        break;
    case GAME_OVER:
        renderGameOver(r, st);
        break;
    }
}
