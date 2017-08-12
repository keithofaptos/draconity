#include <bson.h>
#include <czmq.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/time.h>

#include "phrase.h"
#include "server.h"
#include "maclink.h"
#include "tack.h"

#define align4(len) ((len + 4) & ~3)

// #define NODRAGON

#ifndef streq
#define streq(a, b) !strcmp(a, b)
#endif

struct state maclink_state = {0};
#define state maclink_state

#define US  1
#define MS  (1000 * US)
#define SEC (1000 * MS)

typedef struct {
    uint64_t key;
    uint64_t ts;
    uint64_t serial;
} reusekey;

void maclink_publish(const char *topic, bson_t *obj) {
    BSON_APPEND_INT64(obj, "ts", bson_get_monotonic_time());
    uint32_t length = 0;
    uint8_t *buf = bson_destroy_with_steal(obj, true, &length);

    pthread_mutex_lock(&state.publock);
    zsock_send(state.pubsock, "sb", topic, buf, length);
    pthread_mutex_unlock(&state.publock);
    bson_free(buf);
}

void maclink_logf(const char *fmt, ...) {
    char *str = NULL;
    va_list va;
    va_start(va, fmt);
    vasprintf(&str, fmt, va);
    va_end(va);

    bson_t obj = BSON_INITIALIZER;
    BSON_APPEND_UTF8(&obj, "msg", str);
    maclink_publish("log", &obj);
    free(str);
}

static void zbson_send(zsock_t *sock, bson_t *obj) {
    uint32_t length = 0;
    uint8_t *buf = bson_destroy_with_steal(obj, true, &length);
    zsock_send(sock, "b", buf, length);
    bson_free(buf);
}

static void send_reply(bson_t *doc) {
    zbson_send(state.cmdsock, doc);
}

static void send_err(const char *msg) {
    send_reply(BCON_NEW("success", BCON_BOOL(false), "error", BCON_UTF8(msg)));
}

static void send_success() {
    send_reply(BCON_NEW("success", BCON_BOOL(true)));
}

static int grammar_disable(maclink_grammar *grammar, char **errmsg) {
    int rc = 0;
    if ((rc =_DSXGrammar_Deactivate(grammar->handle, 0, grammar->main_rule))) {
        asprintf(errmsg, "error deactivating grammar: %d", rc);
        return rc;
    }
    grammar->enabled = false;
    if ((rc =_DSXGrammar_Unregister(grammar->handle, grammar->endkey))) {
        asprintf(errmsg, "error removing end cb: %d", rc);
        return rc;
    }
    if ((rc = _DSXGrammar_Unregister(grammar->handle, grammar->hypokey))) {
        asprintf(errmsg, "error removing hypothesis cb: %d", rc);
        return rc;
    }
    if ((rc = _DSXGrammar_Unregister(grammar->handle, grammar->beginkey))) {
        asprintf(errmsg, "error removing begin cb: %d", rc);
        return rc;
    }
    return 0;
}

static void grammar_free(maclink_grammar *g) {
    free((void *)g->main_rule);
    free((void *)g->name);
    free((void *)g->appname);
    memset(g, 0, sizeof(maclink_grammar));
    free(g);
}

static int grammar_unload(maclink_grammar *g) {
    char *disablemsg = NULL;
    int rc = 0;
    if (g->enabled) {
        if ((rc = grammar_disable(g, &disablemsg))) {
            maclink_logf("during unload: %s", disablemsg);
            free(disablemsg);
            return rc;
        }
    }
    rc = _DSXGrammar_Destroy(g->handle);

    pthread_mutex_lock(&state.keylock);
    tack_hdel(&state.grammars, g->name);
    tack_remove(&state.grammars, g);
    // see comment in g.load for more info on keys
    tack_set(&state.gkeys, g->key, NULL);

    reusekey *reuse = malloc(sizeof(reusekey));
    reuse->key = g->key;
    reuse->ts = bson_get_monotonic_time();
    reuse->serial = state.serial;
    tack_push(&state.gkfree, reuse);

    grammar_free(g);
    pthread_mutex_unlock(&state.keylock);
    return rc;
}

