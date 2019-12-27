#include "core.h"

int watchPath(Watcher *w, const char *path, uint32_t events) {
    struct pollfd p;
    if ((p.fd = inotify_init1(IN_NONBLOCK)) < 1) {
        perror("watch");
        return -1;
    }
    p.events = POLLIN | POLLRDNORM;
    if (inotify_add_watch(p.fd, path, events) < 1) {
        perror("watch");
        return -1;
    }
    int idx = array_len(w->pollees);
    array_push(w->pollees, p);
    array_push(w->updated, false);
    return idx;
}

int watchPath(Watcher *w, const char *path) {
    return watchPath(w, path, WATCH_DEFAULT);
}

bool checkWatches(Watcher *w, int ms) {
    for (int i = 0; i < array_len(w->pollees); i++) {
        w->pollees[i].revents = 0;
    }
    int count = poll(w->pollees, array_len(w->pollees), ms);
    if (count == -1) {
        return false;
    }
    for (int i = 0; i < array_len(w->pollees); i++) {
        struct pollfd p = w->pollees[i];
        if (p.revents & POLLIN) {
            int r = read(p.fd, &w->buf, EVENT_BUF_LEN);
            if (r < 0) {
                return false;
            }
            if (r == 0) {
                continue;
            }
            struct inotify_event *ev = (struct inotify_event *)&w->buf;
            w->updated[i] = true;
        }
    }
    return true;
}

bool wait(Watcher *w) {
    return checkWatches(w, -1);
}

bool peek(Watcher *w, int i) {
    return (i >= 0 && i < array_len(w->pollees) && w->updated[i]);
}

bool updated(Watcher *w, int i) {
    if (!peek(w, i)) {
        return false;
    }
    w->updated[i] = false;
    return true;
}

void cleanupWatcher(Watcher *w) {
    for (int i = 0; i < array_len(w->pollees); i++) {
        close(w->pollees[i].fd);
    }
    array_free(w->pollees);
}
