// AudioSys.c

#include "AudioSys.h"

IN_DTCMRAM audio_sys_t audioSystem; // global audio instance

// USER: manually add modules here
static void include_modules(audio_sys_t *self) {
    // create all modules  ++
    mixer_t      mixer                    ;
    amp_t        amp_io     [CHANNELS_NUM];
    chorus_t     chorus     [CHANNELS_NUM];
    compressor_t compressor [CHANNELS_NUM];
    delay_t      delay      [CHANNELS_NUM];
    distortion_t distortion [CHANNELS_NUM];
    flanger_t    flanger    [CHANNELS_NUM];
    overdrive_t  overdrive  [CHANNELS_NUM];
    phaser_t     phaser     [CHANNELS_NUM];
    reverb_t     reverb     [CHANNELS_NUM];
    tremolo_t    tremolo    [CHANNELS_NUM];
    vibrato_t    vibrato    [CHANNELS_NUM];

    // Init all modules   ++
    mixer                        ->vtable-> moduleInit (&mixer        );
    for (int i = 0; i < CHANNELS_NUM; i++) {
        amp_io    [CHANNELS_NUM] ->vtable-> moduleInit (amp_io     + i);
        chorus    [CHANNELS_NUM] ->vtable-> moduleInit (chorus     + i);
        compressor[CHANNELS_NUM] ->vtable-> moduleInit (compressor + i);
        delay     [CHANNELS_NUM] ->vtable-> moduleInit (delay      + i);
        distortion[CHANNELS_NUM] ->vtable-> moduleInit (distortion + i);
        flanger   [CHANNELS_NUM] ->vtable-> moduleInit (flanger    + i);
        overdrive [CHANNELS_NUM] ->vtable-> moduleInit (overdrive  + i);
        phaser    [CHANNELS_NUM] ->vtable-> moduleInit (phaser     + i);
        reverb    [CHANNELS_NUM] ->vtable-> moduleInit (reverb     + i);
        tremolo   [CHANNELS_NUM] ->vtable-> moduleInit (tremolo    + i);
        vibrato   [CHANNELS_NUM] ->vtable-> moduleInit (vibrato    + i);
    }

    for (int i = 0; i < CHANNELS_NUM; i++) {
        if (i < CHANNELS_NUM / 2) {  // TODO why?
            // assign system in and out modules
            self->inputModule[i] = amp_io[i];
            self->outputModule[i] = amp_io[i];
            // default mixer plug in
            mixer->vtable->plug_prev(self->inputModule[i], i);
            mixer->vtable->plug_next(self->outputModule[i], i);
        }
        // assign fx to modules array on every channel  ++
        modules[i][M_CHORUS]     = chorus     + i;
        modules[i][M_COMPRESSOR] = compressor + i;
        modules[i][M_DELAY]      = delay      + i;
        modules[i][M_DISTORTION] = distortion + i;
        modules[i][M_FLANGER]    = flanger    + i;
        modules[i][M_OVERDRIVE]  = overdrive  + i;
        modules[i][M_PHASER]     = phaser     + i;
        modules[i][M_REVERB]     = reverb     + i;
        modules[i][M_TREMOLO]    = tremolo    + i;
        modules[i][M_VIBRATO]    = vibrato    + i;
    }
}

// USER: call this to init the system
static void audio_sys_init(audio_sys_t *self) {
    self->vtable = _AUDIOSYS_VTABLE();
    // SAI global driver adapters
    self->adapter1 = &sai1adapter;
    self->adapter2 = &sai2adapter;
    self->vtable->include_modules(self);
}

static void get_audio_samples_in_input(audio_sys_t *self, u8 chnum, SAIadapter_TypeDef *adapter) {
    for (u8 lr = 0; lr < STEREO; lr++) {     // convert input, left & right
    self->inputModule[chnum]->output[lr] = ((float) saiAdapter_get_next_sample(adapter)) / MY_INT16_MAX;
    }
}

static void set_audio_samples_to_output(audio_sys_t *self, u8 chnum, SAIadapter_TypeDef *adapter) {
    for (u8 lr = 0; lr < STEREO; lr++) {        // convert output, left & right
        int16_t tmp = 0;
        if (self->outputModule[chnum]->prevModule != NULL)
            tmp = (int16_t) (self->outputModule[chnum]->prevModule->output[lr]
                    * MY_INT16_MAX);
        saiAdapter_set_next_sample(adapter, tmp);
    }
} 

static void update_GUI(audio_sys_t *self, GUIadapter_TypeDef *adapter) {
    // TODO:
}

// USER: call this at the sample time to process all sound and aply param updates from GUI when they arrive
static void audio_sys_update(audio_sys_t *self) {
    // update parameters from GUI if needed
    if (guiadapter.updateFlag) {
        self->vtable->update_GUI(self, &guiadapter);
    }

    // get audio samples if needed
    if (saiAdapterCheckUpdate(adapter1) || saiAdapterCheckUpdate(adapter2)) {
        for (uint8_t i = 0; i < SAI_HALF_BUF; i += STEREO) { // until half of RxBuf & TxBuf is not fully processed
            get_audio_samples_in_input(self, 0, adapter1); // load next sample pair in input module
            get_audio_samples_in_input(self, 1, adapter2); // load next sample pair in input module

            for (int j = 0; j < CHANNELS_NUM / 2; j++) {
                module_t *module = self->inputModule[j]->nextModule;
                while (module) {
                    module->vtable->process();
                    module = module->nextModule;
                }
            }

            mixer->vtable->process();

            for (int j = 0; j < CHANNELS_NUM / 2; j++) {
                module_t *module = mixer->nextModule[j];
                while (module) {
                    module->vtable->process();
                    module = module->nextModule;
                }
            }

            set_audio_samples_to_output(self, 0, adapter1);
            set_audio_samples_to_output(self, 1, adapter2);
        }
    }
}



