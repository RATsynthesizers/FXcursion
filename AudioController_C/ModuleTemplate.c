// ModuleTemplate.h

#include "ModuleTemplate.h"

static u32 template_local_func(u32 a) {
    //...
}

static void process_template(module_t *self){
    u32 a = 0;
    b = template_local_func(a);

    // ...

    for (u8 lr = 0; lr < STEREO; lr++) {
        float p1 = self->param1->val;
        self->output[lr] = self->output[lr] * self->outputVolume * p1;
    }
}

static void template_init(module_t *self) {
    self->vtable = _MODULE_VTABLE(process_template);
    self->moduleInit((module_t *) self, MODULETEMPLATE_PARAMETERS, MODULETEMPLATE_NAME);

    // init parameters
    self->param1->val = 0;
    self->param1->id  = p_id_t::P_GAIN;  // of p_id_t
    self->param2->val = 1;
    self->param1->id  = p_id_t::P_PAN;    

    //other mixer shit init 
}