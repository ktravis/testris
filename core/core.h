#ifndef CORE_H
#define CORE_H

#include <ctime>
#include <cmath>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cassert>

#include <sys/inotify.h>
#include <unistd.h>
#include <poll.h>

#define GL3_PROTOTYPES 1
#include <GL/glew.h>
#include <SDL2/SDL.h>

#ifdef CORE_IMPL
#define STB_IMAGE_IMPLEMENTATION
#define STB_TRUETYPE_IMPLEMENTATION
#endif // CORE_IMPL

#include "stb_image.h"
#include "stb_truetype.h"

#include "array.h"

// vector.cpp
//

float lerp(float a, float b, float step, float min = 0.01);

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define LERP(from, to, t) add(scaled((to), (t)), scaled((from), (1 - (t))))
#define SMOOTHSTEP(x) ((x) * (x) * (3 - 2 * (x)))
#define SMOOTHERSTEP(x) ((x) * (x) * (x) * ((x) * ((x) * 6 - 15) + 10))
#define SQUARED(t) ((t) * (t))
#define CUBED(t) ((t) * (t) * (t))

struct Vec2 {
    float x;
    float y;
};

Vec2 vec2(float x, float y);
float dsqr(Vec2 a);
Vec2 add(Vec2 a, Vec2 b);
Vec2 sub(Vec2 a, Vec2 b);
void scale(Vec2 *a, float s);
void scale(Vec2 *a, Vec2 b);
Vec2 scaled(Vec2 a, float s);
Vec2 scaled(Vec2 a, Vec2 b);
float dot(Vec2 a, Vec2 b);
void normalize(Vec2 *a);
Vec2 normalized(Vec2 a);
Vec2 lerp(Vec2 a, Vec2 b, float step, float min = 0.01);

struct Vec3 {
    float x;
    float y;
    float z;
};

Vec3 vec3(float x, float y, float z);
Vec3 vec3(Vec2 xy, float z);
float dsqr(Vec3 a);
Vec3 add(Vec3 a, Vec3 b);
Vec3 sub(Vec3 a, Vec3 b);
void scale(Vec3 *a, float s);
Vec3 scaled(Vec3 a, float s);
float dot(Vec3 a, Vec3 b);
Vec3 cross(Vec3 u, Vec3 v);
void normalize(Vec3 *a);
Vec3 normalized(Vec3 a);

struct Vec4 {
    float x;
    float y;
    float z;
    float w;
};

Vec4 vec4(float x, float y, float z, float w);
Vec4 vec4(Vec2 xy, float z, float w);
Vec4 vec4(Vec3 xyz, float w);
Vec4 vec4();

// rect.cpp
//

struct Rect {
    union {
        Vec2 pos;
        struct {
            float x;
            float y;
        };
    };
    union {
        Vec2 box;
        struct {
            float w;
            float h;
        };
    };
};

Rect rect(float x0, float y0, float x1, float y1);
Rect rect(Vec2 pos, float w, float h);
Rect rect(Vec2 pos, Vec2 box);
bool collide(Rect a, Rect b);
bool wouldCollide(Rect a, Vec2 d, Rect b);
bool contains(Rect r, Vec2 v);


struct KeyState {
    bool down;
    bool up;
};

struct KeyEvent {
    // TODO: don't use SDL enum here
    SDL_Keycode key;
    KeyState state;
};

#define KEY_EVENT_BUFCOUNT 16

struct InputData {
    float dt;
    bool quit;
    bool mouseMoved;
    Vec2 mouse;
    Vec2 deltaMouse;

    KeyState lmb;
    KeyState rmb;
    // scroll_delta

    int numKeyEvents;
    int keyEventBufferOffset;
    KeyEvent keys[KEY_EVENT_BUFCOUNT];
};

bool anyKeyPress(InputData *in);

// matrix.cpp
//

typedef float Mat3[3][3];
typedef float Mat4[4][4];
float radians(float deg);
void copy(Mat4 from, Mat4 to);
void zero(Mat4 result);
void identity(Mat4 result);
void translation(Mat4 result, float x, float y, float z);
void multiply(Mat4 result, Mat4 A, Mat4 B);
void multiply(Mat4 A, Mat4 B);
Vec4 multiply(Mat4 A, Vec4 v);
void translate(Mat4 result, float x, float y, float z);
void translate(Mat4 result, Vec3 by);
void scale(Mat4 result, Vec3 s);
void scale(Mat4 result, Vec2 s);
void scale(Mat4 result, float s);
void rotation(Mat4 result, float rads, Vec3 axis);
void rotate(Mat4 result, float rads);
void rotate(Mat4 result, float rads, Vec3 axis);
void rotateAroundOffset(Mat4 result, float rads, Vec3 axis, Vec3 offset);
void lookAt(Mat4 result, Vec3 eye, Vec3 center, Vec3 up);
void ortho(Mat4 result, float left, float right, float top, float bottom, float near, float far);

