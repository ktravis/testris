#include "game.h"

#include "pieces.cpp"

#define SETTINGS_FILE "testris.conf"
#define HIGHSCORES_FILE "testris.highscores"

uint8_t *ubuntu_ttf_buffer;
FontAtlas ubuntu_m16;
FontAtlas ubuntu_m32;
uint8_t *mono_ttf_buffer;
FontAtlas mono_m18;

int32_t sound_ready;
int32_t whooop;

ShaderProgram titleShader;
ShaderProgram titleBGShader;
ShaderProgram vhsShader;;

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
        if (st->inRound.board[y+iy][x+ix].full) {
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
        if (st->inRound.board[y+iy][x+ix].full) {
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
            if (st->inRound.board[y+iy][x+ix].full) {
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
DEFINE_BUTTON(MainMenu_WatchReplayButton);
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
    addMenuLine(menu, (char *)"watch a replay", MainMenu_WatchReplayButton);
    addMenuLine(menu, (char *)"options", MainMenu_OptionsButton);
    addMenuLine(menu, (char *)"high scores", MainMenu_HighScoresButton);
#ifndef __EMSCRIPTEN__
    addMenuLine(menu, (char *)"quit", MainMenu_QuitButton);
#endif

    menu->hotIndex = 0;
}

DEFINE_BUTTON(OptionsMenu_ResumeButton);
DEFINE_BUTTON(OptionsMenu_ControlsButton);
DEFINE_BUTTON(OptionsMenu_SettingsButton);
DEFINE_BUTTON(OptionsMenu_BackButton);
DEFINE_BUTTON(OptionsMenu_QuitButton);
DEFINE_BUTTON(OptionsMenu_MainMenuButton);
DEFINE_BUTTON(OptionsMenu_ResetDefaults);

DEFINE_BUTTON(OptionsMenu_ScaleUpButton);
DEFINE_BUTTON(OptionsMenu_ScaleDownButton);

void initOptionsMenu(GameState *st) {
    MenuContext *menu = &st->options;
    menu->font = &mono_m18;
    menu->lines = 0;
    menu->alignWidth = 24;
    menu->topCenter = vec2(app->width/2, 200*scale());
    addMenuLine(menu, (char *)"[ options ]");
    addMenuLine(menu, (char *)"resume", OptionsMenu_ResumeButton);
    addMenuLine(menu, (char *)"controls", OptionsMenu_ControlsButton);
    addMenuLine(menu, (char *)"settings", OptionsMenu_SettingsButton);
    addMenuLine(menu, (char *)"main menu", OptionsMenu_MainMenuButton);
#ifndef __EMSCRIPTEN__
    addMenuLine(menu, (char *)"quit", OptionsMenu_QuitButton);
#endif

    menu->hotIndex = 1;

    menu = &st->controlsMenu;
    menu->font = &mono_m18;
    menu->lines = 0;
    menu->alignWidth = 24;
    menu->topCenter = vec2(app->width/2, 200*scale());
    addMenuLine(menu, (char *)"[ controls ]");
    addMenuLine(menu, (char *)"left", &st->settings.controls.left);
    addMenuLine(menu, (char *)"right", &st->settings.controls.right);
    addMenuLine(menu, (char *)"down", &st->settings.controls.down);
    addMenuLine(menu, (char *)"drop", &st->settings.controls.drop);
    addMenuLine(menu, (char *)"store", &st->settings.controls.store);
    addMenuLine(menu, (char *)"rotate cw", &st->settings.controls.rotateCW);
    addMenuLine(menu, (char *)"rotate ccw", &st->settings.controls.rotateCCW);
    addMenuLine(menu, (char *)"");
    addMenuLine(menu, (char *)"pause", &st->settings.controls.pause);
    addMenuLine(menu, (char *)"rewind", &st->settings.controls.rewind);
    addMenuLine(menu, (char *)"mute", &st->settings.controls.mute);
    addMenuLine(menu, (char *)"quick reset", &st->settings.controls.reset);
    addMenuLine(menu, (char *)"save", &st->settings.controls.save);
    addMenuLine(menu, (char *)"restore", &st->settings.controls.restore);
    addMenuLine(menu, (char *)"");
    addMenuLine(menu, (char *)"reset defaults", OptionsMenu_ResetDefaults);
    addMenuLine(menu, (char *)"back", OptionsMenu_BackButton);

    menu->hotIndex = 1;

    menu = &st->settingsMenu;
    menu->font = &mono_m18;
    menu->lines = 0;
    menu->alignWidth = 30;
    menu->topCenter = vec2(app->width/2, 200*scale());
    addMenuLine(menu, (char *)"[ settings ]");
    addMenuLine(menu, (char *)"muted", &st->settings.muted);
    addMenuLine(menu, (char *)"ghost", &st->settings.showGhost);
    addMenuLine(menu, (char *)"screen shake", &st->settings.screenShake);
    addMenuLine(menu, (char *)"move delay (ms)", &st->settings.moveDelayMillis);
    addMenuLine(menu, (char *)"move start delay (ms)", &st->settings.moveStartDelayMillis);
    addMenuLine(menu, (char *)"");
    addMenuLine(menu, (char *)"scale up", OptionsMenu_ScaleUpButton);
    addMenuLine(menu, (char *)"scale down", OptionsMenu_ScaleDownButton);
    addMenuLine(menu, (char *)"");
    addMenuLine(menu, (char *)"reset defaults", OptionsMenu_ResetDefaults);
    addMenuLine(menu, (char *)"back", OptionsMenu_BackButton);

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
    KEY_FIELD(rewind),
    KEY_FIELD(reset),
    KEY_FIELD(mute),
    KEY_FIELD(save),
    KEY_FIELD(restore),
    BOOL_FIELD(muted),
    BOOL_FIELD(showGhost),
    BOOL_FIELD(screenShake),
    INT_FIELD(moveDelayMillis),
    INT_FIELD(moveStartDelayMillis)
)

void saveHighScores(GameState *st, const char *filename) {
    uint8_t *inPtr = (uint8_t*)&st->highScores[0];
    uint32_t inSize = sizeof(st->highScores);
#ifdef __EMSCRIPTEN__
    EM_ASM({
        var a = Module.HEAPU8.subarray($0, $0+$1);
        var data = String.fromCharCode.apply(null, a);
        localStorage.setItem("highscores", data);
    }, inPtr, inSize);
#else
    char *expanded = expandPath(filename);
    if (expanded) {
        filename = expanded;
    }

    FILE *f = fopen(filename, "wb");
    if (!f) {
        fprintf(stderr, (char*)"error writing to file %s: %d\n", filename, errno);
        free(expanded);
        return;
    }
    fwrite(inPtr, sizeof(uint8_t), inSize, f);
    fclose(f);
    free(expanded);
#endif
}

bool loadHighScores(GameState *st, const char *filename) {
    uint8_t *buf = NULL;
#ifdef __EMSCRIPTEN__
    buf = (uint8_t*)EM_ASM_INT({
        try {
          var jsString = localStorage.getItem("highscores");
          var lengthBytes = jsString.length+1;
          var stringOnWasmHeap = _malloc(lengthBytes);
          stringToAscii(jsString, stringOnWasmHeap, lengthBytes);
          return stringOnWasmHeap;
       } catch (e) {
          console.error(e);
          return null;
       }
    });
#else
    buf = readFile(filename);
#endif
    if (!buf) {
        return false;
    }
    HighScore *src = (HighScore*)buf;
    for (int i = 0; i < HIGH_SCORE_LIST_LENGTH; i++)
        st->highScores[i] = *src++;
    free(buf);
    return true;
}

bool loadSettings(Settings *s, const char *filename) {
    uint8_t *buf = NULL;
#ifdef __EMSCRIPTEN__
    buf = (uint8_t*)EM_ASM_INT({
        try {
          var jsString = localStorage.getItem("settings");
          var lengthBytes = jsString.length+1;
          var stringOnWasmHeap = _malloc(lengthBytes);
          stringToAscii(jsString, stringOnWasmHeap, lengthBytes);
          return stringOnWasmHeap;
       } catch (e) {
          console.error(e);
          return null;
       }
    });
#else
    buf = readFile(filename);
#endif
    if (!buf) {
        return false;
    }
    deserializeSettings(s, buf);
    free(buf);
    return true;
}

bool saveSettings(Settings *s, const char *filename) {
    bool result;
    uint8_t *buf = serializeSettings(s);
#ifdef __EMSCRIPTEN__
    EM_ASM({
        localStorage.setItem("settings", AsciiToString($0));
    }, buf);
    result = true;
#else
    result = writeFile(filename, buf);
#endif
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

    shaderBuf = readFile("assets/shaders/vhs.fs");
    if (!shaderBuf) {
        return false;
    }

    vhsShader.vert = r->defaultShader.vert;
    if (!compileShader(&vhsShader.frag, shaderBuf, GL_FRAGMENT_SHADER)) {
        return false;
    }

    if (!compileShaderProgram(&vhsShader)) {
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

    loadHighScores(st, HIGHSCORES_FILE);

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
    whooop = loadAudioAndConvert("./assets/sounds/whooop.wav");
    if (whooop == -1) return false;

    
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
    st->inRound.faller.orientation = 0;
    st->inRound.faller.type = st->inRound.queue[0];
    st->inRound.faller.x = 3;
    st->inRound.faller.y = -(firstFilledRow(st->inRound.faller.type, st->inRound.faller.orientation));
    for (int i = 0; i < QUEUE_SIZE-1; i++)
        st->inRound.queue[i] = st->inRound.queue[i+1];
    st->inRound.queueRemaining--;
    if (st->inRound.queueRemaining <= 2*NUM_PIECES) {
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
                st->inRound.queue[st->inRound.queueRemaining++] = base[i];
            }
        }
    }
    if (!pieceFits(st, &st->inRound.faller)) {
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

bool replaying = false;
int cursor = 0;
struct {
    uint32_t seed;
    uint32_t cap = 0, len = 0;
    InputData *replayInputs;
} replayData;

void startRound(GameState *st) {
    st->rw.cursor = 0;
    st->rw.size = 0;
    st->inRound.score = 0;
    st->inRound.stored = -1;
    st->inRound.canStore = true;
    st->inRound.roundInProgress = true;
    if (!replaying) {
        replayData.len = 0;
        replayData.cap = 0;
        if (replayData.replayInputs) free(replayData.replayInputs);
        replayData.replayInputs = NULL;
    }
    log("setting seed %u", replayData.seed);
    srand(replayData.seed);


    for (int y = 0; y < BOARD_HEIGHT; y++)
        for (int x = 0; x < BOARD_WIDTH; x++)
            st->inRound.board[y][x] = (Cell){.full = false};

    for (int i = 0; i < MAX_BLOCKS; i++)
        st->inRound.blocks[i].inUse = false;

    int base[NUM_PIECES];
    int n = NUM_PIECES;
    for (int i = 0; i < n; i++) {
        base[i] = i;
    }
    // fill the entire queue
    st->inRound.queueRemaining = 0;
    for (int x = 0; x < 4; x++) {
        // fisher-yates shuffle
        for (int i = n-1; i > 0; i--) { 
            int j = randn(i+1); 
            swap(&base[i], &base[j]);
        } 
        // copy in
        for (int i = 0; i < n; i++) {
            st->inRound.queue[st->inRound.queueRemaining++] = base[i];
        }
    }

    assert(spawnFaller(st));
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
    Cell *row = st->inRound.board[line];
    for (int x = 0; x < BOARD_WIDTH; x++) {
        if (!row[x].full) {
            debug("U WUT");
        }
        row[x].full = false;
        Block *b = &st->inRound.blocks[row[x].blockIndex];

        // fly off board
        b->onBoard = false;
        b->timeLeft = 3200.0f;
        b->rotv = randf(8.0f) - 4.0f;
        b->vel.x = randf(9.0f) - 4.5f;
        b->vel.y = randf(-7.5f) - 0.5f;

        for (int y = line-1; y >= 0; y--) {
            st->inRound.board[y+1][x] = st->inRound.board[y][x];
            if (!st->inRound.board[y][x].full) continue;
            st->inRound.blocks[st->inRound.board[y+1][x].blockIndex].y = y + 1;
            st->inRound.blocks[st->inRound.board[y+1][x].blockIndex].tweenStep = 0;
        }
        st->inRound.board[0][x].full = false;
    }
}

void clearLines(GameState *st) {
    for (int y = 0; y < BOARD_HEIGHT; y++) {
        bool all = true;
        for (int x = 0; x < BOARD_WIDTH; x++) {
            if (!st->inRound.board[y][x].full) {
                all = false;
                break;
            }
        }
        if (!all) {
            continue;
        }
        tryClearingLine(st, y);
        st->inRound.score++;
        st->inRound.shaking += st->inRound.shaking > 1 ? 250.0f : 600.0f;
    }
}

int placeBlock(GameState *st, Block b) {
    int idx;
    for (int i = 0; i < MAX_BLOCKS; i++) {
        idx = (i + st->inRound.lastBlockInUse) % MAX_BLOCKS;
        if (!st->inRound.blocks[idx].inUse)
            break;
        // fall through if we run out
    }
    st->inRound.blocks[idx] = b;
    st->inRound.lastBlockInUse = idx;
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
        st->inRound.board[y][x].full = true;
        st->inRound.board[y][x].blockIndex = placeBlock(st, b);
        //int idx = placeBlock(st, b);
        //st->board[y][x].block = &st->inRound.blocks[idx];
    }
    clearLines(st);
    st->inRound.canStore = true;
}

FlashMessage *freeMessageSlot(GameState *st) {
    for (int i = 0; i < FLASH_MESSAGE_COUNT; i++) {
        FlashMessage *msg = &st->inRound.messages[i];
        if (msg->active) continue;
        return msg;
    }
    return &st->inRound.messages[0];
}

FlashMessage *flashMessage(GameState *st, const char *text, Vec2 pos) {
    FlashMessage *msg = freeMessageSlot(st);
    msg->active = true;
    int i = 0;
    while ((msg->text[i] = text[i]) && ++i < sizeof(msg->text));
    msg->totalLifetime = 2000.0f;
    msg->lifetime = msg->totalLifetime;
    msg->pos = pos;
    return msg;
}

FlashMessage *flashMessage(GameState *st, const char *text) {
    return flashMessage(st, text, vec2(0, 0));
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
    st->inRound.elapsed += in.dt;

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
        if (!st->inRound.blocks[i].inUse) continue;
        Block *b = &st->inRound.blocks[i];
        // fix for scaling while in-round scene is visible but not updating
        if (b->onBoard) {
            b->targetPos = gridBlockPos(st, ul, b->x, b->y);
            b->pos = b->targetPos;
        }
        DrawOpts2d opts;
        Rect rx = rect(b->pos, blockSide(st), blockSide(st));
        opts.origin = scaled(rx.box, 0.5);
        opts.tint = b->color;
        opts.rotation = b->rotation;
        drawRect(r, rx, opts);
    }

    // faller
    for (int i = 0; i < PIECE_BLOCK_TOTAL; i++) {
        if (!pieces[st->inRound.faller.type][st->inRound.faller.orientation][i]) {
            continue;
        }
        int x = i % 4;
        int y = i / 4;
        if (!inBounds(st->inRound.faller.x+x, st->inRound.faller.y+y) || st->inRound.faller.y+y < 0) {
            continue;
        }
        
        Rect rx = rect(gridBlockPos(st, ul, st->inRound.faller.x+x, st->inRound.faller.y+y), blockSide(st), blockSide(st));
        rx.pos.x -= blockSide(st)/2;
        rx.pos.y -= blockSide(st)/2;
        drawRect(r, rx, colors[st->inRound.faller.type]);
    }

    if (st->inRound.stored != -1) {
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
            if (!pieces[st->inRound.stored][0][i]) {
                continue;
            }
            anyInRow = true;
            
            Vec2 p = vec2(
                (x * (side + pad) + v.x),
                (y * (side + pad) + v.y)
            );
            drawRect(r, rect(p, box), colors[st->inRound.stored]);
        }
    }
}

