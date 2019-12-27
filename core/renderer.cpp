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

inline void useShader(Renderer *r, ShaderHandle s) {
    if (r->currentShader != s) {
        //flush(r);
        glUseProgram(s);
        r->currentShader = s;
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

void attachProjectionMatrix(Renderer *r, Mat4 proj) {
    useShader(r, r->defaultShader);
    glUniformMatrix4fv(r->defaultProjLoc, 1, GL_FALSE, (const GLfloat *)proj);
}

bool setupShaders(Renderer *r) {
    const unsigned char *defaultVertexShader;
    const unsigned char *defaultFragShader;
    if (!(defaultVertexShader = readFile("assets/shaders/default.vs"))) {
        return false;
    }
    if (!(defaultFragShader = readFile("assets/shaders/default.fs"))) {
        return false;
    }

    // TODO: free file text

    GLuint qs;
    if (!compileShader(&qs, defaultVertexShader, defaultFragShader)) {
        return false;
    }

    Mat4 model;
    identity(model);

    glUseProgram(qs);
    r->defaultProjLoc = glGetUniformLocation(qs, "proj");
    glUniformMatrix4fv(r->defaultProjLoc, 1, GL_FALSE, (const GLfloat *)r->proj);

    r->defaultModelLoc = glGetUniformLocation(qs, "model");
    glUniformMatrix4fv(r->defaultModelLoc, 1, GL_FALSE, (const GLfloat *)model);

    GLint posAttrib = glGetAttribLocation(qs, "position");
    if (posAttrib == -1) {
        printf("%s is not a valid glsl program variable\n", "position");
        return false;
    }
    glEnableVertexAttribArray(posAttrib);
    glVertexAttribPointer(posAttrib, 4, GL_FLOAT, GL_FALSE, sizeof(VertexData), 0);

    GLint colAttrib = glGetAttribLocation(qs, "c");
    if (colAttrib == -1) {
        printf("%s is not a valid glsl program variable\n", "position");
        return false;
    }
    glEnableVertexAttribArray(colAttrib);
    glVertexAttribPointer(colAttrib, 4, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)offsetof(VertexData, color));

    GLint texAttrib = glGetAttribLocation(qs, "tc");
    if (texAttrib == -1) {
        printf("%s is not a valid glsl program variable\n", "tc");
        return false;
    }
    glEnableVertexAttribArray(texAttrib);
    glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)offsetof(VertexData, tex));

    glUniform1i(glGetUniformLocation(qs, "tex"), 0);

    r->tintLoc = glGetUniformLocation(qs, "tint");
    glUniform4fv(r->tintLoc, 1, (const GLfloat *)&white);

    if (r->defaultShader) {
        glDeleteProgram(r->defaultShader);
    }
    r->defaultShader = qs;

    attachProjectionMatrix(r, r->proj);
    return true;
}

void cleanupRenderer(Renderer *r) {
    glDeleteProgram(r->defaultShader);
    glDeleteBuffers(1, &r->vbo);
    glDeleteVertexArrays(1, &r->vao);
}

void renderToScreen(Renderer *r) {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, r->screenWidth, r->screenHeight);

    GLfloat clipNear = 0.0f;
    GLfloat clipFar = 10.0f;
    ortho(r->proj, 0, r->screenWidth, 0, r->screenHeight, clipNear, clipFar);
    attachProjectionMatrix(r, r->proj);
}

bool renderToTexture(Renderer *r, TextureHandle *tex, uint32_t w, uint32_t h) {
    GLuint fb = 0;
    glGenFramebuffers(1, &fb);
    glBindFramebuffer(GL_FRAMEBUFFER, fb);

    glGenTextures(1, tex);
    glBindTexture(GL_TEXTURE_2D, *tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, *tex, 0);
    GLenum drawbuf = GL_COLOR_ATTACHMENT0;
    glDrawBuffers(1, &drawbuf);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        return false;

    glViewport(0, 0, w, h);
    GLfloat clipNear = 0.0f;
    GLfloat clipFar = 10.0f;
    ortho(r->proj, 0, w, h, 0, clipNear, clipFar);
    attachProjectionMatrix(r, r->proj);

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
    glBufferData(GL_ARRAY_BUFFER, call.mesh->count*sizeof(VertexData), call.mesh->data, GL_DYNAMIC_DRAW);
    useTexture(r, call.texture);
    glUniformMatrix4fv(r->defaultModelLoc, 1, GL_FALSE, (const GLfloat *)call.uniforms.model);
    glUniform4fv(r->tintLoc, 1, (GLfloat *)&call.uniforms.tint);
    glLineWidth(call.uniforms.lineWidth);
    glDrawArrays(glDrawMode(call.mode), 0, call.mesh->count);
}
