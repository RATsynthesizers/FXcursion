/// Module.h

#ifndef MODULE_H_
#define MODULE_H_

#include <Handy.h>
#include <stddef.h>

//--------------------------------------------------
typedef enum {
	M_CHORUS,      
	M_COMPRESSOR,  
	M_DELAY,      
	M_DISTORTION,
	M_FLANGER,     
	M_OVERDRIVE, 
	M_PHASER,   
	M_REVERB,  
	M_TREMOLO,  
	M_VIBRATO   
} m_id_t;

typedef enum {
// amp
	P_GAIN,
	P_PAN
// overdrive
	// P_FAIN
	P_OVERDRIVE
	P_MIX
// mixer
	P_IN_CH1_GAIN
	P_IN_CH2_GAIN
	P_IN_CH3_GAIN
	P_IN_CH4_GAIN
	P_OUT_CH1_GAIN
	P_OUT_CH2_GAIN
	P_OUT_CH3_GAIN
	P_OUT_CH4_GAIN
// ...

} p_id_t;
//--------------------------------------------------

static u32 allParamNum;     // used to count how many Parameters were used by all Modules in total; 0 by default
static u32 allModulesNum;   // used to count how many Module objects were created. To help make MDMA lists for example

typedef struct module_s module_t;

typedef struct module_vtable {
void (*process)(module_t *self); // subclass-defined TODO void?
void (*moduleInit)(module_t *self, u32 paramNum, u16 module_id);
void (*plugNext)(module_t *self, module_t *nextModule;
void (*plugPrev)(module_t *self, module_t *prevModule);
void (*unplugNext)(module_t *self);
void (*unplugPrev)(module_t *self);
} module_vtable_t;

// macro for assigning shared module fcns and polymorphic process fcn
#define _MODULE_VTABLE(process_subclass)  \
{                                         \
    .process    = process_subclass,       \
    .moduleInit = module_init,            \
    .plugNext   = plug_next,              \
    .plugPrev   = plug_prev,              \
    .unplugNext = unplug_next,            \
    .unplugPrev = unplug_prev             \
}

// parameter 
typedef struct {
	float val;
	p_id_t id;
}	param_t;

// module superclass
struct module_s {
	u32 paramNum;           // num of parameters in module

	module_t *prevModule;
	module_t *nextModule;

	float output[STEREO];
	float outputVolume;

	m_id_t id;				// module ID (name)

    const module_vtable *vtable;
}


#endif /* MODULE_H_ */