void renderGhost(Renderer *r, GameState *st) {
    Piece cp = st->inRound.faller;
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

void updateFlashMessages(GameState *st, InputData in) {
    for (int i = 0; i < FLASH_MESSAGE_COUNT; i++) {
        FlashMessage *m = &st->inRound.messages[i];
        if (!m->active) continue;
        m->active = (m->lifetime -= in.dt) > 0;
    }
}

void renderFlashMessages(Renderer *r, GameState *st) {
    Vec2 messageBase = vec2(app->width/2, app->height*0.7);
    Vec2 diff = vec2(0, -scale()*ubuntu_m32.lineHeight*2.0f);
    for (int i = 0; i < FLASH_MESSAGE_COUNT; i++) {
        FlashMessage *m = &st->inRound.messages[i];
        if (!m->active) continue;
        Vec2 pos = m->pos;
        if (pos.x == 0 && pos.y == 0)
            pos = add(messageBase, scaled(diff, i));
        DrawOpts2d opts = {};
        opts.tint.a = 3*m->lifetime / m->totalLifetime;
        opts.tint.a *= opts.tint.a;
        drawTextCentered(r, &ubuntu_m32, pos.x, pos.y, m->text, opts);
    }
}

void renderHighScores(Renderer *r, GameState *st) {
    DrawOpts2d opts = scaleOpts();

    drawTextCentered(r, &ubuntu_m32, app->width/2, app->height*0.1f, "high scores", opts);
    for (int i = 0; i < HIGH_SCORE_LIST_LENGTH; i++) {
        DrawOpts2d opts = scaleOpts();
        HighScore hs = st->highScores[i];
        if (i == st->newHighScore) {
            opts.tint = red;
            if (st->highScoreNameCursor == 0) {
                sprintf(hs.name, "%s", "<enter a name!>");
            } else if (st->highScoreNameCursor < sizeof(hs.name)-1) {
                hs.name[st->highScoreNameCursor] = '_';
                hs.name[st->highScoreNameCursor+1] = '\0';
            }
        }
        if (hs.score) {
            drawTextf(r, &ubuntu_m32, app->width*0.22, app->height*0.2+i*32*scale(), opts, "%d. %s", i+1, hs.name);
        } else {
            opts.tint.a = 0.5f;
            drawTextf(r, &ubuntu_m32, app->width*0.22, app->height*0.2+i*32*scale(), opts, "%d. <no entry>", i+1);
        }
        drawTextf(r, &ubuntu_m32, app->width*0.68, app->height*0.2+i*32*scale(), opts, "%04d", hs.score);
    }

    renderFlashMessages(r, st);

    if (replayData.len) {
        char buf[256];
        snprintf(buf, sizeof(buf), "( '%s' to save replay )", "i");
        drawTextCentered(r, &ubuntu_m16, app->width/2, app->height*0.3+scale()*370.0f, buf, opts);
    }

    drawTextCentered(r, &ubuntu_m16, app->width/2, app->height*0.3+scale()*400.0f, "escape to return", opts);
}

void renderGameOver(Renderer *r, GameState *st) {
    DrawOpts2d opts = scaleOpts();
    drawTextCentered(r, &ubuntu_m32, app->width/2, app->height/2, "OOPS", opts);

    char buf[256];
    snprintf(buf, sizeof(buf), "( '%s' to save replay, '%s' to restart )", "i", SDL_GetKeyName(st->settings.controls.reset));
    drawTextCentered(r, &ubuntu_m16, app->width/2, app->height/2+scale()*128.0f, buf, opts);
    renderFlashMessages(r, st);
}

void renderScore(Renderer *r, GameState *st) {
    DrawOpts2d opts = scaleOpts();
    Vec2 ul = border(st).pos;
    float h = scale()*ubuntu_m32.lineHeight;
    // TODO(ktravis): right-align text?
    float left = ul.x - scale()*92.0f;
    drawTextf(r, &ubuntu_m32, left, ul.y + h*0.75f, opts, (const char*)"score:\n%04d", st->inRound.score, opts);

    drawText(r, &ubuntu_m32, left, ul.y + border(st).h - scale()*(h+5.0f), "hi:", opts);
    char buf[5];
    uint32_t hi = st->highScores[0].score;
    snprintf(buf, sizeof(buf), "%04d", st->inRound.score > hi ? st->inRound.score : hi);
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
            int t = st->inRound.queue[q];
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
    case REPLAY_FINISHED:
        return false;
    default:
        return true;
    }
}

