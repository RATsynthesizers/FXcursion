/// Module.c

#include "Module.h"

static u32 allParamNum = 0; // used to count how many Parameters were used by all Modules in total; 0 by default

void module_init(module_t *const self, parameter_t p, u32 paramNum) {
     

     self->paramNum = paramNum;             // num of parameters in module
     self->prevModule = NULL;               // pointer for module params to be stored to
     self->nextModule = NULL;

     if (allParamNum + paramNum > MAX_SYNTH_PARAMS) {        // if MAX_SYNTH_PARAMS is not enough to store
          while (1)
               ;                                                // error, increase MAX_SYNTH_PARAMS
     } else {
          self->p = p + allParamNum;                            // find where to store Module params in allocated array
          allParamNum += paramNum;                           // increase alloc array offset (to store params of the next Module if created)
          for (u32 i = 0; i < paramNum; i++)
               self->p[i].index = i;                               // init parameter indexes
     }     

     self->outputVolume = 1;
     for (int i = 0; i < STEREO; i++)                           // let output be zeroed
         self->output[i] = 0;


}

void plug_next  (module_t *self, module_t *nextModule) {
     self->nextModule = nextModule;
     nextModule->vtable->plugPrev(self);
}
void plug_prev  (module_t *self, module_t *prevModule) {
     self->prevModule = prevModule;
}
void unplug_next(module_t *self) {
     nextModule->vtable->unplugPrev(nextModule);
     self->nextModule = NULL;
}
void unplug_prev(module_t *self) {}









//--mixer.c-------------------------------------------
void process_mixer(){}


void mixer_init(module_t *const self, parameter_t p, u32 paramNum) {
     self->vtable = _MODULE_VTABLE(process_mixer);
     self->moduleInit((module_t *) self, p, paramNum);

     //other mixer shit init 

}