static void handle(const uint8_t *msg, uint32_t msglen) {
    char *errmsg = NULL;
    bool errfree = false;

    maclink_grammar *grammar = NULL;
    char *cmd = NULL, *name = NULL, *list = NULL, *main_rule = NULL;
    bool enabled = false, exclusive = false;
    int priority = 0;
    bool has_enabled = false, has_exclusive = false, has_priority = false;

    const uint8_t *items_buf = NULL, *data_buf = NULL, *phrase_buf = NULL;
    uint32_t items_len = 0, data_len = 0, phrase_len = 0;

    bson_t root;
    if (!bson_init_static(&root, msg, msglen)) {
        errmsg = "bson init error";
        goto end;
    }
    bson_iter_t iter;
    if (bson_iter_init(&iter, &root)) {
        while (bson_iter_next(&iter)) {
            const char *key = bson_iter_key(&iter);
            if (streq(key, "cmd") && BSON_ITER_HOLDS_UTF8(&iter)) {
                cmd = bson_iter_dup_utf8(&iter, NULL);
            } else if (streq(key, "name") && BSON_ITER_HOLDS_UTF8(&iter)) {
                name = bson_iter_dup_utf8(&iter, NULL);
            } else if (streq(key, "main_rule") && BSON_ITER_HOLDS_UTF8(&iter)) {
                main_rule = bson_iter_dup_utf8(&iter, NULL);
            } else if (streq(key, "list") && BSON_ITER_HOLDS_UTF8(&iter)) {
                list = bson_iter_dup_utf8(&iter, NULL);
            } else if (streq(key, "enabled") && BSON_ITER_HOLDS_BOOL(&iter)) {
                enabled = bson_iter_bool(&iter);
                has_enabled = true;
            } else if (streq(key, "exclusive") && BSON_ITER_HOLDS_BOOL(&iter)) {
                exclusive = bson_iter_bool(&iter);
                has_exclusive = true;
            } else if (streq(key, "priority") && BSON_ITER_HOLDS_INT32(&iter)) {
                priority = bson_iter_int32(&iter);
                has_priority = true;
            } else if (streq(key, "items") && BSON_ITER_HOLDS_ARRAY(&iter)) {
                bson_iter_array(&iter, &items_len, &items_buf);
            } else if (streq(key, "phrase") && BSON_ITER_HOLDS_ARRAY(&iter)) {
                bson_iter_array(&iter, &phrase_len, &phrase_buf);
            } else if (streq(key, "data") && BSON_ITER_HOLDS_BINARY(&iter)) {
                bson_iter_binary(&iter, NULL, &data_len, &data_buf);
            }
        }
    }
    if (name) grammar = tack_hget(&state.grammars, name);
    if (!cmd) {
        errmsg = "missing or broken cmd field";
        goto end;
    }
    if (streq(cmd, "g.update")) {
        if (!state.ready) goto not_ready;
        if (!grammar) goto no_grammar;

        if (has_enabled && enabled != grammar->enabled) {
            errfree = true;
            if (enabled) {
                int rc = 0;
                if ((rc = _DSXGrammar_Activate(grammar->handle, 0, false, grammar->main_rule))) {
                    asprintf(&errmsg, "error activating grammar: %d", rc);
                    goto end;
                }
                if ((rc = _DSXGrammar_RegisterEndPhraseCallback(grammar->handle, phrase_end, (void *)grammar->key, &grammar->endkey))) {
                    asprintf(&errmsg, "error registering end phrase callback: %d", rc);
                    goto end;
                }
                if ((rc = _DSXGrammar_RegisterPhraseHypothesisCallback(grammar->handle, phrase_hypothesis, (void *)grammar->key, &grammar->hypokey))) {
                    asprintf(&errmsg, "error registering phrase hypothesis callback: %d", rc);
                    goto end;
                }
                if ((rc = _DSXGrammar_RegisterBeginPhraseCallback(grammar->handle, phrase_begin, (void *)grammar->key, &grammar->beginkey))) {
                    asprintf(&errmsg, "error registering begin phrase callback: %d", rc);
                    goto end;
                }
            } else {
                if (grammar_disable(grammar, &errmsg)) {
                    errfree = true;
                    goto end;
                }
            }
            errfree = false;
            grammar->enabled = enabled;
        }
        if (has_exclusive && exclusive != grammar->exclusive) {
            int rc = _DSXGrammar_SetSpecialGrammar(grammar->handle, exclusive);
            if (rc) {
                asprintf(&errmsg, "error setting exclusive grammar: %d", rc);
                errfree = true;
                goto end;
            }
            grammar->exclusive = exclusive;
        }
        if (has_priority && priority != grammar->priority) {
            int rc = _DSXGrammar_SetPriority(grammar->handle, priority);
            if (rc) {
                asprintf(&errmsg, "error setting priority: %d", rc);
                errfree = true;
                goto end;
            }
            grammar->priority = priority;
        }
        send_success();
    } else if (streq(cmd, "g.listset")) {
        if (!state.ready) goto not_ready;
        if (!grammar) goto no_grammar;
        if (!list) {
            errmsg = "missing or broken list name";
            goto end;
        }
        if (!items_buf || !items_len) {
            errmsg = "missing or broken items array";
            goto end;
        }
        dsx_dataptr dp = {.data = NULL, .size = 0};
        if (!bson_iter_init_from_data(&iter, items_buf, items_len)) {
            errmsg = "list item iter failed";
            goto end;
        }
        // get size of the new list's data block
        while (bson_iter_next(&iter)) {
            if (!BSON_ITER_HOLDS_UTF8(&iter)) {
                errmsg = "list contains non-string value";
                goto end;
            }
            uint32_t length = 0;
            bson_iter_utf8(&iter, &length);
            dp.size += sizeof(dsx_id) + align4(length);
        }
        dp.data = calloc(1, dp.size);
        uint8_t *pos = (uint8_t *)dp.data;
        if (!bson_iter_init_from_data(&iter, items_buf, items_len)) {
            errmsg = "list item iter failed";
            goto end;
        }
        while (bson_iter_next(&iter)) {
            dsx_id *ent = (dsx_id *)pos;
            uint32_t length = 0;
            const char *word = bson_iter_utf8(&iter, &length);
            ent->size = sizeof(dsx_id) + align4(length);
            memcpy(ent->name, word, length);
            pos += ent->size;
        }
        if (_DSXGrammar_SetList(grammar->handle, list, &dp)) {
            errmsg = "error setting list";
            goto end;
        }
        send_success();
        free(dp.data);
    } else if (streq(cmd, "g.unload")) {
        if (!state.ready) goto not_ready;
        if (!grammar) goto no_grammar;
        int rc = grammar_unload(grammar);
        if (rc) {
            asprintf(&errmsg, "error unloading grammar: %d", rc);
            errfree = true;
            goto end;
        }
        send_success();
    } else if (streq(cmd, "g.load")) {
        if (!state.ready) goto not_ready;
        if (!data_buf || !data_len) {
            errmsg = "missing or broken data field";
            goto end;
        }
        if (!main_rule) {
            errmsg = "missing main_rule";
            goto end;
        }
        if (grammar) {
            maclink_logf("warning: reloading \"%s\"", name);
            int rc = grammar_unload(grammar);
            if (rc) {
                asprintf(&errmsg, "error unloading grammar: %d", rc);
                errfree = true;
                goto end;
            }
        }
        // TODO: BOUNDS CHECK GRAMMAR HERE
        grammar = calloc(1, sizeof(maclink_grammar));
        grammar->name = strdup(name);
        grammar->main_rule = strdup(main_rule);
        dsx_dataptr dp = {.data = (void *)data_buf, .size = data_len};

        int ret = _DSXEngine_LoadGrammar(_engine, 1 /*cfg*/, &dp, &grammar->handle);
        if (ret > 0) {
            asprintf(&errmsg, "error loading grammar: %d", ret);
            errfree = true;
            grammar_free(grammar);
            goto end;
        }
        tack_push(&state.grammars, grammar);
        tack_hset(&state.grammars, grammar->name, grammar);
        // keys are used to associate grammar objects with callbacks
        // in a way that allows us to free the grammar objects without crashing if a callback was in flight
        // once freed, keys can be reused after 30 seconds, or if the global serial has increased by at least 3
        // (to prevent both callback confusion and key explosion)
        bool reused = false;
        reusekey *reuse;
        int64_t now = bson_get_monotonic_time();
        pthread_mutex_lock(&state.keylock);
        tack_foreach(&state.gkfree, reuse) {
            if (now - reuse->ts > 30 * SEC || reuse->serial + 3 <= state.serial) {
                reused = true;
                grammar->key = reuse->key;
                free(reuse);
                tack_del(&state.gkfree, i);
                tack_set(&state.gkeys, grammar->key, grammar);
                break;
            }
        }
        if (!reused) {
            grammar->key = tack_len(&state.gkeys);
            tack_push(&state.gkeys, grammar);
        }
        pthread_mutex_unlock(&state.keylock);
        // printf("%d\n", _DSXGrammar_SetApplicationName(grammar->handle, grammar->name));
        send_success();

    // diagnostic commands
    } else if (streq(cmd, "status")) {
        bson_t grammars, child;
        char keystr[16];
        const char *key;
        bson_t *doc = BCON_NEW(
            "success", BCON_BOOL(true),
            "ready", BCON_BOOL(state.ready),
            "runtime", BCON_INT64(bson_get_monotonic_time() - state.start_ts));

        BSON_APPEND_ARRAY_BEGIN(doc, "grammars", &grammars);
        tack_foreach(&state.grammars, grammar) {
            bson_uint32_to_string(i, &key, keystr, sizeof(keystr));
            BSON_APPEND_DOCUMENT_BEGIN(&grammars, key, &child);
            BSON_APPEND_UTF8(&child, "name", grammar->name);
            BSON_APPEND_BOOL(&child, "enabled", grammar->enabled);
            BSON_APPEND_INT32(&child, "priority", grammar->priority);
            BSON_APPEND_BOOL(&child, "exclusive", grammar->exclusive);
            bson_append_document_end(&grammars, &child);
        }
        bson_append_array_end(doc, &grammars);

        send_reply(doc);
    } else if (streq(cmd, "mimic")) {
        if (!state.ready) goto not_ready;
        if (!phrase_buf || !phrase_len) {
            errmsg = "missing or broken phrase field";
            goto end;
        }

        dsx_dataptr dp = {.data = NULL, .size = 0};
        if (!bson_iter_init_from_data(&iter, phrase_buf, phrase_len)) {
            errmsg = "mimic phrase iter failed";
            goto end;
        }
        // get size of all strings in phrase
        while (bson_iter_next(&iter)) {
            if (!BSON_ITER_HOLDS_UTF8(&iter)) {
                errmsg = "phrase contains non-string value";
                goto end;
            }
            uint32_t length = 0;
            bson_iter_utf8(&iter, &length);
            dp.size += length + 1;
        }
        dp.data = calloc(1, dp.size);
        uint8_t *pos = (uint8_t *)dp.data;
        if (!bson_iter_init_from_data(&iter, phrase_buf, phrase_len)) {
            errmsg = "mimic phrase iter failed";
            goto end;
        }
        int count = 0;
        while (bson_iter_next(&iter)) {
            uint32_t length = 0;
            const char *word = bson_iter_utf8(&iter, &length);
            memcpy(pos, word, length);
            pos += length + 1;
            count++;
        }
        int rc = _DSXEngine_Mimic(_engine, 0, count, &dp, 0, 2);
        if (rc) {
            asprintf(&errmsg, "error during mimic: %d", rc);
            errfree = true;
            free(dp.data);
            goto end;
        }
        free(dp.data);
        // TODO: if dragon fails the callback, draconity will hang forever
        bool success = (zsock_wait(state.mimic_wait) == 0);
        if (!success) {
            errmsg = "mimic failed";
            goto end;
        }
        send_success();
    } else {
        errmsg = "unsupported command";
        goto end;
    }
    bson_t *pub;
end:
    free(cmd);
    free(name);
    free(main_rule);
    free(list);

    pub = bson_new();
    BSON_APPEND_BOOL(pub, "success", errmsg == NULL);
    if (errmsg != NULL) {
        send_err(errmsg);
        BSON_APPEND_UTF8(pub, "error", errmsg);
        if (errfree) free(errmsg);
    }
    // reinit to reset + appease ASAN
    if (!bson_init_static(&root, msg, msglen)) {
        errmsg = "bson init error";
        goto end;
    }
    BSON_APPEND_DOCUMENT(pub, "cmd", &root);
    maclink_publish("cmd", pub);
    return;

no_grammar:
    errmsg = "grammar not found";
    goto end;

not_ready:
    errmsg = "engine not ready";
    goto end;
}