bool saveReplay(GameState *st) {
    uint8_t *rdPtr = (uint8_t*)&replayData;
    uint8_t *inPtr = (uint8_t*)replayData.replayInputs;
    uint32_t inSize = sizeof(InputData)*replayData.len;
#ifdef __EMSCRIPTEN__
    EM_ASM({
        /* var buf = Module._malloc($1+$3); */
        var a = Module.HEAPU8.subarray($0, $0+$1);
        var b = Module.HEAPU8.subarray($2, $2+$3);

        var blob = new Blob([a, b], {
          type: 'application/octet-stream',
        });
        var url = window.URL.createObjectURL(blob);

        var dl = document.createElement('a');
        dl.href = url;
        dl.download = (new Date().getTime())+'.tsreplay';
        dl.style = 'display: none';
        document.body.appendChild(dl);
        dl.click();
        dl.remove();

        setTimeout(() => {
          return window.URL.revokeObjectURL(url);
        }, 1000);
    }, rdPtr, sizeof(replayData), inPtr, inSize);
#else
    char *filename = (char*)"blah.tsreplay";
    char *expanded = expandPath(filename);
    if (expanded) {
        filename = expanded;
    }

    FILE *f = fopen(filename, "wb");
    if (!f) {
        fprintf(stderr, (char*)"error writing to file %s: %d\n", filename, errno);
        free(expanded);
        return false;
    }
    fwrite(rdPtr, sizeof(replayData), 1, f);
    fwrite(inPtr, sizeof(uint8_t), inSize, f);
    fclose(f);
    free(expanded);
#endif

    flashMessage(st, "replay saved");
    return true;
}

