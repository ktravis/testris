#include "core.h"

static SDL_AudioSpec hw_audio_spec;

static int32_t next_handle;
static int32_t next_sound_id;

#define MAX_LOADED_SOUNDS 256
static Sound loaded_sounds[MAX_LOADED_SOUNDS];

#define MAX_PLAYING_SOUNDS 16
static Playing playing_sounds[MAX_PLAYING_SOUNDS];

SoundProps globalSoundProps = {.volume = 1.0f};
SoundProps defaultSoundProps = {.volume = 1.0f};

void stopAllSounds() {
    for (int i = 0; i < MAX_PLAYING_SOUNDS; i++) {
        playing_sounds[i].active = false;
    }
}

bool stopSoundByID(int32_t id) {
    bool was_playing = false;
    for (int i = 0; i < MAX_PLAYING_SOUNDS; i++) {
        if (playing_sounds[i].soundID == id) {
            was_playing = was_playing || playing_sounds[i].active;
            playing_sounds[i].active = false;
        }
    }
    return was_playing;
}

bool stopSound(int32_t handle) {
    bool was_playing = false;
    for (int i = 0; i < MAX_PLAYING_SOUNDS; i++) {
        if (playing_sounds[i].handle == handle) {
            was_playing = playing_sounds[i].active;
            playing_sounds[i].active = false;
            break;
        }
    }
    return was_playing;
}

int32_t playSound(int32_t id, SoundProps props) {
    assert(id < next_sound_id);
    Playing *p = NULL;
    for (int i = 0; i < MAX_PLAYING_SOUNDS; i++) {
        if (!playing_sounds[i].active) {
            p = &playing_sounds[i];
            break;
        } else if (!p || playing_sounds[i].handle < p->handle) {
            p = &playing_sounds[i];
        }
    }
    assert(p);

    // TODO: lock

    p->handle = next_handle++;
    p->soundID = id;
    p->active = true;
    p->s = &loaded_sounds[id];
    p->cur = 0;
    p->props = props;
    return p->handle;
}

static bool muted = false;

bool setMuted(bool m) {
    muted = m;
    globalSoundProps.volume = muted ? 0.0f : 1.0f;
    return muted;
}

bool toggleMute() {
    return setMuted(!muted);
}

int32_t playSound(int32_t id) {
    return playSound(id, defaultSoundProps);
}

void mixAudioS16LE(uint8_t *dst, uint8_t *src, uint32_t len_bytes, SoundProps props) {
    if (globalSoundProps.volume == 0.0f || props.volume == 0.0f) {
        return;
    }
    SDL_MixAudio(dst, src, len_bytes, props.volume * globalSoundProps.volume * SDL_MIX_MAXVOLUME);
}

void audioCallback(void *user_data, uint8_t *stream, int32_t len) {
    memset(stream, 0, len);

    for (int i = 0; i < MAX_PLAYING_SOUNDS; i++) {
        Playing *p = &playing_sounds[i];
        if (!p->active) {
            continue;
        }
        uint32_t play_len = len;
        if (p->cur + len > p->s->len) {
            play_len = p->s->len - p->cur;
        }

        mixAudioS16LE(stream, p->s->buf + p->cur, play_len, p->props);
        p->cur += len;
        if (p->cur >= p->s->len) {
            p->active = false;
        }
    }
}

int32_t loadAudioAndConvert(const char *filename) {
    SDL_AudioCVT cvt;
    SDL_AudioSpec loaded;
    int32_t id = next_sound_id++;
    assert(id < MAX_LOADED_SOUNDS);
    Sound *s = &loaded_sounds[id];
    s->name = filename;
    if (SDL_LoadWAV(filename, &loaded, &s->buf, &s->len) == NULL) {
        fprintf(stderr, "error loading sound %s: %s\n", filename, SDL_GetError());
        return -1;
    }
    printf("loaded format: channels %d samples %d freq %d\n", loaded.channels, loaded.samples, loaded.freq);
    printf("hw format: channels %d samples %d freq %d\n", hw_audio_spec.channels, hw_audio_spec.samples, hw_audio_spec.freq);

    if (SDL_BuildAudioCVT(&cvt, loaded.format, loaded.channels, loaded.freq,
        hw_audio_spec.format, hw_audio_spec.channels, hw_audio_spec.freq) < 0) {
        fprintf(stderr, "error converting sound %s: %s\n", filename, SDL_GetError());
        SDL_FreeWAV(s->buf);
        return -1;
    }

    cvt.len = s->len;
    printf("loaded file %s len %d len mult %d\n", filename, cvt.len, cvt.len_mult);
    uint8_t *new_buf = (uint8_t *)malloc(cvt.len * cvt.len_mult);
    if (!new_buf) {
        fprintf(stderr, "error converting sound %s: unable to allocate %d bytes\n",
                filename, cvt.len * cvt.len_mult);
        SDL_FreeWAV(s->buf);
        return -1;
    }
    memcpy(new_buf, s->buf, s->len);

    cvt.buf = new_buf;
    if (SDL_ConvertAudio(&cvt) < 0) {
        fprintf(stderr, "error converting sound %s: %s\n", filename, SDL_GetError());
        SDL_FreeWAV(s->buf);
        free(new_buf);
        return -1;
    }

    SDL_FreeWAV(s->buf);
    s->buf = cvt.buf;
    s->len = cvt.len_cvt;

    return id;
}

bool openAudioDevice() {
    SDL_AudioSpec desired;
    desired.freq = 44100;
    desired.format = AUDIO_S16;
    desired.samples = 4096;
    desired.channels = 2;
    desired.callback = audioCallback;
    desired.userdata = NULL;

    if (SDL_OpenAudio(&desired, &hw_audio_spec) < 0) {
        fprintf(stderr, "audio failure: %s\n", SDL_GetError());
        return false;
    }
    // TODO(ktravis): hmmm
    //if (hw_audio_spec.format != desired.format) {
        //fprintf(stderr, "couldn't get desired audio format\n");
        //return false;
    //}

    return true;
}

void cleanupAudio() {
    SDL_PauseAudio(1);
    SDL_LockAudio();
    for (int i = 0; i < next_sound_id; i++) {
        free(loaded_sounds[i].buf);
    }
    SDL_UnlockAudio();
    SDL_CloseAudio();
}
