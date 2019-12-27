#include "core.h"

#define DEFAULT_WINDOW_WIDTH 640
#define DEFAULT_WINDOW_HEIGHT 480

inline float updateFrameTime(App *app) {
    app->last = app->now;
    clock_gettime(CLOCK_MONOTONIC, &app->now);
    return (float)diffMilli(&app->last, &app->now);
}

bool initApp(App *app, int mode) {
    if  (app->width == 0) app->width = DEFAULT_WINDOW_WIDTH;
    if (app->height == 0) app->height = DEFAULT_WINDOW_HEIGHT;

    app->mode = mode;
    if (mode & INIT_SDL) {
        debug("initializing SDL2");
        int sdl_mode = SDL_INIT_VIDEO;
        if (mode & INIT_AUDIO) {
            sdl_mode |= SDL_INIT_AUDIO;
        }
        if (SDL_Init(sdl_mode) < 0) {
            log("Failed to load SDL: %s", SDL_GetError());
            return false;
        }

        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    }
    if (mode & INIT_AUDIO) {
        debug("initializing audio");

        if (!openAudioDevice()) {
            return false;
        }

        SDL_PauseAudio(0);
    }

    if (!initWindow(&app->window, app->title, app->width, app->height)) {
        log("failed to create window");
        return false;
    }

    updateFrameTime(app);
    srand(time(NULL));
    return true;
}

InputData step(App *app) {
    // TODO: collect mouse delta and carry over x/y by swapping between two
    // input events
    InputData in = {};
    in.mouse = app->lastInput.mouse;

    in.dt = updateFrameTime(app);

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_QUIT:
            in.quit = true;
            break;
        case SDL_MOUSEBUTTONDOWN:
            if(event.button.button == 1) {
                in.lmb.down = true;
            }
            break;
        case SDL_MOUSEBUTTONUP:
            if(event.button.button == 1) {
                in.lmb.up = true;
            }
            // TODO: rmb
            break;
        case SDL_MOUSEMOTION:
            in.mouseMoved = true;
            in.mouse = vec2(event.motion.x, event.motion.y);
            in.deltaMouse = sub(in.mouse, app->lastInput.mouse);
            break;
        case SDL_KEYUP:
        case SDL_KEYDOWN:
            {
                int i = in.keyEventBufferOffset + (in.numKeyEvents % KEY_EVENT_BUFCOUNT);
                KeyEvent *e = &in.keys[i];

                e->key = event.key.keysym.sym;
                e->state.up   = (event.type == SDL_KEYUP);
                e->state.down = (event.type == SDL_KEYDOWN);

                if (in.numKeyEvents < KEY_EVENT_BUFCOUNT) {
                    in.numKeyEvents++;
                } else {
                    in.keyEventBufferOffset = (in.keyEventBufferOffset + 1) % KEY_EVENT_BUFCOUNT;
                }
            }
            break;
        case SDL_WINDOWEVENT_SIZE_CHANGED:
            break;
        }
    }
    app->lastInput = in;
    return in;
}

bool anyKeyPress(InputData *in) {
    for (int i = 0; i < in->numKeyEvents; i++) {
        KeyEvent e = in->keys[(in->keyEventBufferOffset + i) % KEY_EVENT_BUFCOUNT];
        if (e.state.down) return true;
    }
    return false;
}

void cleanupApp(App *app) {
    if (app->mode & INIT_AUDIO) cleanupAudio();
    cleanupWindow(&app->window);
    if (app->mode & INIT_SDL) SDL_Quit();
}
