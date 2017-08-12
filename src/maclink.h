#ifndef MACLINK_H
#define MACLINK_H

#include <czmq.h>
#include <stdint.h>
#include "tack.h"

#ifdef VERBOSE
#define dprintf(...) printf(__VA_ARGS__);
#else
#define dprintf(...)
#endif

typedef struct {
    void *data;
    uint32_t size;
} dsx_dataptr;

typedef struct {
    uint32_t size, id;
    char name[0];
} __attribute__((packed)) dsx_id;

typedef struct {
    uint32_t var1;
    uint32_t var2;
    uint32_t var3;
    uint32_t var4;
    uint32_t var5;
    uint32_t var6;
    uint64_t start_time;
    uint64_t end_time;
    uint32_t var7;
    uint32_t var8;
    uint32_t var9;
    uint32_t var10;
    uint32_t var11;
    uint32_t var12;
    uint32_t rule;
    uint32_t var13;
} __attribute__((packed)) dsx_word_node;

typedef struct {} drg_grammar;
typedef struct {} drg_engine;
typedef struct {} drg_filesystem;
typedef struct {} dsx_result;

typedef struct {
    void *var0;
    unsigned int var1;
    unsigned int flags;
    uint64_t var3;
    uint64_t var4;
    char *phrase;
    dsx_result *result;
    void *var7;
} dsx_end_phrase;

typedef struct {
    void *user;
    char *name;
} dsx_attrib;

typedef struct {
    void *user;
    uint64_t token;
} dsx_paused;

typedef struct {
    void *user;
    uint64_t flags;
} dsx_mimic;

#define _engine maclink_engine
extern drg_engine *_engine;

#define DLAPI extern
#include "api.h"

struct state {
    zsock_t *pubsock;
    zsock_t *cmdsock;
    pthread_t tid;
    pthread_mutex_t publock, keylock;

    tack_t grammars;
    tack_t gkeys, gkfree;

    // dragon state
    const char *micstate;
    void *speaker;
    bool ready;
    uint64_t start_ts;
    uint64_t serial;

    zsock_t *mimic_wait, *mimic_signal;
    bool pause_waiting;
    zsock_t *pause_wait, *pause_signal;
    zsock_t *resume_wait, *resume_signal;
};

extern struct state maclink_state;

typedef struct {
    uint64_t key;
    const char *name, *main_rule;
    drg_grammar *handle;

    bool enabled, exclusive;
    int priority;
    const char *appname;
    unsigned int endkey, beginkey, hypokey;
} maclink_grammar;

#endif
