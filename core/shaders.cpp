#include "core.h"

void glPrintLog(GLuint id) {
    int infoLogLength = 0;
    int maxLength = 0;
    char *buf;
    if (glIsShader(id)) {
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &maxLength);
        buf = (char *)malloc(sizeof(char) * maxLength);
        glGetShaderInfoLog(id, maxLength, &infoLogLength, buf);
    } else if (glIsProgram(id)) {
        glGetProgramiv(id, GL_INFO_LOG_LENGTH, &maxLength);
        buf = (char *)malloc(sizeof(char) * maxLength);
        glGetProgramInfoLog(id, maxLength, &infoLogLength, buf);
    } else {
        log("GL ID %d not recognized", id);
        return;
    }
    if (infoLogLength > 0) {
        log(buf);
    }
    free(buf);
}

bool compileShader(ShaderHandle *h, const unsigned char *text, GLenum type) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, (const char* *)&text, NULL);
    glCompileShader(s);

    GLint success = GL_FALSE;
    glGetShaderiv(s, GL_COMPILE_STATUS, &success);
    if (success != GL_TRUE) {
        fprintf(stderr, "Unable to compile vertex shader %d!\n", s);
        glPrintLog(s);
        return false;
    }
    *h = s;

    return true;
}

void dumpUniformNames(ShaderHandle s) {
    GLint count;
    glGetProgramiv(s, GL_ACTIVE_UNIFORMS, &count);
    log("shader %d has %d uniforms", s, count);
    for (int i = 0; i < count; i++) {
        GLint size;
        GLenum type;
        GLchar name[255];
        glGetActiveUniform(s, i, sizeof(name), NULL, &size, &type, name);
        log("  %s - %d (%d)", name, type, size);
    }
}

bool compileShaderProgram(ShaderProgram *prog) {
    if (prog->handle) {
        glDeleteProgram(prog->handle);
    }
    prog->handle = glCreateProgram();
    glAttachShader(prog->handle, prog->vert);
    glAttachShader(prog->handle, prog->frag);
    glLinkProgram(prog->handle);

    GLint success = GL_FALSE;
    glGetProgramiv(prog->handle, GL_LINK_STATUS, &success);
    if (success != GL_TRUE) {
        fprintf(stderr, "Error shader linking program %d\n", prog->handle);
        glPrintLog(prog->handle);
        return false;
    }

    glUseProgram(prog->handle);
    dumpUniformNames(prog->handle);

    prog->uniforms.timeLoc = glGetUniformLocation(prog->handle, "time");
    glUniform1f(prog->uniforms.timeLoc, 0.0f);

    prog->uniforms.modelviewLoc = glGetUniformLocation(prog->handle, "modelview");
    Mat4 modelview;
    identity(modelview);
    glUniformMatrix4fv(prog->uniforms.modelviewLoc, 1, GL_FALSE, (const GLfloat *)modelview);

    prog->uniforms.projLoc = glGetUniformLocation(prog->handle, "proj");
    Mat4 proj;
    identity(proj);
    glUniformMatrix4fv(prog->uniforms.projLoc, 1, GL_FALSE, (const GLfloat *)proj);

    prog->uniforms.texLoc = glGetUniformLocation(prog->handle, "tex");
    glUniform1i(prog->uniforms.texLoc, 0);

    prog->uniforms.tintLoc = glGetUniformLocation(prog->handle, "tint");
    glUniform4fv(prog->uniforms.tintLoc, 1, (const GLfloat *)&white);

    prog->uniforms.mouseLoc = glGetUniformLocation(prog->handle, "mouse");
    if (prog->uniforms.mouseLoc != -1) {
        Vec2 v = {};
        glUniform2fv(prog->uniforms.mouseLoc, 1, (const GLfloat *)&v);
    }

    glUseProgram(0);

    return true;
}