// color.cpp
//

struct Color {
    float r;
    float g;
    float b;
    float a;
};

Color hex(uint32_t d);
Color multiply(Color c, float f);

extern Color white;
extern Color red;
extern Color grey;
extern Color green;
extern Color blue;
extern Color black;

// util.cpp
//

long diffMicro(struct timespec *start, struct timespec *end);
long diffMilli(struct timespec *start, struct timespec *end);
uint64_t fileSize(FILE *f);
char *expandPath(const char *path);
uint8_t *readFile(const char *filename);
bool writeFile(const char *filename, const uint8_t *b);
bool loadTextureFile(const char *fname, GLuint *tex);
uint8_t *eatLine(uint8_t **s);
void eatSpaces(uint8_t **s);
uint8_t *eatStringUntil(uint8_t **s, uint8_t c);
void trimTrailingSpaces(uint8_t **s);
float randf(float max);
int randn(int max);

void vlog(const char *msg, va_list);
void log(const char *msg, ...);

#ifdef DEBUG
void debug(const char *msg, ...);
#else
#define debug(msg, ...)
#endif

// serde.cpp
//

// Extremely simple text serializer/deserializer

struct SerDeField {
    void **address;
    size_t size;
    const char *name;
    const char *fmt;
    void *(*ser_fn)(void*);
    void *(*deser_fn)(void*);
};

uint8_t *boolToString(bool b);
bool boolFromString(uint8_t *s);
uint8_t *_serializeFields(int n, SerDeField *fields);
bool _deserializeFields(uint8_t *buf, int n, SerDeField *fields);

#define FIELD_FN(name, fmt, ser_fn, deser_fn) {(void**)&(item->name), sizeof(item->name), #name, (fmt), (void *(*)(void*))(ser_fn), (void *(*)(void*))(deser_fn)}

#define FIELD(name, fmt) FIELD_FN(name, (fmt), NULL, NULL)

#define BOOL_FIELD(name) FIELD_FN(name, "%s", boolToString, boolFromString)

#define DECLARE_SERIALIZER(obj) \
    uint8_t *serialize ## obj(obj *item)

#define DECLARE_DESERIALIZER(obj) \
    bool deserialize ## obj(obj *item, uint8_t *buf)

#define DECLARE_SERDE(obj) \
    DECLARE_SERIALIZER(obj);\
    DECLARE_DESERIALIZER(obj)

#define DEFINE_SERIALIZER(obj, ...) \
    DECLARE_SERIALIZER(obj) {\
        SerDeField fields[] = { __VA_ARGS__ };\
        return _serializeFields(int(sizeof(fields)/sizeof(SerDeField)), fields);\
    }

#define DEFINE_DESERIALIZER(obj, ...) \
    DECLARE_DESERIALIZER(obj) {\
        SerDeField fields[] = { __VA_ARGS__ };\
        return _deserializeFields(buf, int(sizeof(fields)/sizeof(SerDeField)), fields);\
    }

#define DEFINE_SERDE(obj, ...) \
    DEFINE_SERIALIZER(obj, __VA_ARGS__)\
    DEFINE_DESERIALIZER(obj, __VA_ARGS__)

// text.cpp
//

#define ATLAS_CHAR_COUNT 96
struct FontAtlas {
    stbtt_fontinfo info;
    stbtt_bakedchar cdata[ATLAS_CHAR_COUNT]; // ASCII 32..126 is 95 glyphs
    GLuint tex;
};

void loadFontAtlas(FontAtlas *font, const unsigned char *font_data, float size);
Vec2 getTextDimensions(const char *text, FontAtlas *font);
Vec2 getTextCenterOffset(const char *text, FontAtlas *font);

// sound.cpp
//

struct Sound {
    const char *name;
    uint8_t *buf;
    uint32_t len;
};

struct SoundProps {
    float volume;
    // balance
    // pitch shift?
    // etc
};

struct Playing {
    int32_t handle;
    bool active;
    Sound *s;
    uint32_t cur;
    SoundProps props;
};

void stopAllSounds();
bool stopSound(int32_t handle);
int32_t playSound(int32_t id, SoundProps props);
int32_t playSound(int32_t id);

extern SoundProps globalSoundProps;
extern SoundProps defaultSoundProps;

