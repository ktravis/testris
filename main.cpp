#define CORE_IMPL
#include "core/core.h"
#undef CORE_IMPL

#include "game.h"

#include <pthread.h>

void *watchLoop(Watcher *w) {
    while (true) {
        if(!wait(w)) {
            perror("poll");
        }
    }
}

typedef void *(*threadFunc)(void*);

#define WINDOW_WIDTH  600
#define WINDOW_HEIGHT 800

int main(int argc, char *argv[]) {
    const char *title = "testris";
    App app = {
       .title  = title,
       .width  = WINDOW_WIDTH,
       .height = WINDOW_HEIGHT,
    };
    if (!initApp(&app, INIT_DEFAULT)) {
        log("init failed, exiting");
        return 1;
    }

    // setup default 2d renderer
    Renderer r;
    initRenderer(&r, app.width, app.height);
    if (!setupShaders(&r)) {
        // TODO: really we should be jumping to the cleanup routine here rather
        // than returning, but for now this is fine.
        return -1;
    }
    renderToScreen(&r);

    // setup watched resources and watcher loop in thread
    Watcher watcher = {};
    int shaderWatch = watchPath(&watcher, "./assets/shaders");
    //int levelWatch = watchPath(&watcher, "./assets/levels");
    pthread_t watchThread;
    if (pthread_create(&watchThread, NULL, (threadFunc)watchLoop, (void*)&watcher)) {
        log("pthread creation failed");
        // TODO: really we should be jumping to the cleanup routine here rather
        // than returning, but for now this is fine.
        return 1;
    }

    GameState st = {};
    st.width = app.width;
    st.height = app.height;
    if (!startGame(&st)) {
        log("start game failed, exiting");
        // TODO: really we should be jumping to the cleanup routine here rather
        // than returning, but for now this is fine.
        return 1;
    }

    while (true) {
        clear(&r, black);
        InputData in = step(&app);
        updateGameState(&st, in);
        if (st.exitNow) {
            break;
        }

        renderGameState(&r, &st);
        swapWindow(&app.window);

        if (updated(&watcher, shaderWatch)) {
            debug("SHADERS UPDATED");
            if (!setupShaders(&r)) {
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
    return -1;
}
