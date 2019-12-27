#include "core.h"

uint8_t *boolToString(bool b) {
    return (uint8_t *)(b ? "true" : "false");
}

// TODO: this is a little silly
bool boolFromString(uint8_t *b) {
    char *s = (char *)b;
    return (!strcmp(s, "t") || !strcmp(s, "T") ||
        !strcmp(s, "true") || !strcmp(s, "True") ||
        !strcmp(s, "TRUE"));
}


uint8_t *_serializeFields(int n, SerDeField *fields) {
    int alloc = 256;
    int used = 0;
    char *buf = (char*)malloc(alloc);
    for (int i = 0; i < n; i++) {
        char linefmt[50];
        snprintf(linefmt, sizeof(linefmt), "%%s: %s\n", fields[i].fmt);
        void *v = *(fields[i].address);
        if (fields[i].ser_fn) {
            v = fields[i].ser_fn(v);
        }
        int n = snprintf(NULL, 0, linefmt, fields[i].name, v);
        if (used + n > alloc) {
            alloc *= 2;
            buf = (char*)realloc(buf, alloc);
        }
        snprintf(&buf[used], n+1, linefmt, fields[i].name, v);
        used += n;
    }
    return (uint8_t*)buf;
}

bool _deserializeFields(uint8_t *buf, int n, SerDeField *fields) {
    while (*buf) {
        uint8_t *line = eatLine(&buf);
        eatSpaces(&line);
        if (!*line) continue;
        uint8_t *name = eatStringUntil(&line, ':');
        trimTrailingSpaces(&name);
        eatSpaces(&line);
        uint8_t *value = line;
        trimTrailingSpaces(&value);
        for (int i = 0; i < n; i++) {
            if (!strcmp(fields[i].name, (char*)name)) {
                uint8_t valbuf[512];
                assert(fields[i].size < 512);
                const char *fmt = fields[i].fmt;
                if (!strcmp(fmt, "%s")) {
                    fmt = "%[^\n]";
                }
                sscanf((char*)value, fmt, valbuf);
                void *v = valbuf;
                if (fields[i].deser_fn) {
                    v = fields[i].deser_fn(v);
                }
                memcpy(fields[i].address, &v, fields[i].size);
                break;
            }
        }
    }
    return true;
}