static void *maclink_thread(void *user) {
    printf("[-] cmd thread launched\n");
    while (1) {
        uint8_t *data = NULL;
        size_t size = 0;
        if (zsock_recv(state.cmdsock, "b", &data, &size) == 0) {
            handle(data, size);
            free(data);
        }
    }
}

void maclink_init() {
    state.mimic_wait = zsock_new_pair("@inproc://mimic_sync");
    state.mimic_signal = zsock_new_pair(">inproc://mimic_sync");

    pthread_mutex_init(&state.keylock, NULL);
    pthread_mutex_init(&state.publock, NULL);

    state.pubsock = zsock_new_pub("ipc:///tmp/ml_pub");
    state.cmdsock = zsock_new_rep("ipc:///tmp/ml_cmd");
    if (state.pubsock == NULL || state.cmdsock == NULL) {
        printf("zmq init failed\n");
        exit(1);
    }
#ifdef NODRAGON
    maclink_thread(NULL);
    exit(0);
#endif

    int rc = pthread_create(&state.tid, NULL, maclink_thread, NULL);
    if (rc) {
        printf("thread creation failed\n");
        exit(1);
    }
    state.start_ts = bson_get_monotonic_time();
}

static char *micstates[] = {
    "disabled",
    "off",
    "on",
    "sleeping",
    "pause",
    "resume",
};

