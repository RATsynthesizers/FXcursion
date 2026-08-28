//// .h


typedef struct module_s module_t;
typedef uint32_t u32;

struct module_s {
	static u32 allParamNum; // used to count how many Parameters were used by all Modules in total; 0 by default
	u32 paramNum;           // num of parameters in module
	Parameter_t *p;           // pointer for module params to be stored to

	Module_t *prevModule;
	Module_t *nextModule;

	float output[STEREO];
	float outputVolume;

	char* name;

     module_vtable_t vtable;


	process();

}

typedef struct {
void (*ModuleInit)(module_t *const m, parameter_t p, u32 paramNum);
void (*process)(module_t *m);
void (*plugNext)(module_t *m);
void (*plugPrev)(module_t *m);
void (*unplugNext)(module_t *m);
void (*unplugPrev)(module_t *m);
} module_vtable_t;




typedef struct {
	typedef struct module_t super;
	oter shit

}	module_mixer_t



module_mixer_t mix1;
module_mixer_t mix2;




