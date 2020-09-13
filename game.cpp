#include "game.h"

#include "pieces.cpp"

// TODO:
// - spawn delay
// - do scoring right
// - score indicators
// - score animation
// - game over animation
// - save the high score somewhere
// - sounds
// - music
//
// - screen shake
// - animated backgrounds

#ifdef __EMSCRIPTEN__
#define SETTINGS_FILE "/offline/testris.conf"
#else
#define SETTINGS_FILE "testris.conf"
#endif

uint8_t *ubuntu_ttf_buffer;
FontAtlas ubuntu_m16;
FontAtlas ubuntu_m32;
uint8_t *mono_ttf_buffer;
FontAtlas mono_m18;

int32_t sound_ready;

ShaderProgram titleShader;
ShaderProgram titleBGShader;

Piece faller;

static App *app = NULL;

float scale() {
    float x = app->width/600.0f;
    float y = app->height/800.0f;
    return MIN(x, y);
}

float blockSide(GameState *st) {
    return scale()*BLOCK_SIDE;
}

float blockPad(GameState *st) {
    return scale()*BLOCK_PAD;
}

DrawOpts2d scaleOpts() {
    DrawOpts2d opts = {};
    opts.scale.x = opts.scale.y = scale();
    return opts;
}

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
    menu->lines = 0;
    menu->alignWidth = 24;
    menu->topCenter = vec2(app->width/2, app->height/2 + 140*scale());
    addMenuLine(menu, (char *)"start a game", MainMenu_StartGameButton);
    addMenuLine(menu, (char *)"options", MainMenu_OptionsButton);
    addMenuLine(menu, (char *)"high scores", MainMenu_HighScoresButton);
    addMenuLine(menu, (char *)"quit", MainMenu_QuitButton);

    menu->hotIndex = 0;
}

DEFINE_BUTTON(OptionsMenu_ResumeButton);
DEFINE_BUTTON(OptionsMenu_QuitButton);
DEFINE_BUTTON(OptionsMenu_ResetDefaults);

DEFINE_BUTTON(OptionsMenu_ScaleUpButton);
DEFINE_BUTTON(OptionsMenu_ScaleDownButton);

void initOptionsMenu(GameState *st) {
    MenuContext *menu = &st->options;
    menu->font = &mono_m18;
    menu->lines = 0;
    menu->alignWidth = 24;
    menu->topCenter = vec2(app->width/2, 200*scale());
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
    addMenuLine(menu, (char *)"screen shake", &st->settings.screenShake);

    addMenuLine(menu, (char *)"");
    addMenuLine(menu, (char *)"");

    addMenuLine(menu, (char *)"resume", OptionsMenu_ResumeButton);
    addMenuLine(menu, (char *)"quit", OptionsMenu_QuitButton);
    addMenuLine(menu, (char *)"scale up", OptionsMenu_ScaleUpButton);
    addMenuLine(menu, (char *)"scale down", OptionsMenu_ScaleDownButton);
    addMenuLine(menu, (char *)"reset defaults", OptionsMenu_ResetDefaults);

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
    BOOL_FIELD(showGhost),
    BOOL_FIELD(screenShake)
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
    return true;
}