extern "C" {
    bool loadReplay(GameState *st, uint8_t *b) {
        for (int i = 0; i < sizeof(replayData); i++) {
            ((uint8_t*)(&replayData))[i] = b[i];
        }
        replayData.replayInputs = (InputData *)malloc(sizeof(InputData)*replayData.len);
        replayData.cap = replayData.len;
        for (int i = 0; i < replayData.len*sizeof(InputData); i++) {
            ((uint8_t*)replayData.replayInputs)[i] = b[sizeof(replayData)+i];
        }
        transitionStartRound(st);
        replaying = true;
        return true;
    }
};

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
        replaying = false;
        replayData.seed = rand();
        transitionStartRound(st);
    } else if (btn == MainMenu_WatchReplayButton) {
#ifdef __EMSCRIPTEN__
        EM_ASM({
            try {
              var chooser = document.getElementById('fileElem');
              chooser.value = "";
              chooser.addEventListener('change', function() {
                if (this.files.length > 0) {
                  this.files[0].arrayBuffer().then(x => {
                    var result = Module.ccall('loadReplay', 'number', ['number', 'array'], [$0, new Uint8Array(x)]);
                    console.log(result);
                  });
                }
              }, {capture: false, once: true});
              document.getElementById('fileElem').click();
           } catch (e) {
              console.error(e);
              return null;
           }
        }, st);
#else
        char *replayFile = (char*)"replay.testris";
        uint8_t *b = readFile(replayFile);
        if (b) {
            loadReplay(st, b);
            free(b);
        }
#endif
    } else if (btn == MainMenu_OptionsButton) {
        transition(st, Transition::CHECKER_IN_OUT, OPTIONS, 1500);
    } else if (btn == MainMenu_HighScoresButton) {
        transition(st, Transition::CHECKER_IN_OUT, HIGH_SCORES, 1500);
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
    drawRect(r, rect(0, 0, app->width, app->height), bgOpts);

    DrawOpts2d opts = scaleOpts();
    opts.shader = &titleShader;

    drawTextCentered(r, &mono_m18, app->width/2, app->height/2-300*scale(), title, opts);

    st->titleMenu.topCenter = vec2(app->width/2, app->height/2 + 140*scale());
    st->titleMenu.scale = scaleOpts().scale;

    DrawOpts2d hotOpts = defaultHotOpts(st->inRound.elapsed);
    scale(&hotOpts.scale, scale());

    drawMenu(r, &st->titleMenu, hotOpts, scaleOpts());
}

