#ifndef GENERAL_MACROS_H
#define GENERAL_MACROS_H



/**
 * @brief Contains platform-specific types definitions
 */
#include "general_platform.h"



// The function doing nothing definition
#ifndef do_nothing
#define do_nothing()
#endif

// Number of bits in each of the C data type
#define U8_BIT_QTY              (8U)
#define U16_BIT_QTY             (16U)
#define U32_BIT_QTY             (32U)
#define U64_BIT_QTY             (64U)

// Maximum values for various types
#define U8_MAX_VALUE            (0xFFU)
#define U16_MAX_VALUE           (0xFFFFU)
#define U32_MAX_VALUE           (0xFFFFFFFFU)
#define U64_MAX_VALUE           (0xFFFFFFFFFFFFFFFFU)

// Bit manipulation macro
#define _CHECK_BIT(x, bitnum)       (x & (1U << bitnum))
#define _SET_BIT(x, bitnum)         (x |= (1U << bitnum))
#define _CLR_BIT(x, bitnum)         (x &= ~(1U << bitnum))
#define _TOGGLE_BIT(x, bitnum)      (x ^= (1U << bitnum))


#define MAKEBYTE(lowNibble, highNibble) (((uint8_t)(lowNibble) & 0x0FU) | (((uint8_t)(highNibble) & 0x0FU) << 4U))
#define MAKEWORD(lowByte, highByte)     ((uint16_t)(((uint8_t)(lowByte)) | ((uint16_t)((uint8_t)(highByte))) << U8_BIT_QTY))
#define MAKELONG(lowWord, highWord)     ((uint32_t)(((uint16_t)((uint16_t)(lowWord) & 0xFFFFU)) | ((uint32_t)((uint16_t)((uint16_t)(highWord) & 0xFFFFU))) << U16_BIT_QTY))
#define LONIBBLE(b) (((uint8_t)(b)) & 0x0FU)
#define HINIBBLE(b) ((((uint8_t)(b)) & 0xF0U) >> 4U)
#define LOBYTE(w)   ((uint8_t)((uint16_t)(w) & 0x00FFU))
#define HIBYTE(w)   ((uint8_t)((((uint16_t)(w)) >> U8_BIT_QTY) & (uint16_t)0x00FFU))
#define LOWORD(l)   ((uint16_t)((uint32_t)(l) & 0xFFFFU))
#define HIWORD(l)   ((uint16_t)(((uint32_t)(l) >> U16_BIT_QTY) & 0xFFFFU))

// Returns size of the specified array in elements
#define SIZE_OF_ARRAY(array)    (sizeof(array) / sizeof((array)[0U]))

// Check, whether value is in the specified range.
// This inequality is UNstrict,
// i.e. LOW and HIGH values are possible (in the specified range).
#define IsValueInRange(LOW, Value, HIGH)     ((((LOW)  <   (Value))    \
                                            || ((LOW)  ==  (Value)))   \
                                            && ((HIGH) >=  (Value)))

// Converts (MHz - frequency) into (Hz - frequency)
#define MHZ_TO_HZ(MHz)        ((MHz) * (1000000UL))
#define HZ_TO_MHZ(Hz)         ((Hz)  / (1000000UL))

// Integer rounding support macro
#define ROUND_TO(nValue, nRoundTo)          (((nValue) / (nRoundTo)) * (nRoundTo))

// ASCII conversion support
// Convert HEX value to ASCII symbol: HEX = {0x0..0xF} -> {'0'..'F'}
#define BIN_TO_ASCII(digit)     (((digit) > 9U) ? ((digit) + 0x37U) : ((digit) + 0x30U))
// Convert ASCII symbol to its HEX value: CHAR = {'0'..'F'} -> {0x0..0xF}
#define ASCII_TO_BIN(symbol)    (((symbol) >= 0x41U) ? ((symbol) - 0x37U) : ((symbol) - 0x30U))

//// Find maximum value macro
//#define MAX(valueA, valueB)     (((valueA) > (valueB)) ? (valueA) : (valueB))
//// Find minimum value macro
//#define MIN(valueA, valueB)     (((valueA) < (valueB)) ? (valueA) : (valueB))

// Amount seconds in one hour
#define SECOND_IN_ONE_HOUR                  (3600U)
// Amount milliseconds in one second
#define SECOND_IN_ONE_MINUTE                (60U)
// Amount milliseconds in one second
#define MILLISECOND_IN_ONE_SECOND           (1000U)
// Amount microseconds in one millisecond
#define MICROSECOND_IN_ONE_MILLISECOND      (1000U)


// Assert macro
//void assert_failed(uint8_t *file, uint32_t line);
#define ASSERT(expr) ((expr) ? (void)0U : assert_failed((uint8_t *)__FILE__, __LINE__))



// Macro for passing a string and it size to a function
#define PASS_STRING(string)          string, sizeof(string) - 1U



#endif // #ifndef GENERAL_MACROS_H

/****************************************** end of file *******************************************/