bool startGame(GameState *st, Renderer *r, App *a) {
    assert(a);
    app = a;
    if (!(ubuntu_ttf_buffer = readFile("./assets/fonts/Ubuntu-M.ttf"))) {
        return false;
    }
    loadFontAtlas(&ubuntu_m32, ubuntu_ttf_buffer, 32.0f);
    ubuntu_m32.padding = 10.0f;
    loadFontAtlas(&ubuntu_m16, ubuntu_ttf_buffer, 16.0f);
    ubuntu_m16.padding = 10.0f;
    
    if (!(mono_ttf_buffer = readFile("./assets/fonts/DejaVuSansMono-Bold.ttf"))) {
        return false;
    }
    loadFontAtlas(&mono_m18, mono_ttf_buffer, 18.0f);
    mono_m18.padding = 10.0f;

    //sound_ready = loadAudioAndConvert("./assets/sounds/ready.wav");
    //if (sound_ready == -1) { return false; }

    if (!loadSettings(&st->settings, SETTINGS_FILE)) {
        log("settings not found, saving defaults");
        if (!saveSettings(&st->settings, SETTINGS_FILE)) {
            log("settings save failed");
        }
        loadSettings(&st->settings, SETTINGS_FILE);
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

Rect border(GameState *st) {
    int board_width_px = (BOARD_WIDTH * blockSide(st) + (BOARD_WIDTH + 1) * blockPad(st));
    int board_height_px = (BOARD_HEIGHT * blockSide(st) + (BOARD_HEIGHT + 1) * blockPad(st));

    Vec2 ul_corner = {
        .x = app->width/2.0f - board_width_px / 2.0f,
        .y = app->height/2.0f - board_height_px / 2.0f,
    };

    return Rect{
        .pos = ul_corner,
        .box = vec2(board_width_px, board_height_px), 
    };
}

void startRound(GameState *st) {
    st->score = 0;
    st->stored = -1;
    st->canStore = true;
    st->roundInProgress = true;


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

Vec2 gridBlockPos(GameState *st, Vec2 ul, int x, int y) {
    return vec2(
        (x + 0.5) * blockSide(st) + (x + 1) * blockPad(st) + ul.x,
        (y + 0.5) * blockSide(st) + (y + 1) * blockPad(st) + ul.y
    );
}

void tryClearingLine(GameState *st, int line) {
    Vec2 ul = border(st).pos;
    Cell *row = st->board[line];
    for (int x = 0; x < BOARD_WIDTH; x++) {
        if (!row[x].full) {
            debug("U WUT");
        }
        row[x].full = false;
        Block *b = &st->blocks[row[x].blockIndex];

        // fly off board
        b->onBoard = false;
        b->timeLeft = 3200.0f;
        b->rotv = randf(8.0f) - 4.0f;
        b->vel.x = randf(9.0f) - 4.5f;
        b->vel.y = randf(-7.5f) - 0.5f;

        for (int y = line-1; y >= 0; y--) {
            st->board[y+1][x] = st->board[y][x];
            if (!st->board[y][x].full) continue;
            st->blocks[st->board[y+1][x].blockIndex].y = y + 1;
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
        st->shaking += st->shaking > 1 ? 250.0f : 600.0f;
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
    Vec2 ul = border(st).pos;
    for (int i = 0; i < PIECE_BLOCK_TOTAL; i++) {
        if (!pieces[p->type][p->orientation][i]) continue;
        Block b = {0};
        b.inUse = true; 
        b.onBoard = true; 
        b.color = colors[p->type];
        int x = p->x+i%4;
        int y = p->y+i/4;
        b.x = x;
        b.y = y;
        b.targetPos = gridBlockPos(st, ul, x, y);
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

bool updateGameState(GameState *st, InputData in) {
    // only update the fps once a second so it's (slightly) smoother
    static float acc = 0;
    acc += in.dt;
    if (acc > 1000.0f) {
        acc = 0;
        st->fps = 1000.0f/in.dt;
    }
    st->elapsed += in.dt;

    if (in.quit) {
        return false;
    }

    Transition *t = &st->transition;

    if (t->type == Transition::NONE) {
        return updateScene(st, in, st->scenes[st->currentScene]);
    } else {
        if (updateTransition(t, in)) {
            if (t->fn) {
                t->fn(st);
                t->fn = NULL;
            }
            setScene(st, t->to);
        }
    }
    return true;
}

void renderBoard(Renderer *r, GameState *st) {
    Vec2 ul = border(st).pos;
    for (int i = 0; i < MAX_BLOCKS; i++) {
        if (!st->blocks[i].inUse) continue;
        Block *b = &st->blocks[i];
        DrawOpts2d opts;
        Rect rx = rect(b->pos, blockSide(st), blockSide(st));
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
        
        Rect rx = rect(gridBlockPos(st, ul, faller.x+x, faller.y+y), blockSide(st), blockSide(st));
        rx.pos.x -= blockSide(st)/2;
        rx.pos.y -= blockSide(st)/2;
        drawRect(r, rx, colors[faller.type]);
    }

    if (st->stored != -1) {
        bool anyInRow = false;

        float side = blockSide(st) * 0.7f;
        float pad = blockPad(st) * 0.7f;
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
    Vec2 ul = border(st).pos;
    for (int i = 0; i < PIECE_BLOCK_TOTAL; i++) {
        if (!pieces[cp.type][cp.orientation][i]) {
            continue;
        }
        int x = i % 4;
        int y = i / 4;
        if (!inBounds(cp.x+x, cp.y+y) || cp.y+y < 0) {
            continue;
        }
        
        Rect rx = rect(gridBlockPos(st, ul, cp.x+x, cp.y+y), blockSide(st), blockSide(st));
        rx.pos.x -= blockSide(st)/2;
        rx.pos.y -= blockSide(st)/2;
        drawRectOutline(r, rx, c, 2.0f);
    }
}

void renderGameOver(Renderer *r, GameState *st) {
    DrawOpts2d opts = scaleOpts();
    drawTextCentered(r, &ubuntu_m32, app->width/2, app->height/2, "OOPS", opts);

    drawTextCentered(r, &ubuntu_m16, app->width/2, app->height/2+scale()*128.0f, "( r to restart )", opts);
}

void renderScore(Renderer *r, GameState *st) {
    DrawOpts2d opts = scaleOpts();
    Vec2 ul = border(st).pos;
    float h = scale()*ubuntu_m32.lineHeight;
    // TODO(ktravis): right-align text?
    float left = ul.x - scale()*92.0f;
    drawTextf(r, &ubuntu_m32, left, ul.y + scale()*1.75f*h, opts, (const char*)"score:\n%04d", st->score, opts);

    drawText(r, &ubuntu_m32, left, ul.y + border(st).h - scale()*(h+5.0f), "hi:", opts);
    char buf[5];
    snprintf(buf, sizeof(buf), "%04d", st->hiscore);
    drawText(r, &ubuntu_m32, left, ul.y + border(st).h - scale()*5.0f, buf, opts);
}

void renderQueue(Renderer *r, GameState *st) {
    const int VISIBLE_QUEUE_BLOCKS = 5;

    float side = blockSide(st) * 0.7f;
    float pad = blockPad(st) * 0.7f;

    Rect b = border(st);
    Vec2 v = vec2(b.x+b.w + (side+2.0f*pad), 80);
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
    //drawTextCentered(r, &mono_m18, app->width - 50.0f, 24, buf, scaleOpts());
    drawTextCentered(r, &mono_m18, app->width - 50.0f, 24, buf);
#endif
}

bool updateTitle(GameState *st, InputData in) {
    MenuButton *btn = updateMenu(&st->titleMenu, in);
    if (btn == MainMenu_StartGameButton) {
        transitionStartRound(st);
    } else if (btn == MainMenu_OptionsButton) {
        transition(st, Transition::CHECKER_IN_OUT, OPTIONS, 1500);
    } else if (btn == MainMenu_HighScoresButton) {
    } else if (btn == MainMenu_QuitButton) {
        return false;
    }
    return true;
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
    Vec2 dim = vec2(app->width, app->height);
    glUniform2fv(glGetUniformLocation(titleBGShader.handle, "dim"), 1, (GLfloat*)&dim);
    drawRect(r, rect(0, 0, app->width, app->height), bgOpts);

    const int N = 6 * sizeof(title)/sizeof(char);
    VertexData vbuf[N];

    DrawOpts2d opts = scaleOpts();
    opts.meshBuffer = Mesh{.count = N, .data = vbuf};
    opts.shader = &titleShader;

    drawTextCentered(r, &mono_m18, app->width/2, 0.35f*app->height, title, opts);
    //Vec2 dim = getTextDimensions(title, &mono_m18);
    //opts.origin.x = dim.x / 2;
    //opts.origin.y = -dim.y / 2;
    //drawText(r, &mono_m18, app->width/2, 0, title, opts);

    st->titleMenu.topCenter = vec2(app->width/2, app->height/2 + 140*scale());
    st->titleMenu.scale = scaleOpts().scale;

    DrawOpts2d hotOpts = defaultHotOpts(st->elapsed);
    scale(&hotOpts.scale, scale());

    drawMenu(r, &st->titleMenu, hotOpts, scaleOpts());
}

float moveRate = 12.0f;
float maxMoveDelayMillis = 1000.0f/moveRate; 
float moveStartDelayMillis = 225.0f;

void gameOver(GameState *st) {
    transition(st, Transition::ROWS_ACROSS, GAME_OVER, 1500);
}

bool onscreen(GameState *st, Block *b) {
    return contains(rect(-blockSide(st), -blockSide(st), app->width+blockSide(st), app->height+blockSide(st)), b->pos);
}

bool updateInRound(GameState *st, InputData in) {
    for (int i = 0; i < in.numKeyEvents; i++) {
        KeyEvent e = keyEvent(&in, i);
        if (e.state.down) {
            if (e.key == st->settings.controls.mute) {
                toggleMute();
            } else if (e.key == st->settings.controls.pause) {
                st->paused = !st->paused;
            } else {
                switch (e.key) {
                case SDLK_r:
                    // TODO: only if gameOver
                    transitionStartRound(st);
                    return true;
                case SDLK_ESCAPE:
                    pushScene(st, OPTIONS);
                    return true;
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
                            gameOver(st);
                            return true;
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
                    gameOver(st);
                    return true;
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

    if (st->paused) return true;

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

	if (st->shaking > 0)
    	st->shaking -= in.dt;
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
    Vec2 ul = border(st).pos;
    for (int i = 0; i < MAX_BLOCKS; i++) {
        if (!st->blocks[i].inUse) continue;
        Block *b = &st->blocks[i];
        if (b->onBoard) {
            b->targetPos = gridBlockPos(st, ul, b->x, b->y);
            b->tweenStep += MIN(in.dt / 400.0f, 1.0f);
            if (b->tweenStep > 1) b->tweenStep = 1;
            b->pos = LERP(b->pos, b->targetPos, SQUARED(b->tweenStep));
        } else {
            b->timeLeft -= in.dt;
            if (onscreen(st, b) && b->timeLeft > 0) {
                b->rotation += b->rotv;
                b->pos = add(b->pos, scaled(b->vel, scale()));
                b->vel.y += .25f;
            } else {
                b->inUse = false;
            }
        }
    }
    return true;
}

void renderBackground(Renderer *r, GameState *st) {
    static Mesh m = {};
    if (!m.data) {
        m = polygon(6, 100);
    }
    DrawOpts2d opts;
    opts.tint = white;
    opts.tint.r = 0.9f-0.4f*cosf(st->elapsed/7500.0f);
    opts.tint.b = 0.3f+0.4f*sinf(st->elapsed/2500.0f);
    opts.tint.g = 0.35f+0.3f*cosf(st->elapsed/5500.0f);
    opts.tint.a = 0.08f;
    const int n = 10;
    for (int i = 0; i < n; i++) {
        opts.scale = vec2(scale()*7*((float)i)/n, scale()*7*((float)i)/n);
        opts.rotation = M_PI/2 + st->elapsed/(i*20.0f);
        Vec2 center = vec2(r->screenWidth/2+scale()*60.0f/i*cosf(st->elapsed/2200.0f), r->screenHeight/2+scale()*60.0f/i*sinf(st->elapsed/2200.0f));
        drawMesh(r, &m, center, r->defaultTexture, opts);
    }
}

void renderInRound(Renderer *r, GameState *st) {
    if (st->shaking > 0 && st->settings.screenShake) {
        r->offset = vec2(powf(st->shaking/750.0f,2)*randn(scale()*10), powf(st->shaking/750.0f,2)*randn(scale()*10));
    } else {
        r->offset.x = 0;
        r->offset.y = 0;
    }
    renderBackground(r, st);
    if (st->paused) {
        //drawTextCentered(r, &ubuntu_m32, app->width/2, app->height/2, "PAUSED", scaleOpts());
        drawTextCentered(r, &ubuntu_m32, app->width/2, app->height/2, "PAUSED");
        return;
    }

    drawRectOutline(r, border(st), white, 2.0f);
    if (st->settings.showGhost)
        renderGhost(r, st);
    renderBoard(r, st);
    renderScore(r, st);
    renderQueue(r, st);
}

bool updateGameOver(GameState *st, InputData in) {
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
    return true;
}

bool updateOptions(GameState *st, InputData in) {
    if (st->options.interaction != KEYBINDING) {
        for (int i = 0; i < in.numKeyEvents; i++) {
            KeyEvent e = keyEvent(&in, i);
            if (!e.state.down)
                continue;

            switch (e.key) {
            case SDLK_ESCAPE:
                if (!st->roundInProgress) transition(st, Transition::ROWS_ACROSS, TITLE, 1500);
                else popScene(st);
                return true;
            }
        }
    }

    Settings last = st->settings;
    MenuButton *btn = updateMenu(&st->options, in);
    if (btn == OptionsMenu_ScaleUpButton) {
        float s = (roundf(floorf(scale() * 4.0f+0.5f) + 1) / 4.0f);
        SDL_SetWindowSize(app->window.handle, 600.0f*s, 800.0f*s);
    } else if (btn == OptionsMenu_ScaleDownButton) {
        float s = (roundf(ceilf(scale() * 4.0f-.05f) - 1) / 4.0f);
        SDL_SetWindowSize(app->window.handle, 600.0f*s, 800.0f*s);
    } else if (btn == OptionsMenu_ResetDefaults) {
        Settings s = {};
        st->settings = s;
    }
    if (!EQUAL(st->settings, last)) {
        if (!saveSettings(&st->settings, SETTINGS_FILE)) {
            log("settings save failed");
        }
    }
    if (btn == OptionsMenu_ResumeButton) {
        if (!st->roundInProgress) transition(st, Transition::ROWS_ACROSS, TITLE, 1500);
        else popScene(st);
        return true;
    } else if (btn == OptionsMenu_QuitButton) {
        return false;
    }
    return true;
}

void renderOptions(Renderer *r, GameState *st) {
    // I'd rather do this in some helper function
    st->options.topCenter = vec2(app->width/2, 200*scale());
    st->options.scale = scaleOpts().scale;

    DrawOpts2d hotOpts = defaultHotOpts(st->elapsed);
    scale(&hotOpts.scale, scale());
    drawMenu(r, &st->options, hotOpts, scaleOpts());

    // draw help text
    DrawOpts2d opts = scaleOpts();
    opts.tint.r = opts.tint.g = opts.tint.b = 0.65;
    drawTextCentered(r, st->options.font, app->width/2, app->height - 2.0f*scale()*st->options.font->lineHeight, "arrow keys + enter / mouse + click", opts);
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
            drawRect(r, rect(0, 0, app->width, app->height), t.opts);
            break;
        }
    case Transition::CHECKER_IN_OUT:
        {
            int side = 50 * scale();
            int nx = 1 + app->width / side;
            int ny = 1 + app->height / side;
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
            int side = 30 * scale();
            int nx = 1 + app->width / side;
            int ny = 1 + app->height / side;
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
            int side = 75 * scale();
            int ny = 1 + app->height / side;
            float rw = app->width*2;
            float offset = app->width;

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
                float endx = app->width+eo;
                rx.x = startx + (endx-startx)*z;
                rx.y = side * i;

                drawRect(r, rx, t.opts);
            }
            break;
        }
    }
}

bool updateScene(GameState *st, InputData in, Scene s) {
    st->lastMousePos = in.mouse;
    switch (s) {
    case IN_ROUND:
        return updateInRound(st, in);
    case TITLE:
        return updateTitle(st, in);
    case OPTIONS:
        return updateOptions(st, in);
    case GAME_OVER:
        return updateGameOver(st, in);
    }
    assert(false);
    return false;
}

void renderScene(Renderer *r, GameState *st, Scene s) {
    // XXX(ktravis): I don't like this, the time should be in the renderer so it can be
    // updated all at once (once the shader is bound, also);
    // really would like to refactor the uniforms generally as well
    useShader(r, &titleShader);
    glUniform1f(r->currentShader->uniforms.timeLoc, st->elapsed);
    glUniformMatrix4fv(r->currentShader->uniforms.projLoc, 1, GL_FALSE, (const GLfloat *)&r->proj);
    glUniform2fv(r->currentShader->uniforms.mouseLoc, 1, (const GLfloat *)&st->lastMousePos);

    useShader(r, &titleBGShader);
    glUniform1f(r->currentShader->uniforms.timeLoc, st->elapsed);
    glUniformMatrix4fv(r->currentShader->uniforms.projLoc, 1, GL_FALSE, (const GLfloat *)&r->proj);
    Vec2 dim = vec2(app->width, app->height);
    glUniform2fv(glGetUniformLocation(titleBGShader.handle, "dim"), 1, (GLfloat*)&dim);

    // TODO(ktravis): we should do this inside the "useShader" call maybe, or perhaps something similar in the renderer

    useShader(r, &r->defaultShader);
    glUniform1f(r->currentShader->uniforms.timeLoc, st->elapsed);
    glUniformMatrix4fv(r->currentShader->uniforms.projLoc, 1, GL_FALSE, (const GLfloat *)&r->proj);

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