void gameOver(GameState *st) {
    st->newHighScore = -1;
    st->highScoreNameCursor = 0;
    for (int i = 0; i < HIGH_SCORE_LIST_LENGTH; i++) {
        if (st->highScores[i].score < st->inRound.score) {
            for (int j = HIGH_SCORE_LIST_LENGTH-1; j > i; j--) {
                st->highScores[j] = st->highScores[j-1];
            }
            st->highScores[i].score = st->inRound.score;
            st->newHighScore = i;
            transition(st, Transition::ROWS_ACROSS, HIGH_SCORES, 1500);
            return;
        }
    }
    transition(st, Transition::ROWS_ACROSS, GAME_OVER, 1500);
}

bool onscreen(GameState *st, Block *b) {
    return contains(rect(-blockSide(st), -blockSide(st), app->width+blockSide(st), app->height+blockSide(st)), b->pos);
}

bool updateReplayFinished(GameState *st, InputData in) {
    if (keyState(&in, SDLK_r).down) {
        popScene(st);
        transitionStartRound(st);
    } else if (anyKeyPress(&in)) {
        popScene(st);
        transition(st, Transition::ROWS_ACROSS, TITLE, 1200);
    }
    return true;
}

bool updateInRound(GameState *st, InputData in) {
    if (replaying && anyKeyPress(&in)) {
        replaying = false;
        flashMessage(st, "replay stopped");
        InRoundState zero = {};
        st->inRound.held = zero.held;
    }
    if (keyState(&in, SDLK_ESCAPE).down) {
        st->currentMenu = 0;
        pushScene(st, OPTIONS);
        return true;
    }
    if (replaying) {
        if (cursor+1 >= replayData.len) {
            cursor = 0;
            pushScene(st, REPLAY_FINISHED);
            return true;
        }
        in = replayData.replayInputs[cursor++];
    } else {
        if (replayData.len+1 >= replayData.cap) {
            replayData.cap = (replayData.cap == 0 ? 4096 : replayData.cap*2);
            replayData.replayInputs = (InputData *)realloc(replayData.replayInputs, sizeof(InputData)*replayData.cap);
        }
        replayData.replayInputs[replayData.len++] = in;
    }

    for (int i = 0; i < in.numKeyEvents; i++) {
        KeyEvent e = keyEvent(&in, i);
        if (e.state.down) {
            if (e.key == st->settings.controls.down) {
                st->inRound.held.down = true;
            } else if (e.key == st->settings.controls.left) {
                st->inRound.held.left = true;
            } else if (e.key == st->settings.controls.right) {
                st->inRound.held.right = true;
            }
        } else if (e.state.up) {
            if (e.key == st->settings.controls.down) {
                st->inRound.held.down = false;
            } else if (e.key == st->settings.controls.left) {
                st->inRound.held.left = false;
            } else if (e.key == st->settings.controls.right) {
                st->inRound.held.right = false;
            }
        }
    }

    if (st->rw.rewinding) {
        st->rw.size -= st->rw.rwFactor;
        if (st->rw.size < 0) st->rw.size = 0;
        if (keyState(&in, st->settings.controls.rewind).up || st->rw.size == 0) {
            st->rw.rewinding = false;
            for (int i = 0; i < sizeof(st->inRound.held); i++)
                ((char*)(&st->inRound.held))[i] = 0;
        } else {
            st->inRound = st->rw.savedStates[(st->rw.cursor + st->rw.size) % SANDS_OF_TIME];
            /* in = savedInputs[(st->rw.cursor + size)  % SANDS_OF_TIME]; */
        }
    } else {
        /* savedInputs[(st->rw.cursor + size) % SANDS_OF_TIME] = in; */
        st->rw.savedStates[(st->rw.cursor + st->rw.size) % SANDS_OF_TIME] = st->inRound;
        if (st->rw.size < SANDS_OF_TIME) st->rw.size++;
        else st->rw.cursor = (st->rw.cursor+1) % SANDS_OF_TIME;
        if (st->rw.size >= RW_THRESHOLD && keyState(&in, st->settings.controls.rewind).down) {
            st->rw.rewinding = true;
            /* playSound(whooop); */
        }
    }

    Piece oldFaller = st->inRound.faller;

    for (int i = 0; i < in.numKeyEvents; i++) {
        KeyEvent e = keyEvent(&in, i);
        if (e.state.down) {
            if (e.key == st->settings.controls.mute) {
                toggleMute();
            }

            if (e.key == st->settings.controls.rotateCCW) {
                tryRotate(st, &st->inRound.faller, CCW);
            } else if (e.key == st->settings.controls.rotateCW) {
                tryRotate(st, &st->inRound.faller, CW);
            } else if (e.key == st->settings.controls.store) {
                if (st->inRound.canStore) {
                    int t = st->inRound.faller.type;
                    if (st->inRound.stored == -1) {
                        st->inRound.stored = t;
                        if (!spawnFaller(st)) {
                            gameOver(st);
                            return true;
                        }
                    } else {
                        // TODO: make this "resetPiece" or something
                        st->inRound.faller.orientation = 0;
                        st->inRound.faller.type = st->inRound.stored;
                        st->inRound.faller.x = 3;
                        st->inRound.faller.y = -(firstFilledRow(st->inRound.faller.type, st->inRound.faller.orientation));
                        st->inRound.stored = t;
                    }
                    st->inRound.canStore = false;
                }
            } else if (e.key == st->settings.controls.drop) {
                while (tryMove(st, &st->inRound.faller, 0, 1));
                placePiece(st, &st->inRound.faller);
                if (!spawnFaller(st)) {
                    gameOver(st);
                    return true;
                }
            } else if (e.key == st->settings.controls.reset) {
                replayData.seed = rand();
                transitionStartRound(st);
                return true;
            } else if (e.key == st->settings.controls.save) {
                // save state
                if (!writeFileBinary("snapshot.testris", (uint8_t*)&st->inRound, sizeof(st->inRound)+sizeof(st->rw)))
                    log("failed to save snapshot");
                flashMessage(st, "state saved");
            } else if (e.key == st->settings.controls.restore) {
                // load state
                uint8_t *b = readFile("snapshot.testris");
                if (b) {
                    st->inRound = *(InRoundState*)(b);
                    st->rw = *(RewindBuffer*)(b+sizeof(InRoundState));
                    flashMessage(st, "state restored");
                    free(b);
                }
            } else if (e.key == SDLK_i) {
                // save state
                saveReplay(st);
            }
        }
    }

    if (st->inRound.moveDelayMillis <= 0.0f) {
        if (st->inRound.held.down) {
            tryMove(st, &st->inRound.faller, 0, 1);
            st->inRound.dropAccel += st->inRound.dropAccel + 5;
            st->inRound.moveDelayMillis = st->settings.moveDelayMillis;
        } else if (st->inRound.held.left) {
            tryMove(st, &st->inRound.faller, -1, 0);
            st->inRound.moveDelayMillis = st->settings.moveDelayMillis;
        } else if (st->inRound.held.right) {
            tryMove(st, &st->inRound.faller, 1, 0);
            st->inRound.moveDelayMillis = st->settings.moveDelayMillis;
        }
    }
    if (st->inRound.held.left || st->inRound.held.right || st->inRound.held.down) {
        st->inRound.moveDelayMillis -= in.dt;
        if (!st->inRound.moving) {
            st->inRound.moveDelayMillis = st->settings.moveStartDelayMillis;
        }
        st->inRound.moving = true;
    } else {
        st->inRound.moving = false;
        st->inRound.dropAccel = 0;
        st->inRound.moveDelayMillis = 0;
    }

	if (st->inRound.shaking > 0)
    	st->inRound.shaking -= in.dt;
    st->inRound.droptick += in.dt;
    float tick = tickInterval - st->inRound.score*20;
    if (tick < 200) {
        tick = 200;
    }

    // if the faller moved or rotated...
    if (st->inRound.faller.x != oldFaller.x || st->inRound.faller.y != oldFaller.y || st->inRound.faller.orientation != oldFaller.orientation) {
        // ...and it is "resting"...
        Piece f = st->inRound.faller;
        if (!tryMove(st, &f, 0, 1)) {
            // ...reset droptick
            st->inRound.droptick = tick/2;
        }
    }
    if (st->inRound.droptick > tick) {
        st->inRound.droptick = 0;
        if (!tryMove(st, &st->inRound.faller, 0, 1)) {
            placePiece(st, &st->inRound.faller);
            if (!spawnFaller(st)) {
                gameOver(st);
                return true;
            }
        }
    }
    Vec2 ul = border(st).pos;
    for (int i = 0; i < MAX_BLOCKS; i++) {
        if (!st->inRound.blocks[i].inUse) continue;
        Block *b = &st->inRound.blocks[i];
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

    updateFlashMessages(st, in);
    return true;
}

void renderBackground(Renderer *r, GameState *st) {
    static Mesh m = {};
    if (!m.data) {
        m = polygon(6, 100);
    }
    DrawOpts2d opts;
    opts.tint = white;
    opts.tint.r = 0.9f-0.4f*cosf(st->inRound.elapsed/7500.0f);
    opts.tint.b = 0.3f+0.4f*sinf(st->inRound.elapsed/2500.0f);
    opts.tint.g = 0.35f+0.3f*cosf(st->inRound.elapsed/5500.0f);
    opts.tint.a = 0.08f;
    const int n = 10;
    for (int i = 0; i < n; i++) {
        opts.scale = vec2(scale()*7*((float)i)/n, scale()*7*((float)i)/n);
        opts.rotation = M_PI/2 + st->inRound.elapsed/(i*20.0f);
        Vec2 center = vec2(app->width/2+scale()*60.0f/i*cosf(st->inRound.elapsed/2200.0f), app->height/2+scale()*60.0f/i*sinf(st->inRound.elapsed/2200.0f));
        drawMesh(r, &m, center, r->defaultTexture, opts);
    }
}

void renderReplayFinished(Renderer *r, GameState *st) {
    Color c = black;
    c.a = 0.5f;
    drawRect(r, rect(vec2(0, app->height/2-scale()*50), app->width, 100*scale()), c);
    drawTextCentered(r, &ubuntu_m32, app->width/2, app->height/2 - 25*scale(), "replay finished");
    drawTextCentered(r, &mono_m18, app->width/2, app->height/2 + 15*scale(), "press r to restart, any key to return");
}

void renderInRound(Renderer *r, GameState *st) {
    static RenderTarget rt = {};
    if (app->width != rt.w || app->height != rt.h) {
        createRenderTarget(&rt, app->width, app->height);
    }
    renderToTexture(r, &rt);
    clear(r);

    if (st->inRound.shaking > 0 && st->settings.screenShake) {
        r->offset = vec2(powf(st->inRound.shaking/750.0f,2)*randn(scale()*10), powf(st->inRound.shaking/750.0f,2)*randn(scale()*10));
    } else {
        r->offset.x = 0;
        r->offset.y = 0;
    }
    renderBackground(r, st);

    drawRectOutline(r, border(st), white, 2.0f);
    if (st->settings.showGhost)
        renderGhost(r, st);
    if (st->rw.size < SANDS_OF_TIME || st->rw.rewinding) {
        Color c = st->rw.size >= RW_THRESHOLD ? white : red;
        drawRectOutline(r, rect(
            vec2(0.25*app->width, app->height - scale()*42),
            0.5*app->width, scale()*16
        ), c, scale()*3.0f);
        drawRect(r, rect(
            vec2(0.25*app->width, app->height - scale()*40),
            (float(st->rw.size)/SANDS_OF_TIME)*(0.5*app->width), scale()*12
        ), c);
    }
    renderBoard(r, st);
    renderScore(r, st);
    renderQueue(r, st);
    renderFlashMessages(r, st);

    renderToScreen(r);

    DrawOpts2d opts = {};
    if (st->rw.rewinding) {
        useShader(r, &vhsShader);
        glUniform1f(r->currentShader->uniforms.timeLoc, st->inRound.elapsed);
        glUniformMatrix4fv(r->currentShader->uniforms.projLoc, 1, GL_FALSE, (const GLfloat *)&r->proj);
        opts.shader = &vhsShader;
    }
    drawTexturedQuad(r, vec2(), app->width, app->height, rt.tex, rect(0, 1, 1, 0), opts);

    useShader(r, &r->defaultShader);

    if (replaying) {
        Color c = white;
        c.g = c.b = (sinf(st->inRound.elapsed/200.0f)+1)/2;
        c.a = 0.75f;
        DrawOpts2d opts = {};
        opts.tint = c;
        drawTextCentered(r, &mono_m18, app->width/2, 25*scale(), "replay in progress", opts);
    }
}

bool updateHighScores(GameState *st, InputData in) {
    updateFlashMessages(st, in);
    if (st->newHighScore != -1) {
        char *name = &st->highScores[st->newHighScore].name[0];
        for (int i = 0; i < in.numKeyEvents; i++) {
            KeyEvent e = keyEvent(&in, i);
            if (!e.state.down) continue;
            if (e.key >= SDLK_SPACE && e.key <= SDLK_z && st->highScoreNameCursor < sizeof(st->highScores[i].name)-1) {
                name[st->highScoreNameCursor++] = e.key; 
                name[st->highScoreNameCursor] = '\0'; 
            } else if (e.key == SDLK_BACKSPACE && st->highScoreNameCursor) {
                name[--st->highScoreNameCursor] = '\0';
            } else if (e.key == SDLK_RETURN && st->highScoreNameCursor) {
                saveHighScores(st, HIGHSCORES_FILE);
                st->newHighScore = -1;
            }
        }
    } else if (keyState(&in, SDLK_ESCAPE).down) {
        transition(st, Transition::ROWS_ACROSS, TITLE, 1500);
        return true;
    } else if (keyState(&in, SDLK_i).down && replayData.len) {
        saveReplay(st);
        return true;
    }
    return true;
}

bool updateGameOver(GameState *st, InputData in) {
    updateFlashMessages(st, in);
    if (keyState(&in, st->settings.controls.reset).down) {
        replayData.seed = rand();
        transitionStartRound(st);
    } else if (keyState(&in, SDLK_i).down)
        saveReplay(st);
    else if (keyState(&in, SDLK_ESCAPE).down)
        transition(st, Transition::ROWS_ACROSS, TITLE, 1500);
    return true;
}

bool updateOptions(GameState *st, InputData in) {
    if (st->menus[st->currentMenu].interaction != KEYBINDING) {
        if (keyState(&in, SDLK_ESCAPE).down) {
            if (st->currentMenu == 0) {
                if (!st->inRound.roundInProgress) transition(st, Transition::ROWS_ACROSS, TITLE, 1500);
                else popScene(st);
            } else {
                st->currentMenu = 0;
            }
            return true;
        }
    }

    Settings last = st->settings;
    MenuButton *btn = updateMenu(&st->menus[st->currentMenu], in);
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
        if (!st->inRound.roundInProgress) transition(st, Transition::ROWS_ACROSS, TITLE, 1500);
        else popScene(st);
        return true;
    } else if (btn == OptionsMenu_BackButton) {
        st->currentMenu = 0;
    } else if (btn == OptionsMenu_ControlsButton) {
        st->currentMenu = 1;
    } else if (btn == OptionsMenu_SettingsButton) {
        st->currentMenu = 2;
    } else if (btn == OptionsMenu_MainMenuButton) {
        if (st->inRound.roundInProgress) popScene(st);
        transition(st, Transition::ROWS_ACROSS, TITLE, 1500);
        return true;
    } else if (btn == OptionsMenu_QuitButton) {
        return false;
    }
    return true;
}

