#include "core.h"

#define PIX_TRANSPARENT 0xffd34dfd

long diffMicro(struct timespec *start, struct timespec *end) {
    /* us */
    return ((end->tv_sec * (1000000)) + (end->tv_nsec / 1000)) -
        ((start->tv_sec * 1000000) + (start->tv_nsec / 1000));
}

long diffMilli(struct timespec *start, struct timespec *end) {
    /* ms */
    return ((end->tv_sec * 1000) + (end->tv_nsec / 1000000)) -
        ((start->tv_sec * 1000) + (start->tv_nsec / 1000000));
}

uint64_t fileSize(FILE *f) {
    long pos = ftell(f);
    fseek(f, 0, SEEK_END);
    uint64_t n = ftell(f);
    fseek(f, pos, SEEK_SET);
    return n;
}

// This isn't really technically right, but oh well
char *expandPath(const char *path) {
    if (!(path[0] == '~' && path[1] == '/')) {
        return NULL;
    }
    char *filename_buf = NULL;
    int n = snprintf(NULL, 0, "%s%s", getenv("HOME"), path+1);
    filename_buf = (char*)malloc((n+1)*sizeof(char));
    snprintf(filename_buf, n+1, "%s%s", getenv("HOME"), path+1);
    return filename_buf;
}

uint8_t *readFile(const char *filename) {
    char *expanded = expandPath(filename);
    if (expanded) {
        filename = expanded;
    }
    FILE *f = fopen(filename, "rb");
    if (!f) {
        fprintf(stderr, (char*)"error reading file %s: %s\n", filename, strerror(errno));
        free(expanded);
        return 0;
    }

    uint64_t size = fileSize(f);
    uint8_t *buf = (uint8_t *)malloc(sizeof(uint8_t) * (size+1));
    if (!buf) {
        fprintf(stderr, (char*)"there's been a problem allocating %lu bytes\n", size);
        free(expanded);
        return 0;
    }
    fread(buf, 1, size, f);
    fclose(f);
    buf[size] = '\0';
    free(expanded);
    return buf;
}

bool writeFile(const char *filename, const uint8_t *b) {
    char *expanded = expandPath(filename);
    if (expanded) {
        filename = expanded;
    }
    FILE *f = fopen(filename, "wb");
    if (!f) {
        fprintf(stderr, (char*)"error writing to file %s: %s\n", filename, strerror(errno));
        free(expanded);
        return false;
    }
    fprintf(f, "%s", b);
    fclose(f);
    free(expanded);
    return true;
}

uint8_t *eatStringUntil(uint8_t **s, uint8_t d) {
    uint8_t c;
    uint8_t *line = *s;
    uint8_t *start = line;
    while ((c = *line)) {
        if (c == d) {
            *line = '\0';
            *s = line+1;
            return start;
        }
        line++;
    }
    return NULL;
}

void eatSpaces(uint8_t **s) {
    uint8_t c;
    uint8_t *x = *s;
    while ((c = *x) && ((c == ' ') || (c == '\n') || (c == '\t'))) {
        x++;
    }
    *s = x;
    return;
}

uint8_t *eatLine(uint8_t **s) {
    return eatStringUntil(s, '\n');
}

// TODO: this is probably wrong
void trimTrailingSpaces(uint8_t **s) {
    uint8_t *start = *s;
    uint8_t *end = start;
    while ((*end)) {
        end++;
    }
    uint8_t c;
    while (end > start) {
        c = *end;
        if (c && c != ' ' && c != '\t' && c != '\n') {
            *end = '\0';
            return;
        }
        end--;
    }
}

bool loadTextureFile(const char *fname, TextureHandle *tex, int *w, int *h) {
    if (!tex) {
        log("texture handle pointer passed to loadTextureFile was null");
        return false;
    }
    glGenTextures(1, tex);
    //glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, *tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    stbi_uc *pix = stbi_load(fname, w, h, 0, STBI_rgb_alpha);
    if (!pix) {
        log("unable to open file: %s", fname);
        return false;
    }
    // TODO: c'mon man
    for (int i = 0; i < 4 * *w * *h; i += 4) {
        uint32_t *p = (uint32_t*)&pix[i];
        if (*p == PIX_TRANSPARENT) {
            *p = *p & 0x00ffffff;
        }
    }
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, *w, *h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pix);
    stbi_image_free(pix);
    return true;
}

bool loadTextureFile(const char *fname, GLuint *tex) {
    int w, h;
    return loadTextureFile(fname, tex, &w, &h);
}

void vlog(const char *msg, va_list args) {
    vfprintf(stderr, msg, args);
    fprintf(stderr, "\n");
}

void log(const char *msg, ...) {
    va_list args;
    va_start(args, msg);
    vlog(msg, args);
    va_end(args);
}

#ifdef DEBUG
void debug(const char *msg, ...) {
    va_list args;
    va_start(args, msg);
    fprintf(stderr, "[D] ");
    vlog(msg, args);
    va_end(args);
}
#endif

float randf(float max) {
    return max*(rand()/(float)RAND_MAX);
}

int randn(int max) {
    return rand()%max;
}
