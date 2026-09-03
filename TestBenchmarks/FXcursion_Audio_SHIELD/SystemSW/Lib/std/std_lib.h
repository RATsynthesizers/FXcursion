
#ifndef STD_LIB_H
#define STD_LIB_H



/***************************************************************************************************
 * Module includes
 **************************************************************************************************/

// Get general definitions
#include "general.h"

/* The C Standard Library <stdint.h> header */ 
#include <stdint.h>

/* The C Standard Library <stdbool.h> header */ 
#include <stdbool.h>



/***************************************************************************************************
 * Declarations of global (public) functions
 **************************************************************************************************/

/**
 * @brief The sprintf function formats and stores character sets and values in buffer.
 *          Each argument (if any) preis formed and output according to the corresponding
 *          specification of the form mat in format-string
 *
 * @param p_str Pointer to the string to which the characters will be written 
 * @param p_fmt Format control string
 * @return If successful it returns the total number of characters written excluding
 *          null-character appended in the string, in case of failure a negative number is returned
 */
extern int std_sprintf(char* const p_str, const char *p_fmt, ...);



/**
 * @fn int8_t mem_cmp(void* const, void* const, const uint16_t)
 * 
 * @brief Compares content of the memory blocks.
 *
 * @param  p_mem1 - pointer to memory block1
 * @param  p_mem2 - pointer to memory block2
 * @param  size - memory block's size
 * 
 * @return int8_t - result of the function execution.
 *                  0 - the blocks are equal, !0 - the blocks are not equal
*/
extern int8_t mem_cmp(void* const p_mem1,
                      void* const p_mem2,
                      const uint16_t size);



/**
 * @fn int8_t ascii_to_uint(char* const p_str,
                            const uint16_t str_length,
                            uint32_t* const p_int_value)
 *
 * @brief Compares content of the memory blocks.
 *
 * @param  p_str - pointer to ascii string
 * @param  str_length - string length
 * @param  p_int_value - pointer to store integer value
 *
 * @return int8_t - result of the function execution.
 *                  0 - success, !0 - not success
*/
extern int8_t ascii_to_uint(char* const p_str,
                            const uint16_t str_length,
                            uint32_t* const p_int_value);



/**
 * @fn    void Byte2AsciiHex(const U8 nByte,
                             char* const pAsciiBuf)
 *
 * @brief Translate byte to 2 ASCII chars in hex manner.
 *
 * @param[in] nByte - input byte.
 * @param[out] pAsciiBuf - pointer to output buffer.
 *
 * @return    None.
 */
extern void Byte2AsciiHex(const char nByte,
                          char* const pAsciiBuf);



extern int ip_to_bytes(char* const ip_str, char* const ip_bytes);



#endif // #ifndef STD_LIB_H

/****************************************** end of file *******************************************/
