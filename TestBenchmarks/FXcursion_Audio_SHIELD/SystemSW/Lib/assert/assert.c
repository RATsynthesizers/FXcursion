/**
 * @file      assert.c
 * 
 * @details   Implementation of the assert module.
 * 
 * @version   1.0.0
 * 
 * @authors   Vladimir Adamovskiy (adam2034@yandex.ru)
 * 
 * \date      01.06.2023 - First release
 * 
 */



/* Module includes */

// Get native header
#include "assert.h"

//// Get debug console interface
//#include "debug_console.h"


/* Module constants */

// None.



/* Global functions implementation */

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
void assert_failed(U8 *file, U32 line)
{
//    PRINTF_ATTR_SYNC(DEBUG_COLOR_RED,
//                     DEBUG_FONT_BOLD,
//                     WITH_TIMESTAMP,
//                     "Assert failed from %s file %d line\r\n",
//                     file, line);
    while (1)
    {

    }
}
