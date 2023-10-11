/*
 * ModuleTemplate.hpp
 *
 *  Created on: Nov 11, 2020
 *      Author: romte
 */

#ifndef MODULETEMPLATE_HPP_
#define MODULETEMPLATE_HPP_

#include "../../Modules/libModules/Module.hpp"

#define MODULETEMPLATE_PARAMETERS 2    // refactor name

class ModuleTemplate: public Module {  // refactor name
public:
//------------------ parameter ptrs decl & def ------------------------//
	Parameter *parameter0;
	Parameter *parameter1;

	ModuleTemplate(Parameter *p) :
			Module(p, MODULETEMPLATE_PARAMETERS) {
//----------------- private variables init ----------------------------//
		priv = 0;
//----------------- parameter address init  ---------------------------//
		Parameter *tmp = this->p;
		parameter0 = tmp++;
		parameter1 = tmp++;
		this->paramNum = tmp - this->p; // calculate actual number of parameters
//----------------- parameter values init  ----------------------------//
		param0 = 0;
		param1 = 0;
	}
	void process(/* args */);   // define in .cpp
	~ModuleTemplate(){};
private:
//------------------ parameter shortcuts decl & def -------------------//
	float &param0 = parameter0->val;   // reference to actual value
	float &param1 = parameter1->val;

//------------------- private variables -------------------------------//
	float priv;


};




#endif /* MODULETEMPLATE_HPP_ */
