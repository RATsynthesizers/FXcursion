/**
 * @file      assert.h
 * 
 * @details   Interface of the assert module.
 * 
 * @version   1.0.0
 * 
 * @authors   Vladimir Adamovskiy (adam2034@yandex.ru)
 * 
 * \date      01.06.2023 - First release
 * 
 */

#ifndef ASSERT_H
#define ASSERT_H



/*  Module includes */
#include "general.h"



/* Module Interface */

/**
 * @fn     void assert_failed(U8*, U32)
 *
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @example U8 i = 5;
            U8 j = 6;
            ASSERT(i > j);
 *
 * @param  file - pointer to file name
 * @param  line - line number
 *
 * @return None.
 */
extern void assert_failed(U8 *file, U32 line);



#endif
