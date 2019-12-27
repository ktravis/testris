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

bool compileShader(GLuint *prog, const unsigned char *vs, const unsigned char *fs) {
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, (const char* const*)&vs, NULL);
    glCompileShader(vertexShader);

    GLint success = GL_FALSE;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (success != GL_TRUE) {
        fprintf(stderr, "Unable to compile vertex shader %d!\n", vertexShader);
        glPrintLog(vertexShader);
        return false;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, (const char* const*)&fs, NULL);
    glCompileShader(fragmentShader);

    success = GL_FALSE;
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (success != GL_TRUE) {
        fprintf(stderr, "Unable to compile fragment shader %d!\n", fragmentShader);
        glPrintLog(fragmentShader);
        return false;
    }

    GLuint progID = glCreateProgram();
    glAttachShader(progID, vertexShader);
    glAttachShader(progID, fragmentShader);
    glBindFragDataLocation(progID, 0, "outColor");
    glLinkProgram(progID);

    success= GL_TRUE;
    glGetProgramiv(progID, GL_LINK_STATUS, &success);
    if (success != GL_TRUE) {
        fprintf(stderr, "Error shader linking program %d\n", progID);
        glPrintLog(progID);
        return false;
    }

    *prog = progID;
    return true;
}
