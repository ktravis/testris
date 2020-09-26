#ifndef GAME_H
#define GAME_H

#include "core/core.h"

#define BOARD_WIDTH  10
#define BOARD_HEIGHT 20

#define BLOCK_SIDE 30
#define BLOCK_PAD   4

struct GameState;

enum Scene {
    TITLE = 0,
    IN_ROUND,
    OPTIONS,
    GAME_OVER,
};

struct Transition {
    enum Type {
        NONE = 0,
        CHECKER_IN_OUT,
        CHECKER_IN_OUT_ROTATE,
        FADE_IN_OUT,
        ROWS_ACROSS,
    };

    Type type = NONE;
    Scene to;
    float duration;
    float t = 0;
    float z = 0;
    DrawOpts2d opts = {};
    void (*fn)(GameState *) = NULL;
};

struct FlashMessage {
    bool active;
    char text[255];
    Vec2 pos;
    float lifetime;
    float totalLifetime = 2.0f;
};

struct Block {
    int x;
    int y;
    bool inUse;
    bool onBoard;
    Color color;
    float rotation;
    Vec2 pos;

    // on board
    Vec2 targetPos;
    float tweenStep;

    // not on board
    float rotv;
    Vec2 vel;
    float timeLeft;
};

struct Cell {
    bool full;
    int blockIndex;
    //Block *block;
};

struct Piece {
    int x;
    int y;
    int type;
    int orientation;
};

enum RotationDir {
    CW,
    CCW,
};

struct Settings {
    struct {
        SDL_Keycode left = SDLK_LEFT;
        SDL_Keycode right = SDLK_RIGHT;
        SDL_Keycode down = SDLK_DOWN;
        SDL_Keycode drop = SDLK_UP;
        SDL_Keycode store = SDLK_LSHIFT;
        SDL_Keycode rotateCW = SDLK_x;
        SDL_Keycode rotateCCW = SDLK_z;
        SDL_Keycode pause = SDLK_p;
        SDL_Keycode rewind = SDLK_BACKSPACE;
        SDL_Keycode reset = SDLK_r;
        SDL_Keycode mute = SDLK_m;
        SDL_Keycode save = SDLK_LEFTBRACKET;
        SDL_Keycode restore = SDLK_RIGHTBRACKET;
    } controls;
    bool muted = false;
    bool showGhost = true;
    bool screenShake = true;
    float renderScale = 1.0f;
};

DECLARE_SERDE(Settings);

bool loadSettings(Settings *s, const char *file);
bool saveSettings(Settings *s, const char *file);

// TODO(ktravis): cleanup
bool compileExtraShaders(Renderer *r);

#define NUM_PIECES 7
#define FLASH_MESSAGE_COUNT 32

#define QUEUE_SIZE (2*2*NUM_PIECES)
#define MAX_BLOCKS (2*BOARD_HEIGHT*BOARD_WIDTH)

struct GameState {
    Scene scenes[128];
    int currentScene;

    Settings settings;

    int score;
    int hiscore;

    float droptick;

    int stored;
    bool canStore;

    float elapsed;
    float fps;

    Transition transition;

    // want smooth control
    struct {
        bool left;
        bool right;
        bool down;
    } held;
    Vec2 lastMousePos;

    Cell board[BOARD_HEIGHT][BOARD_WIDTH];
    int queue[QUEUE_SIZE];
    int queueRemaining;
    Block blocks[MAX_BLOCKS];
    int lastBlockInUse;
    Piece faller;

    // scene specific:

    // TITLE
    MenuContext titleMenu;

    // IN_ROUND
    bool roundInProgress = false;
    bool paused = false;
    bool gameOver = false;
    bool moving = false;
    float moveDelayMillis = 0;
    float dropAccel = 0;
    float shaking;

    FlashMessage messages[FLASH_MESSAGE_COUNT];

    // OPTIONS
    MenuContext options;
};

bool startGame(GameState *, Renderer *, App *);
bool updateGameState(GameState *st, InputData in);
bool updateScene(GameState *st, InputData in, Scene s);
void renderGameState(Renderer *r, GameState *st);
void renderScene(Renderer *r, GameState *st, Scene s);
void renderTransition(Renderer *r, GameState *st);

#endif // GAME_H
