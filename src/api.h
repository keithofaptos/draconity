#ifndef DLAPI
#define DLAPI
#endif

// dragon 5
DLAPI drg_engine *(*_DSXEngine_New)();
// dragon 6
DLAPI int (*_DSXEngine_Create)(char *s, uint64_t val, drg_engine **engine);

DLAPI int (*_DSXEngine_LoadGrammar)(drg_engine *engine, int type, dsx_dataptr *data, drg_grammar **grammar_out);
DLAPI void *(*_DSXEngine_GetCurrentSpeaker)(drg_engine *engine);
DLAPI int (*_DSXEngine_GetMicState)(drg_engine *engine, int *state);
DLAPI int (*_DSXEngine_Mimic)(drg_engine *engine, int unk1, unsigned int count, dsx_dataptr *data, unsigned int unk2, int type);
DLAPI int (*_DSXEngine_RegisterAttribChangedCallback)(drg_engine *engine, void *cb, void *user, unsigned int *key);
DLAPI int (*_DSXEngine_RegisterMimicDoneCallback)(drg_engine *engine, void *cb, void *user, unsigned int *key);
DLAPI int (*_DSXEngine_RegisterPausedCallback)(drg_engine *engine, void *cb, void *user, char *name, unsigned int *key);

DLAPI int (*_DSXEngine_SetBeginPhraseCallback)(drg_engine *engine, void *cb, void *user, unsigned int *key);
DLAPI int (*_DSXEngine_SetEndPhraseCallback)(drg_engine *engine, void *cb, void *user, unsigned int *key);

DLAPI int (*_DSXEngine_Pause)(drg_engine *engine);
DLAPI int (*_DSXEngine_Resume)(drg_engine *engine, uint64_t token);
DLAPI int (*_DSXEngine_ResumeRecognition)(drg_engine *engine);

DLAPI int (*_DSXFileSystem_PreferenceSetValue)(drg_filesystem *fs, char *a, char *b, char *c, char *d);
DLAPI int (*_DSXFileSystem_PreferenceGetValue)(drg_filesystem *fs, char *a, char *b, char *c, char *d);

DLAPI int (*_DSXGrammar_Activate)(drg_grammar *grammar, uint64_t unk1, bool unk2, const char *main_rule);
DLAPI int (*_DSXGrammar_Deactivate)(drg_grammar *grammar, uint64_t unk1, const char *main_rule);
DLAPI int (*_DSXGrammar_Destroy)(drg_grammar *);
DLAPI int (*_DSXGrammar_GetList)(drg_grammar *grammar, const char *name, dsx_dataptr *data);
DLAPI int (*_DSXGrammar_RegisterBeginPhraseCallback)(drg_grammar *grammar, void *cb, void *user, unsigned int *key);
DLAPI int (*_DSXGrammar_RegisterEndPhraseCallback)(drg_grammar *grammar, void *cb, void *user, unsigned int *key);
DLAPI int (*_DSXGrammar_RegisterPhraseHypothesisCallback)(drg_grammar *grammar, void *cb, void *user, unsigned int *key);
DLAPI int (*_DSXGrammar_SetApplicationName)(drg_grammar *grammar, const char *name);
DLAPI int (*_DSXGrammar_SetList)(drg_grammar *grammar, const char *name, dsx_dataptr *data);
DLAPI int (*_DSXGrammar_SetPriority)(drg_grammar *grammar, int priority);
DLAPI int (*_DSXGrammar_SetSpecialGrammar)(drg_grammar *grammar, int special);
DLAPI int (*_DSXGrammar_Unregister)(drg_grammar *grammar, unsigned int key);

DLAPI int (*_DSXResult_BestPathWord)(dsx_result *result, int choice, uint32_t *path, size_t pathSize, size_t *needed);
DLAPI int (*_DSXResult_GetWordNode)(dsx_result *result, uint32_t path, void *node, uint32_t *num, char **name);
DLAPI int (*_DSXResult_Destroy)(dsx_result *result);

#undef DLAPI
