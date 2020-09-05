#include "core.h"

void initRenderer(Renderer *r, uint32_t w, uint32_t h) {
    r->screenWidth = w;
    r->screenHeight = h;

    glActiveTexture(GL_TEXTURE0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  

    glGenVertexArrays(1, &r->vao);
    glBindVertexArray(r->vao);

    glGenBuffers(1, &r->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, r->vbo);

    glGenTextures(1, &r->defaultTexture);
    //glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, r->defaultTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    uint32_t flat_pixel= 0xffffffff;
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, (void*)&flat_pixel);

    clear(r, black);
}

void clear(Renderer *r, Color c) {
    glClearColor(c.r, c.g, c.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void clear(Renderer *r) {
    clear(r, black);
}

inline void useShader(Renderer *r, ShaderProgram *s) {
    if (r->currentShader != s || r->currentShaderHandle != s->handle) {
        //flush(r);
        glUseProgram(s->handle);
        r->currentShader = s;
        r->currentShaderHandle = s->handle;
        glUniformMatrix4fv(r->currentShader->uniforms.projLoc, 1, GL_FALSE, (const GLfloat *)&r->proj);
    }
}

inline void useTexture(Renderer *r, TextureHandle t) {
    t = (t == -1) ? r->defaultTexture : t;
    if (r->currentTexture != t) {
        //flush(r);
        glBindTexture(GL_TEXTURE_2D, t);
        r->currentTexture = t;
    }
}

bool setupDefaultShaders(Renderer *r) {
    const unsigned char *defaultVertexShaderText = readFile("assets/shaders/default.vs");
    if (!defaultVertexShaderText) {
        return false;
    }

    const unsigned char *defaultFragShaderText = readFile("assets/shaders/default.fs");
    if (!defaultFragShaderText) {
        return false;
    }

    // TODO: free file text
    
    if (!compileShader(&r->defaultShader.vert, defaultVertexShaderText, GL_VERTEX_SHADER)) {
        return false;
    }
    if (!compileShader(&r->defaultShader.frag, defaultFragShaderText, GL_FRAGMENT_SHADER)) {
        return false;
    }

    if (!compileShaderProgram(&r->defaultShader)) {
        return false;
    }

    // TODO(ktravis): we should instead do this stuff in the vao load which
    // would be done in a "load mesh" routine - part of doing that means rather
    // than generating quad vertexdata we should set transform based on desired
    // w/h and keep the one quad mesh constant.

    GLint posAttrib = glGetAttribLocation(r->defaultShader.handle, "position");
    if (posAttrib == -1) {
        printf("%s is not a valid glsl program variable\n", "position");
        return false;
    }
    glEnableVertexAttribArray(posAttrib);
    glVertexAttribPointer(posAttrib, 4, GL_FLOAT, GL_FALSE, sizeof(VertexData), 0);

    GLint texAttrib = glGetAttribLocation(r->defaultShader.handle, "texCoord");
    if (texAttrib == -1) {
        printf("%s is not a valid glsl program variable\n", "tc");
        return false;
    }
    glEnableVertexAttribArray(texAttrib);
    glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)offsetof(VertexData, tex));

    return true;
}

void cleanupRenderer(Renderer *r) {
    glDeleteProgram(r->defaultShader.handle);
    glDeleteBuffers(1, &r->vbo);
    glDeleteVertexArrays(1, &r->vao);
}

void renderToScreen(Renderer *r) {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, r->screenWidth, r->screenHeight);
    GLfloat clipNear = 0.0f;
    GLfloat clipFar = 10.0f;
    ortho(r->proj, 0, r->screenWidth, 0, r->screenHeight, clipNear, clipFar);
    if (r->currentShader)
        glUniformMatrix4fv(r->currentShader->uniforms.projLoc, 1, GL_FALSE, (const GLfloat *)&r->proj);
}

bool createRenderTarget(RenderTarget *rt, uint32_t w, uint32_t h) {
    rt->w = w;
    rt->h = h;
    glGenFramebuffers(1, &rt->fb);
    glBindFramebuffer(GL_FRAMEBUFFER, rt->fb);

    glGenTextures(1, &rt->tex);
    glBindTexture(GL_TEXTURE_2D, rt->tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, rt->tex, 0);
    GLenum drawbuf = GL_COLOR_ATTACHMENT0;
    glDrawBuffers(1, &drawbuf);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        return false;

    glViewport(0, 0, rt->w, rt->h);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return true;
}

bool renderToTexture(Renderer *r, RenderTarget *rt) {
    glBindFramebuffer(GL_FRAMEBUFFER, rt->fb);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        return false;

    glViewport(0, 0, rt->w, rt->h);
    GLfloat clipNear = 0.0f;
    GLfloat clipFar = 10.0f;
    ortho(r->proj, 0, (float)rt->w, 0, (float)rt->h, clipNear, clipFar);
    if (r->currentShader)
        glUniformMatrix4fv(r->currentShader->uniforms.projLoc, 1, GL_FALSE, (const GLfloat *)&r->proj);
    return true;
}

GLenum glDrawMode(DrawMode m) {
    switch (m) {
    case Triangles:
        return GL_TRIANGLES;
    case Lines:
        return GL_LINES;
    default:
        debug("unrecognized DrawMode %d", m);
    }
    return GL_TRIANGLES;
}

// TODO: group by common mesh / options and draw in batches
void draw(Renderer *r, DrawCall call) {
    // TODO(ktravis): mesh should have its own vbo that doesn't need to be
    // reloaded every time?
    glBufferData(GL_ARRAY_BUFFER, call.mesh->count*sizeof(VertexData), call.mesh->data, GL_DYNAMIC_DRAW);
    // TODO(ktravis): useTexture use in more places?
    useTexture(r, call.texture);
    glUniformMatrix4fv(r->currentShader->uniforms.modelviewLoc, 1, GL_FALSE, (const GLfloat *)&call.uniforms.model);
    glUniform4fv(r->currentShader->uniforms.tintLoc, 1, (const GLfloat *)&call.uniforms.tint);
    glLineWidth(call.uniforms.lineWidth);
    glDrawArrays(glDrawMode(call.mode), 0, call.mesh->count);
}
