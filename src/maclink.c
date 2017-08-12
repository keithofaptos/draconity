#include <bson.h>
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "server.h"
#include "maclink.h"

static void *server_so = NULL;
drg_engine *_engine = NULL;

#include "api.h" // this defines the function pointers

__attribute__((constructor))
static void cons() {
    printf("[+] maclink init\n");
    server_so = dlopen("server.so", RTLD_LOCAL | RTLD_LAZY);
    dprintf("%46s %p\n", "server.so", server_so);
    #define load(x) do { _##x = dlsym(server_so, #x); dprintf("%46s %p\n", #x, _##x); } while (0)
    load(DSXEngine_Create);
    load(DSXEngine_New);

    load(DSXEngine_GetCurrentSpeaker);
    load(DSXEngine_GetMicState);
    load(DSXEngine_LoadGrammar);
    load(DSXEngine_Mimic);
    load(DSXEngine_Pause);
    load(DSXEngine_RegisterAttribChangedCallback);
    load(DSXEngine_RegisterMimicDoneCallback);
    load(DSXEngine_RegisterPausedCallback);
    load(DSXEngine_Resume);
    load(DSXEngine_ResumeRecognition);
    load(DSXEngine_SetBeginPhraseCallback);
    load(DSXEngine_SetEndPhraseCallback);

    load(DSXFileSystem_PreferenceGetValue);
    load(DSXFileSystem_PreferenceSetValue);

    load(DSXGrammar_Activate);
    load(DSXGrammar_Deactivate);
    load(DSXGrammar_Destroy);
    load(DSXGrammar_GetList);
    load(DSXGrammar_RegisterBeginPhraseCallback);
    load(DSXGrammar_RegisterEndPhraseCallback);
    load(DSXGrammar_RegisterPhraseHypothesisCallback);
    load(DSXGrammar_SetApplicationName);
    load(DSXGrammar_SetList);
    load(DSXGrammar_SetPriority);
    load(DSXGrammar_SetSpecialGrammar);
    load(DSXGrammar_Unregister);

    load(DSXResult_BestPathWord);
    load(DSXResult_GetWordNode);
    load(DSXResult_Destroy);

    maclink_init();
    printf("[+] maclink attached\n");
}

static void engine_setup(drg_engine *engine) {
    static unsigned int cb_key = 0;
    int ret = _DSXEngine_RegisterAttribChangedCallback(engine, maclink_attrib_changed, NULL, &cb_key);
    if (ret) maclink_logf("error adding attribute callback: %d", ret);

    ret = _DSXEngine_RegisterMimicDoneCallback(engine, maclink_mimic_done, NULL, &cb_key);
    if (ret) maclink_logf("error adding mimic done callback: %d", ret);

    ret = _DSXEngine_RegisterPausedCallback(engine, maclink_paused, "paused?", NULL, &cb_key);
    if (ret) maclink_logf("error adding paused callback: %d", ret);

    ret = _DSXEngine_SetBeginPhraseCallback(engine, maclink_phrase_begin, NULL, &cb_key);
    if (ret) maclink_logf("error setting phrase begin callback: %d", ret);

    ret = _DSXEngine_SetEndPhraseCallback(engine, maclink_phrase_end, NULL, &cb_key);
    if (ret) maclink_logf("error setting phrase end callback: %d", ret);

    maclink_publish("status", BCON_NEW("cmd", BCON_UTF8("start")));
}

void *DSXEngine_New() {
    _engine = _DSXEngine_New();
    maclink_logf("DSXEngine_New() = %p", _engine);
    engine_setup(_engine);
    return _engine;
}

int DSXEngine_Create(char *s, uint64_t val, drg_engine **engine) {
    int ret = _DSXEngine_Create(s, val, engine);
    _engine = *engine;
    maclink_logf("DSXEngine_Create(%s, %llu, &%p) = %d", s, val, engine, ret);
    engine_setup(_engine);
    return ret;
}

int DSXEngine_LoadGrammar(drg_engine *engine, int format, void *unk, drg_grammar **grammar) {
    return _DSXEngine_LoadGrammar(engine, format, unk, grammar);
}

int DSXEngine_SetBeginPhraseCallback() {
    maclink_logf("warning: called stubbed SetBeginPhraseCallback()\n");
    return 0;
}

int DSXEngine_SetEndPhraseCallback() {
    maclink_logf("warning: called stubbed SetBeginPhraseCallback()\n");
    return 0;
}

int DSXFileSystem_PreferenceSetValue(drg_filesystem *fs, char *a, char *b, char *c, char *d) {
    // printf("DSXFileSystem_PreferenceSetValue(%p, %s, %s, %s, %s);\n", fs, a, b, c, d);
    return DSXFileSystem_PreferenceSetValue(fs, a, b, c, d);
}

/*
// TODO: track which dragon grammars are active, so "dragon" pseudogrammar can activate them
int DSXGrammar_Activate(drg_grammar *grammar, uint64_t unk1, bool unk2, char *name) {
    // printf("hooked DSXGrammar_Activate(%p, %lld, %d, %s)\n", grammar, unk1, unk2, name);
    return 0; // return _DSXGrammar_Activate(grammar, unk1, unk2, name);
}

int DSXGrammar_RegisterEndPhraseCallback(drg_grammar *grammar, void *cb, void *user, unsigned int *key) {
    *key = 0;
    return 0;
    // return _DSXGrammar_RegisterEndPhraseCallback(grammar, cb, user, key);
}

int DSXGrammar_RegisterBeginPhraseCallback(drg_grammar *grammar, void *cb, void *user, unsigned int *key) {
    *key = 0;
    return 0;
    // return _DSXGrammar_RegisterBeginPhraseCallback(grammar, cb, user, key);
}

int DSXGrammar_RegisterPhraseHypothesisCallback(drg_grammar *grammar, void *cb, void *user, unsigned int *key) {
    *key = 0;
    return 0;
    // return _DSXGrammar_RegisterPhraseHypothesisCallback(grammar, cb, user, key);
}
*/
