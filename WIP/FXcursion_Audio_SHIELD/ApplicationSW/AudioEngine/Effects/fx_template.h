/**
 * @file      fx_template.h
 *
 * @details   Copy this file to start a new effect. See fx_template.c.
 *
 * @version   1.0.0
 *
 * @authors   Claude (design draft)
 *
 * \date      31.08.2026 - First release
 *
 * @copyright RAT Synthesizers
 */

#ifndef FX_TEMPLATE_H
#define FX_TEMPLATE_H



/***************************************************************************************************
* Module includes
***************************************************************************************************/

#include "fx_common.h"



/***************************************************************************************************
* Declarations of global (public) functions
***************************************************************************************************/

extern void FxTemplate_Process(const FX_CTX* pCtx, FLOAT32* const apPlane[], const U16 nFrames);
extern void FxTemplate_Reset(const U8 nPlaneBase, const U8 nWidth);



#endif // #ifndef FX_TEMPLATE_H

/****************************************** end of file *******************************************/
