#ifndef DRACONITY_SERVER_H
#define DRACONITY_SERVER_H

#include <bson.h>
#include "maclink.h"

extern void maclink_init();
extern void maclink_publish(const char *topic, bson_t *msg);
extern void maclink_logf(const char *fmt, ...);

// callbacks
extern void maclink_attrib_changed(int key, dsx_attrib *attrib);
extern void maclink_mimic_done(int key, dsx_mimic *mimic);
extern void maclink_paused(int key, dsx_paused *paused);

extern int maclink_phrase_begin(int key, void *data);
extern int maclink_phrase_end(int key, void *data);

#endif
