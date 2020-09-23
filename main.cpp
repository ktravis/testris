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

#define WINDOW_WIDTH  600
#define WINDOW_HEIGHT 800

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

GameState savedState = {};
int historyCursor = 0;
int totalSaved = 0;
InputData savedInputs[2048];
const int historyCount =  sizeof(savedInputs)/sizeof(savedInputs[0]);
bool recording = false;
bool playback = false;

bool step() {
    clear(&r, black);
    InputData in = step(&app);

    if (recording) {
        if (keyState(&in, SDLK_BACKSLASH).down) {
            recording = false;
            playback = true;
            historyCursor = 0;
        } else {
            int next = (historyCursor++) % historyCount;
            if (next > totalSaved) totalSaved = next;
            savedInputs[next] = in;
        }
    } else if (playback) {
        if (anyKeyPress(&in)) {
            playback = false;
        } else {
            if ((historyCursor%totalSaved) == 0) {
                st = savedState;
            }
            in = savedInputs[(historyCursor++) % totalSaved];
        }
    } else {
        if (keyState(&in, SDLK_BACKSPACE).down) {
            savedState = st;
            historyCursor = 0;
            totalSaved = 0;
            recording = true;
            playback = false;
            log("save");
        }
    }

    bool result = updateGameState(&st, in);

    clear(&r, black);
    renderGameState(&r, &st);

    swapWindow(&app.window);
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

        //if (updated(&watcher, levelWatch)) {
            //debug("LEVELS UPDATED");
            //if (!loadLevel(st, "assets/levels/1.txt")) {
                //log("level loading failed");
            //}
        //}
    }
    cleanupWatcher(&watcher);
    cleanupRenderer(&r);
    cleanupApp(&app);
#endif

    return -1;
}