bool toggleMute();
void mixAudioS16LE(uint8_t *dst, uint8_t *src, uint32_t len_bytes, SoundProps props);
void audioCallback(void *user_data, uint8_t *stream, int32_t len);
int32_t loadAudioAndConvert(const char *filename);
bool openAudioDevice();
void cleanupAudio();

// shaders.cpp
//
void printShaderLog(GLuint shader);
void printProgramLog(GLuint program);
bool compileShader(GLuint *prog, const unsigned char *vs, const unsigned char *fs);

// mesh.cpp
//

struct VertexData {
    union {
        Vec4 pos;
        struct {
            float x;
            float y;
            float z;
            float w;
        };
    };
    union {
        Color color;
        struct {
            float r;
            float g;
            float b;
            float a;
        };
    };
    union {
        Vec2 tex;
        struct {
            float s;
            float t;
        };
    };
};

struct Mesh {
    int count;
    VertexData *data;
};

extern Mesh quadMesh;
Mesh texturedQuadMesh(VertexData data[6], Rect v, Rect st);
Mesh texturedQuadMesh(VertexData data[6], Rect st);

// renderer.cpp
//

typedef GLuint ShaderHandle;
typedef GLuint TextureHandle;

enum DrawMode {
    Triangles = 0,
    Lines,
};

struct DrawCall {
    DrawMode mode;
    Mesh *mesh;
    TextureHandle texture;
    Rect texCoords;
    struct {
        Mat4 model;
        Color tint;
        Rect texMod;
        float lineWidth = 1.0f;
    } uniforms;
};

struct DrawOpts2d {
    Vec2 origin = {0};
    Vec2 scale = {1, 1};
    Rect texCoords = {.pos= {0}, .box= {1, 1}};
    float rotation = 0.0f;
    Color tint = white;
    Mesh meshBuffer = {};
};

struct DrawOpts3d {
    Vec3 origin = {0};
    Vec3 scale = {1, 1, 1};
    Rect texCoords = {.pos=(Vec2){0}, .box=(Vec2){1, 1}};
    // TODO: this needs to use a quaternion
    //float rotation = 0.0f;
    Color tint = white;
};

struct Renderer {
    uint32_t screenWidth;
    uint32_t screenHeight;

    Mat4 proj;

    ShaderHandle currentShader;
    ShaderHandle defaultShader;
    GLint defaultColorLoc;
    GLint defaultProjLoc;
    GLint defaultModelLoc;
    GLint tintLoc;
    GLint texModLoc;

    GLuint vao;
    GLuint vbo;

    TextureHandle currentTexture;
    TextureHandle defaultTexture;
};

GLenum glDrawMode(DrawMode m);

void initRenderer(Renderer *r, uint32_t w, uint32_t h);
void clear(Renderer *r);
void clear(Renderer *r, Color c);
void useShader(Renderer *r, ShaderHandle s);
void setupProjection(Renderer *r, Mat4 proj);
bool setupShaders(Renderer *r);
void cleanupRenderer(Renderer *r);
void renderToScreen(Renderer *r);
bool renderToTexture(Renderer *r, TextureHandle *tex, uint32_t w, uint32_t h);
void draw(Renderer *r, DrawCall call);

// draw.cpp
//

void drawMesh(Renderer *r, Mesh *m, Vec2 pos, TextureHandle texture, DrawOpts2d opts = {});
void drawMesh(Renderer *r, Mesh *m, Vec3 pos, TextureHandle texture, DrawOpts3d opts = {});

void drawText(Renderer *r, FontAtlas *font, float x, float y, const char *text, DrawOpts2d opts = {});
void drawTextf(Renderer *r, FontAtlas *font, float x, float y, DrawOpts2d opts, const char *fmt...);
void drawTextCentered(Renderer *r, FontAtlas *font, float x, float y, const char *text, DrawOpts2d opts = {});

void drawTexturedQuad(Renderer *r, Vec2 pos, float w, float h, TextureHandle texture, Rect st, DrawOpts2d opts = {});
void drawTexturedQuad(Renderer *r, Rect quad, TextureHandle texture, Rect st, DrawOpts2d opts = {});

void drawRect(Renderer *rend, Rect r, DrawOpts2d opts = {});
void drawRect(Renderer *rend, Rect r, Color c);

void drawRectOutline(Renderer *rend, Rect r, Color c, float thickness = 1.0f);

void drawLine(Renderer *r, Vec2 a, Vec2 b, Color c);

// sprites.cpp
//

struct SpriteSheet {
    TextureHandle t;
    int sheet_width;
    int sheet_height;
    int tile_w;
    int tile_h;

