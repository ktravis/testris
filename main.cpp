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

#define WINDOW_WIDTH  1200
#define WINDOW_HEIGHT 1600

const char *title = "testris";
App app = {
   .title  = title,
   .width  = WINDOW_WIDTH,
   .height = WINDOW_HEIGHT,
   .scaleFactor = 2.0f,
};

Renderer r;
    
GameState st = {};

RenderTarget boi;

bool step() {
    clear(&r, black);
    InputData in = step(&app);
    bool result = updateGameState(&st, in);

    assert(renderToTexture(&r, &boi));
    clear(&r, red);
    renderGameState(&r, &st);
    drawRectOutline(&r, rect(40, 40, 100, 100), white, 5);

    renderToScreen(&r);
    renderGameState(&r, &st);
    //r.screenWidth = st.thing * st.width;
    //r.screenHeight = st.thing * st.height;
    DrawOpts2d opts = {};
    drawTexturedQuad(&r, in.mouse, boi.w, boi.h, boi.tex, rect(0, 1, 1, 0), opts);
    swapWindow(&app.window);
    return result;
}

int main(int argc, char *argv[]) {
#ifdef __EMSCRIPTEN__
    // EM_ASM is a macro to call in-line JavaScript code.
    //EM_ASM(
        //// Make a directory other than '/'
        //FS.mkdir('/offline');
        //// Then mount with IDBFS type
        //FS.mount(IDBFS, {}, '/offline');

        //// Then sync
        //FS.syncfs(true, function (err) {
            //if (err !== null)
                //console.error(err);
        //});
    //);
#endif

    if (!initApp(&app, INIT_DEFAULT)) {
        log("init failed, exiting");
        return 1;
    }

    // setup default 2d renderer
    initRenderer(&r, app.width, app.height);
    if (!setupDefaultShaders(&r)) {
        // TODO: really we should be jumping to the cleanup routine here rather
        // than returning, but for now this is fine.
        return -1;
    }

    if (!createRenderTarget(&boi, app.width, app.height)) {
        log("failed creating render target");
        return 1;
    }

    //renderToTexture(&r, &buf, app.width, app.height);
    renderToScreen(&r);

    st.width = app.width;
    st.height = app.height;
    st.scaleFactor = app.scaleFactor;
    if (!startGame(&st, &r)) {
        log("start game failed, exiting");
        // TODO: really we should be jumping to the cleanup routine here rather
        // than returning, but for now this is fine.
        return 1;
    }

#ifdef __EMSCRIPTEN__
#define REQUEST_ANIMATION_FRAME 0
    emscripten_set_main_loop(step, REQUEST_ANIMATION_FRAME, 1);
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
