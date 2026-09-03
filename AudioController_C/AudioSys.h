// AudioSys.h

#ifndef AUDIOSYS_H_
#define AUDIOSYS_H_

#include "Handy.h"

#include "../../Drivers/HW/GUIadapter.h"
#include "../../Drivers/HW/MySAIadapter.h"
#include "libModules/Module.h"
#include "libModules/Parameter.h"
#include "libModules/Mixer.h"

#include "Effects/amp/Amp.h"
#include "Effects/chorus/Chorus.h"
#include "Effects/compressor/Compressor.h"
#include "Effects/delay/Delay.h"
#include "Effects/distortion/Distortion.h"
#include "Effects/flanger/Flanger.h"
#include "Effects/overdrive/Overdrive.h"
#include "Effects/phaser/Phaser.h"
#include "Effects/reverb/Reverb.h"
#include "Effects/tremolo/Tremolo.h"
#include "Effects/vibrato/Vibrato.h"

typedef struct audio_sys_s audio_sys_t;

typedef struct audio_sys_vtable {
void (*include_modules)             (audio_sys_t *self);
void (*get_audio_samples_in_input)  (audio_sys_t *self, u8 chnum, SAIadapter_TypeDef *adapter);
void (*set_audio_samples_to_output) (audio_sys_t *self, u8 chnum, SAIadapter_TypeDef *adapter);
void (*update_GUI)                  (audio_sys_t *self, GUIadapter_TypeDef *adapter);
void (*update)                      (audio_sys_t *self);
} audio_sys_vtable_t;

// macro for assigning shared audio_sys_t
#define _AUDIOSYS_VTABLE()                                          \
{                                                                   \
        .include_modules             = include_modules,             \
        .moduleInit                  = module_init,                 \
        .get_audio_samples_in_input  = get_audio_samples_in_input,  \
        .set_audio_samples_to_output = set_audio_samples_to_output, \
        .update_GUI                  = update_GUI,                  \
        .update                      = update                       \
}

typedef struct audio_sys_s {
    const audio_sys_vtable_t *vtable;
    // audio sys data - io and modules
// TODO parameters list for scatter-gather MDMA?
    module_t *inputModule[CHANNELS_NUM/2];
    module_t *outputModule[CHANNELS_NUM/2];
    module_t *modules[CHANNELS_NUM][EXISTING_MODULES];
    mixer_t  *mixer;

    SAIadapter_TypeDef *adapter1;
    SAIadapter_TypeDef *adapter2;
} audio_sys_t;

extern audio_sys_t audioSystem; // global audio instance


#endif /* AUDIOSYS_H_ */