    int nrows;
    int ncols;

    float norm_tile_w;
    float norm_tile_h;
};

SpriteSheet *loadSpriteSheet(const char *fname, int tile_w, int tile_h);

struct Sprite {
    SpriteSheet *sheet;
    int index;
};

void drawSprite(Renderer *r, Sprite s, Vec2 pos, float w, float h);
void drawSpriteFlippedX(Renderer *r, Sprite s, Vec2 pos, float w, float h);
void drawSpriteFlippedY(Renderer *r, Sprite s, Vec2 pos, float w, float h);

struct Animation {
    SpriteSheet *sheet;
    int *indices;
    int count;
    float delay;
};

struct AnimState {
    Animation *a;
    float t;
    int curr;
};

Sprite currFrame(AnimState *a);
Sprite currFrame(Animation *a, float ms);

Sprite step(AnimState *a, float dt);
void init(Animation *a, SpriteSheet *s, double delay, ...);
//anim *new_animation(SpriteSheet *sheet, float delay);
int addFrame(Animation *a, int i);
//Sprite next_sprite(anim *a);
void freeAnimation(Animation *a);

typedef int AssetID;

// watcher.cpp
//

#define EVENT_SIZE (sizeof (struct inotify_event))
#define EVENT_BUF_LEN (1024 * (EVENT_SIZE + 16))

#define WATCH_DEFAULT (IN_MODIFY|IN_MOVE)

struct Watcher {
    struct pollfd *pollees;
    bool *updated;
    char buf[EVENT_BUF_LEN];
};

int watchPath(Watcher *w, const char *path, uint32_t events);
int watchPath(Watcher *w, const char *path);
bool checkWatches(Watcher *w, int ms);
bool wait(Watcher *w);
bool updated(Watcher *w, int i);
void cleanupWatcher(Watcher *w);

// window.cpp
//

struct Window {
    SDL_Window *handle;
    SDL_GLContext ctx;
};

bool initWindow(Window *w, const char *title, int width, int height);
void swapWindow(Window *w);
void cleanupWindow(Window *w);

// menu.cpp
//

enum InteractionType {
    NONE,
    KEYBINDING,
};

enum MenuItemType {
    HEADING,
    TOGGLE_VALUE,
    KEYBINDING_VALUE,
};

struct MenuLine {
    MenuItemType type;
    char *label;

    union {
        bool *toggle;
        struct {
            bool waiting;
            SDL_Keycode *current;
        } key;
    };
};

struct MenuContext {
    Vec2 topCenter;
    MenuLine *lines;
    int hotIndex;
    InteractionType interaction;

    FontAtlas *font;
    // TODO: calculate this default from the font
    float lineHeight = 20.0f;
    int alignWidth = -1;
};

MenuLine *addMenuLine(MenuContext *ctx, MenuItemType t, char *label);
MenuLine *addMenuLine(MenuContext *ctx, char *label);
MenuLine *addMenuLine(MenuContext *ctx, char *label, bool *b);
MenuLine *addMenuLine(MenuContext *ctx, char *label, SDL_Keycode *key);

void menuLineText(MenuContext *ctx, char *buf, int n, MenuLine item);
Rect menuLineDimensions(MenuContext *ctx, MenuLine item);
int prevHotLine(MenuContext *ctx);
int nextHotLine(MenuContext *ctx);
void menuInteract(MenuContext *ctx, MenuLine *item);
void drawMenu(Renderer *r, MenuContext *ctx, DrawOpts2d hotOpts);

// app.cpp
//

#define INIT_SDL     (1 << 0)
#define INIT_AUDIO   (1 << 1)
#define INIT_DEFAULT ( \
        INIT_SDL | INIT_AUDIO \
    )

struct App {
    const char *title;
    int width;
    int height;
    int mode;
    Window window;

    struct timespec last;
    struct timespec now;

    InputData lastInput;
};

bool initApp(App *app, int mode);
InputData step(App *app);
void cleanup(App *app);

#ifdef CORE_IMPL
#include "array.c"
#include "util.cpp"
#include "serde.cpp"
#include "vector.cpp"
#include "tween.cpp"
#include "matrix.cpp"
#include "color.cpp"
#include "rect.cpp"
#include "mesh.cpp"
#include "renderer.cpp"
#include "draw.cpp"
#include "shaders.cpp"
#include "sound.cpp"
#include "text.cpp"
#include "sprites.cpp"
#include "window.cpp"
#include "watcher.cpp"
#include "menu.cpp"
#include "app.cpp"
#endif // CORE_IMPL

#endif // CORE_H