void renderOptions(Renderer *r, GameState *st) {
    // I'd rather do this in some helper function
    st->options.topCenter = vec2(app->width/2, 200*scale());
    st->controlsMenu.topCenter = st->options.topCenter;
    st->settingsMenu.topCenter = st->options.topCenter;
    st->options.scale = scaleOpts().scale;

    DrawOpts2d opts = scaleOpts();
    DrawOpts2d hotOpts = defaultHotOpts(st->inRound.elapsed);
    scale(&hotOpts.scale, scale());
    drawMenu(r, &st->menus[st->currentMenu], hotOpts, opts);

    // draw help text
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
    case REPLAY_FINISHED:
        return updateReplayFinished(st, in);
    case HIGH_SCORES:
        return updateHighScores(st, in);
    }
    assert(false);
    return false;
}

void renderScene(Renderer *r, GameState *st, Scene s) {
    // XXX(ktravis): I don't like this, the time should be in the renderer so it can be
    // updated all at once (once the shader is bound, also);
    // really would like to refactor the uniforms generally as well
    useShader(r, &titleShader);
    glUniform1f(r->currentShader->uniforms.timeLoc, st->inRound.elapsed);
    glUniformMatrix4fv(r->currentShader->uniforms.projLoc, 1, GL_FALSE, (const GLfloat *)&r->proj);
    glUniform2fv(r->currentShader->uniforms.mouseLoc, 1, (const GLfloat *)&st->lastMousePos);

    useShader(r, &titleBGShader);
    glUniform1f(r->currentShader->uniforms.timeLoc, st->inRound.elapsed);
    glUniformMatrix4fv(r->currentShader->uniforms.projLoc, 1, GL_FALSE, (const GLfloat *)&r->proj);
    Vec2 dim = vec2(app->width, app->height);
    glUniform2fv(glGetUniformLocation(titleBGShader.handle, "dim"), 1, (GLfloat*)&dim);

    // TODO(ktravis): we should do this inside the "useShader" call maybe, or perhaps something similar in the renderer

    useShader(r, &r->defaultShader);
    glUniform1f(r->currentShader->uniforms.timeLoc, st->inRound.elapsed);
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
    case REPLAY_FINISHED:
        renderReplayFinished(r, st);
        break;
    case HIGH_SCORES:
        renderHighScores(r, st);
        break;
    }
}
