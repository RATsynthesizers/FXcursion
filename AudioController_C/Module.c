/// Module.c

#include "Module.h"

static u32 allParamNum = 0;
static u32 allModulesNum = 0; 

static void module_init(module_t *const self, u32 paramNum, u16 module_id) {
     allModulesNum++;
     self->paramNum = paramNum;             // num of parameters in module
     allParamNum += paramNum;
     self->prevModule = NULL;               // pointer for module params to be stored to
     self->nextModule = NULL;    

     self->outputVolume = 1;
     for (int i = 0; i < STEREO; i++)                           // let output be zeroed
         self->output[i] = 0;

    self->id = module_id;
}

static void plug_next  (module_t *self, module_t *nextModule) {
     self->nextModule = nextModule;
     nextModule->vtable->plugPrev(self);
}
static void plug_prev  (module_t *self, module_t *prevModule) {
     self->prevModule = prevModule;
}
static void unplug_next(module_t *self) {
     nextModule->vtable->unplugPrev(nextModule);
     self->nextModule = NULL;
}
static void unplug_prev(module_t *self) {
     self->prevModule = NULL;
}
