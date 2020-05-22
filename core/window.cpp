#include "core.h"

bool initWindow(Window *w, const char *title, int width, int height) {
    w->handle = SDL_CreateWindow(title,
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            width, height, SDL_WINDOW_OPENGL);

    // TODO: why does this not seem to work
    SDL_SetWindowResizable(w->handle, SDL_FALSE);

    w->ctx = SDL_GL_CreateContext(w->handle);
    if (!w->ctx) {
        log("unable to create SDL context");
        const char *err = SDL_GetError();
        if (err) {
            log("SDL error: %s", err);
            SDL_ClearError();
        }
        return false;
    }

    // NOTE: this isn't really a part of window creation, but it needs a current
    // GL context in order to succeed.
    glewExperimental = GL_TRUE;
    GLenum glewError = glewInit();
    if (glewError != GLEW_OK) {
        log("glew setup failed: %s", glewGetErrorString(glewError));
        return false;
    }
    return true;
}

//void resizeWindow(Window *w, int w, int h) {
    //SDL_SetWindowSize(w->handle, w, h);
//}

void swapWindow(Window *w) {
    SDL_GL_SwapWindow(w->handle);
}

void cleanupWindow(Window *w) {
	if (w->ctx) SDL_GL_DeleteContext(w->ctx);
	if (w->handle) SDL_DestroyWindow(w->handle);
}
