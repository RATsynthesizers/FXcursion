#ifndef GENERAL_PLATFORM_H
#define GENERAL_PLATFORM_H



/**
 * @brief The C Standard Library <stdint.h> header
 */
#include <stdint.h>

/**
 * @brief The C Standard Library <stdbool.h> header
 */
#include <stdbool.h>



/**
 * @brief 1 byte unsigned (8 bit) for using with TRUE/FALSE
 */
typedef unsigned char         BOOLEAN;
/**
 * @brief 1 byte signed (8 bit)
 */
typedef signed char           S8;

/**
 * @brief 1 byte unsigned (8 bit)
 */
typedef unsigned char         U8;

/**
 * @brief 2 byte signed (16 bit)
 */
typedef short                 S16;

/**
 * @brief 2 byte unsigned (16 bit)
 */
typedef unsigned short        U16;

/**
 * @brief 4 byte signed (32 bit)
 */
typedef long                  S32;

/**
 * @brief 4 byte unsigned (32 bit)
 */
typedef unsigned long         U32;

/**
 * @brief 8 byte signed (64 bit)
 */
typedef long long             S64;

/**
 * @brief 8 byte unsigned (64 bit)
 */
typedef unsigned long long    U64;

/**
 * @brief float (32 bit)
 */
typedef float                 FLOAT32;

/**
 * @brief float (64 bit)
 */
typedef double                FLOAT64;



#endif // #ifndef GENERAL_TYPES_H

/****************************************** end of file *******************************************/