void maclink_attrib_changed(int key, dsx_attrib *attrib) {
    char *attr = attrib->name;
    if (streq(attr, "MICON")) {
        maclink_publish("status", BCON_NEW("cmd", BCON_UTF8("mic"), "status", BCON_UTF8("on")));
    } else if (streq(attr, "MICSTATE")) {
        // micstate crashes on invalid free (some kind of mem corrupt)
        return;

        int micstate = 0;
        _DSXEngine_GetMicState(_engine, &micstate);
        char *name;
        if (micstate >= 0 && micstate <= 5) {
            name = micstates[micstate];
        } else {
            name = "invalid";
        }
        state.micstate = name;
        maclink_publish("status", BCON_NEW("cmd", BCON_UTF8("mic"), "status", BCON_UTF8(name)));
    } else if (streq(attr, "SPEAKERCHANGED")) {
        // this is slow
        // void *speaker = _DSXEngine_GetCurrentSpeaker(_engine);
        if (!state.ready) {
            printf("[+] maclink ready\n");
            maclink_publish("status", BCON_NEW("cmd", BCON_UTF8("ready")));
            state.ready = true;
        }
        // state.speaker = speaker;
    } else if (streq(attr, "TOPICCHANGED")) {
        // ?
    }
}

void maclink_mimic_done(int key, dsx_mimic *mimic) {
    zsock_signal(state.mimic_signal, (mimic->flags != 0));
}

void maclink_paused(int key, dsx_paused *paused) {
    _DSXEngine_Resume(_engine, paused->token);
}

int maclink_phrase_begin(int key, void *data) {
    // TODO: atomics? portability?
    pthread_mutex_lock(&state.keylock);
    state.serial++;
    pthread_mutex_unlock(&state.keylock);
    return 0;
}

int maclink_phrase_end(int key, void *data) {
    return 0;
}

__attribute__((destructor))
static void maclink_shutdown() {
    pthread_exit(NULL);
}
