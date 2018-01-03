#ifndef DRACONITY_SERVER_H
#define DRACONITY_SERVER_H

#include <bson.h>
#include "draconity.h"

extern void draconity_init();
extern void draconity_publish(const char *topic, bson_t *msg);
extern void draconity_logf(const char *fmt, ...);

// callbacks
extern void draconity_attrib_changed(int key, dsx_attrib *attrib);
extern void draconity_mimic_done(int key, dsx_mimic *mimic);
extern void draconity_paused(int key, dsx_paused *paused);

extern int draconity_phrase_begin(int key, void *data);
extern int draconity_phrase_end(int key, void *data);

#endif
