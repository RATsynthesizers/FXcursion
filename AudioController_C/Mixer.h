// Mixer.h - this block is special (and does not inherit from Module) 
// as it has more pointers to the input and output modules and another module
// to process them. Kinda like a tiny modular subsystem of it's own.

#ifndef LIBMODULES_MIXER_H_
#define LIBMODULES_MIXER_H_

#include <stddef.h>
#include "Handy.h"
#include "Module.h"
#include "../Effects/amp/Amp.h"

typedef struct mixer_s mixer_t;

typedef struct mixer_vtable {
void (*include_modules)             (audio_sys_t *self);
void (*get_audio_samples_in_input)  (audio_sys_t *self, u8 chnum, SAIadapter_TypeDef *adapter);
void (*set_audio_samples_to_output) (audio_sys_t *self, u8 chnum, SAIadapter_TypeDef *adapter);
void (*update_GUI)                  (audio_sys_t *self, GUIadapter_TypeDef *adapter);
void (*update)                      (audio_sys_t *self);
} mixer_vtable_t;

// macro for assigning shared audio_sys_t
#define _MIXER_VTABLE()                   \
{                                         \
    .process    = mixer_process,          \
    .moduleInit = mixer_init,             \
    .plugNext   = plug_next,              \
    .plugPrev   = plug_prev,              \
    .unplugNext = unplug_next,            \
    .unplugPrev = unplug_prev             \
}

typedef struct mixer_s {
    module_t *prevModule[CHANNELS_NUM / 2];
    module_t *processingModule[CHANNELS_NUM / 2];
    module_t *nextModule[CHANNELS_NUM / 2];

    float volumeIn[CHANNELS_NUM / 2];
    float volumeOut[CHANNELS_NUM / 2];

    u16 connectionBitmap;
} mixer_t;

#endif /* LIBMODULES_MIXER_H_ */
