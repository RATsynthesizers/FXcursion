/**
 * @file      fx_crc.h
 *
 * @details   CRC-16/CCITT-FALSE, used by the control link and the preset format.
 *
 *            ############################################################
 *            #  DUPLICATED IN THE INTERFACE CONTROLLER - keep in sync.  #
 *            ############################################################
 *
 * @version   1.0.0
 *
 * @authors   Claude (design draft)
 *
 * \date      31.08.2026 - First release
 *
 * @copyright RAT Synthesizers
 */

#ifndef FX_CRC_H
#define FX_CRC_H



/***************************************************************************************************
* Module includes
***************************************************************************************************/

#include "general.h"

/* One wire contract, two compilers: the audio controller builds this as C, the
 * interface controller includes it from C++. Without this any symbol declared
 * below gets a mangled name and the link fails against the C definition. The
 * guard sits AFTER the includes on purpose - wrapping <string.h> in extern "C"
 * is the classic way to break a C++ build. */
#ifdef __cplusplus
extern "C" {
#endif




/***************************************************************************************************
* Declarations of global (public) functions
***************************************************************************************************/

/**
 * @brief CRC-16/CCITT-FALSE. Poly 0x1021, init 0xFFFF, no reflection, no final xor.
 *
 * Bitwise implementation: 8 iterations per byte, no lookup table. A 96-byte
 * configuration frame costs about 3 us on the M7, and this runs in the
 * super-loop, never in the audio ISR. If the control link ever gets fast enough
 * to matter, swap in a 256-entry table - the result must not change.
 *
 * @param pData     buffer, may be NULL only when nLength is 0
 * @param nLength   bytes
 * @param nInit     0xFFFF to start, or the running value to continue
 *
 * @return CRC over the buffer
 */
extern U16 Crc16_Ccitt(const U8* const pData, const U16 nLength, const U16 nInit);


/**
 * @brief CRC-32/ISO-HDLC. Poly 0xEDB88320 reflected, init 0xFFFFFFFF,
 *        reflected in and out, final xor 0xFFFFFFFF.
 *
 * The ordinary CRC32 - the one zlib, PNG, gzip and every desktop `crc32` tool
 * compute. That is the reason for choosing it over anything cleverer: a loop
 * file that arrives on the card can be checked against the value the audio
 * board reported using a tool the user already has, which turns "this loop
 * plays back as noise" into a question with an answer.
 *
 * Guards the loop transport payload, where CRC-16 was too weak to be obviously
 * fine: over 11 MiB it misses a corruption about once in 65536.
 *
 * CHAINING, WITHOUT A FINALISE STEP
 *
 * nInit is 0 to start and the previous return value to continue, and the
 * return is always the finished CRC - never an intermediate that someone has
 * to remember to invert. So
 *
 *      Crc32_Ieee(b, lb, Crc32_Ieee(a, la, 0))
 *
 * equals the CRC of a and b concatenated, and any single call is directly
 * comparable with what a desktop tool prints. This is the zlib crc32() calling
 * convention, for exactly that reason.
 *
 * Nibble table: 16 entries, 64 bytes of flash, two lookups per byte. About
 * 0.2 s for an 11 MiB loop against a transfer that takes several seconds, so
 * the 1 KiB byte-wide table is not worth its source.
 *
 * @param pData     buffer, may be NULL only when nLength is 0
 * @param nLength   bytes
 * @param nInit     0 to start, or the running value to continue
 *
 * @return CRC over the buffer, finished - "123456789" gives 0xCBF43926
 */
extern U32 Crc32_Ieee(const U8* const pData, const U32 nLength, const U32 nInit);




#ifdef __cplusplus
}
#endif

#endif // #ifndef FX_CRC_H

/****************************************** end of file *******************************************/
