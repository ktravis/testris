#define CORE_IMPL
#include "core/core.h"
#undef CORE_IMPL

#include "game.h"

#ifndef __EMSCRIPTEN__
#include <pthread.h>

void *watchLoop(Watcher *w) {
    while (true) {
        if(!wait(w)) {
            perror("poll");
        }
    }
}

typedef void *(*threadFunc)(void*);
#endif

#define WINDOW_WIDTH   600
#define WINDOW_HEIGHT  800
#define MAX_FRAME_RATE 120
#define MIN_FRAME_TIME (1000.0/MAX_FRAME_RATE)

Renderer r;

const char *title = "testris";
App app = {
   .title  = title,
   .width  = WINDOW_WIDTH,
   .height = WINDOW_HEIGHT,
};

void resizeWindow(int w, int h) {
    app.width = w;
    app.height = h;
    r.screenWidth = (float)w;
    r.screenHeight = (float)h;
    renderToScreen(&r);
}

GameState st = {};

bool step() {
    clear(&r, black);
    InputData in = step(&app);

    bool result = updateGameState(&st, in);

    clear(&r, black);
    renderGameState(&r, &st);

    swapWindow(&app.window);
    if (in.dt < MIN_FRAME_TIME) SDL_Delay((uint32_t)(MIN_FRAME_TIME - in.dt));
    return result;
}

void step_forever() {
    step();
}

int main(int argc, char *argv[]) {
#ifdef __EMSCRIPTEN__
    EM_ASM(
        // Make a directory other than '/'
        FS.mkdir('/offline');
        // Then mount with IDBFS type
        FS.mount(IDBFS, {}, '/offline');

        // Then sync
        FS.syncfs(true, function (err) {
            if (err !== null)
                console.error(err);
        });
    );
#endif

    if (!initApp(&app, INIT_DEFAULT)) {
        log("init failed, exiting");
        return 1;
    }

    app.setWindowSize = resizeWindow;

    // setup default 2d renderer
    initRenderer(&r, app.width, app.height);
    if (!setupDefaultShaders(&r)) {
        // TODO: really we should be jumping to the cleanup routine here rather
        // than returning, but for now this is fine.
        return -1;
    }

    renderToScreen(&r);

    if (!startGame(&st, &r, &app)) {
        log("start game failed, exiting");
        // TODO: really we should be jumping to the cleanup routine here rather
        // than returning, but for now this is fine.
        return 1;
    }

#ifdef __EMSCRIPTEN__
#define REQUEST_ANIMATION_FRAME 0
    emscripten_set_main_loop(step_forever, REQUEST_ANIMATION_FRAME, 1);
#else
    // setup watched resources and watcher loop in thread
    Watcher watcher = {};
    int shaderWatch = watchPath(&watcher, "./assets/shaders");

    pthread_t watchThread;
    if (pthread_create(&watchThread, NULL, (threadFunc)watchLoop, (void*)&watcher)) {
        log("pthread creation failed");
        // TODO: really we should be jumping to the cleanup routine here rather
        // than returning, but for now this is fine.
        return 1;
    }

    while (step()) {
        if (updated(&watcher, shaderWatch)) {
            debug("SHADERS UPDATED");
            if (!setupDefaultShaders(&r)) {
                log("failed to update shaders");
            }
            if (!compileExtraShaders(&r)) {
                log("failed to update shaders");
            }
        }
    }
    cleanupWatcher(&watcher);
    cleanupRenderer(&r);
    cleanupApp(&app);
#endif

    return -1;
}
