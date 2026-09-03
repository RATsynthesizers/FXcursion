// ModuleTemplate.h - example of new audio module

#ifndef MODULETEMPLATE_H_
#define MODULETEMPLATE_H_

#include "libModules/Module.hpp"

#define MODULETEMPLATE_PARAMETERS 	2    		
#define MODULETEMPLATE_NAME 		M_TEMPLATE  // of m_id_t

typedef struct {
	typedef struct module_t super;  // super class

	param_t param1; // parameters - right after superclass
	param_t param2;
	
	//oter shit
} template_t;

#endif /* MODULETEMPLATE_H_ */
