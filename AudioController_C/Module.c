/// .c

void module_init(module_t *const m, parameter_t p, u32 paramNum) {
     m->paramNum = paramNum;             // num of parameters in module
     m->prevModule = NULL;               // pointer for module params to be stored to
     m->nextModule = NULL;

     if (m->allParamNum + paramNum > MAX_SYNTH_PARAMS) {        // if MAX_SYNTH_PARAMS is not enough to store
          while (1)
               ;                                                // error, increase MAX_SYNTH_PARAMS
     } else {
          m->p = p + m->allParamNum;                            // find where to store Module params in allocated array
          m->allParamNum += paramNum;                           // increase alloc array offset (to store params of the next Module if created)
          for (u32 i = 0; i < paramNum; i++)
               m->p[i].index = i;                               // init parameter indexes
     }     

     m->outputVolume = 1;
     for (int i = 0; i < STEREO; i++)                           // let output be zeroed
         this->output[i] = 0;


}

void mixer_init(module_t *const mix, parameter_t p, u32 paramNum) {
     module_init((module_t *) mix, p, paramNum);

     other mixer shit init 

}