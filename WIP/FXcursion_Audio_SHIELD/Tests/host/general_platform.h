/**
 * @file      general_platform.h
 *
 * @details   HOST BUILD ONLY. Stand-in for SystemSW/Include/general_platform.h.
 *
 *            The SHIELD version defines U32 as "unsigned long", which is 32 bits
 *            on ARM and on Windows but 64 bits on 64-bit Linux and macOS. Every
 *            wire struct in this project is size-asserted, so building the tests
 *            against that header on a LP64 host fails at compile time - which is
 *            the correct outcome, but not a useful one.
 *
 *            This copy uses the exact-width types from <stdint.h> instead, so the
 *            host build sees the same layouts the target does.
 *
 * @copyright RAT Synthesizers
 */

#ifndef GENERAL_PLATFORM_H
#define GENERAL_PLATFORM_H

#include <stdint.h>
#include <stdbool.h>

typedef unsigned char   BOOLEAN;
typedef int8_t          S8;
typedef uint8_t         U8;
typedef int16_t         S16;
typedef uint16_t        U16;
typedef int32_t         S32;
typedef uint32_t        U32;
typedef int64_t         S64;
typedef uint64_t        U64;
typedef float           FLOAT32;
typedef double          FLOAT64;

#endif // #ifndef GENERAL_PLATFORM_H
