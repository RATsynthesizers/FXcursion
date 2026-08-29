// Mixer.c

#include "Mixer.h"

static void mixer_init(mixer_t *self) {
    connectionBitmap = 0b100010001; // default - 1 to 1, 2 to 2, 3 to 3
    for (int i = 0; i < CHANNELS_NUM / 2; i++) {
        self->prevModule[i] = NULL;
        self->processingModule[i] = new Amp();
        self->nextModule[i] = NULL;
        self->volumeIn[i] = 1;
        self->volumeOut[i] = 1;
    }
}

static void plug_next(mixer_t *self, module_t *nextModule, u8 chnum) {
    self->nextModule[chnum] = nextModule;
    self->processingModule[chnum]->vtable->plug_next(nextModule);
}

static void plug_prev(mixer_t *self, module_t *prevModule, u8 chnum) {
    self->prevModule[chnum] = prevModule;
}

static void unplug_next(mixer_t *self, u8 chnum) {
    self->nextModule[chnum] = NULL;
}

static void unplug_prev(mixer_t *self, u8 chnum) {
    self->prevModule[chnum] = NULL;
}

static void mixer_process(mixer_t *self) {
    for (u8 i = 0; i < CHANNELS_NUM / 2; i++) {
        if (!(self->nextModule[i]) {
            continue;
        }           
        float countNextConnections = 0;
        float output[STEREO];
        for (int j = 0; j < STEREO; j++) {
            output[j] = 0;
        }
        for (u8 j = 0; j < CHANNELS_NUM / 2; j++) {
            u8 isConnected = (self->connectionBitmap)
                    >> (j * (CHANNELS_NUM / 2) + i) & 1;
            if (isConnected && self->prevModule[j]) {
                for (u8 lr = 0; lr < STEREO; lr++) {
                    output[lr] += self->prevModule[j]->output[lr];
                }
            }
            countNextConnections += isConnected;
        }          
        if (!countNextConnections) {
            continue;
        }
        for (int j = 0; j < STEREO; j++) {
            self->processingModule[i]->output[j] = output[j]
                    / countNextConnections;
        }
    }
}
    

