// Amp.c

#include "Amp.h"

static void process_amp(module_t *self){
    for (u8 lr = 0; lr < STEREO; lr++) {
        self->output[lr] = self->output[lr] * self->outputVolume;
    }
}

static void template_init(module_t *self) {
    self->vtable = _MODULE_VTABLE(process_amp);
    self->moduleInit((module_t *) self, AMP_PARAMETERS, AMP_NAME);

    // init parameters
    self->param1->val = 0;
    self->param1->id  = p_id_t::P_GAIN;  // of p_id_t
    self->param2->val = 1;
    self->param1->id  = p_id_t::P_PAN;    

    //other mixer shit init 
}