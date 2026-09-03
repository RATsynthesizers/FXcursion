/**
 * @file      fx_crc.c
 *
 * @details   CRC-16/CCITT-FALSE implementation.
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



/***************************************************************************************************
* Module includes
***************************************************************************************************/

#include "fx_crc.h"



/***************************************************************************************************
* Definitions of local (private) constants
***************************************************************************************************/

#define CRC16_POLY                  (0x1021U)

/*
 * CRC-32/ISO-HDLC nibble table: the reflected polynomial 0xEDB88320 evaluated
 * for each of the 16 low-nibble values.
 *
 * Written out rather than generated at start-up so there is no init call and
 * no "has the table been built yet" state in shared code. The values are only
 * as trustworthy as the test that checks them, which is why test_loop.c runs
 * the standard vectors - "123456789" -> 0xCBF43926 among them - rather than
 * taking the table on faith.
 */
static const U32 aCrc32Nibble[16] =
{
    0x00000000UL, 0x1DB71064UL, 0x3B6E20C8UL, 0x26D930ACUL,
    0x76DC4190UL, 0x6B6B51F4UL, 0x4DB26158UL, 0x5005713CUL,
    0xEDB88320UL, 0xF00F9344UL, 0xD6D6A3E8UL, 0xCB61B38CUL,
    0x9B64C2B0UL, 0x86D3D2D4UL, 0xA00AE278UL, 0xBDBDF21CUL
};



/***************************************************************************************************
* Definitions of global (public) functions
***************************************************************************************************/

U16 Crc16_Ccitt(const U8* const pData, const U16 nLength, const U16 nInit)
{
    U16 nCrc = nInit;
    U16 i;

    if ((pData != NULL_PTR) && (nLength > 0U))
    {
        for (i = 0U; i < nLength; i++)
        {
            U8 nBit;

            nCrc ^= (U16)((U16)pData[i] << 8U);

            for (nBit = 0U; nBit < 8U; nBit++)
            {
                if ((nCrc & 0x8000U) != 0U)
                {
                    nCrc = (U16)(((U16)(nCrc << 1U)) ^ CRC16_POLY);
                }
                else
                {
                    nCrc = (U16)(nCrc << 1U);
                }
            }
        }
    }

    return nCrc;
}


U32 Crc32_Ieee(const U8* const pData, const U32 nLength, const U32 nInit)
{
    /* Held inverted while running, inverted again on the way out, so that both
       the argument and the return value are ordinary finished CRCs and there
       is no separate finalise step for a caller to forget. */
    U32 nCrc = ~nInit;
    U32 i;

    if ((pData != NULL_PTR) && (nLength > 0UL))
    {
        for (i = 0UL; i < nLength; i++)
        {
            const U8 nByte = pData[i];

            /* Reflected CRC, so the LOW nibble goes first. */
            nCrc = aCrc32Nibble[(nCrc ^ (U32)nByte) & 0x0FUL] ^ (nCrc >> 4U);
            nCrc = aCrc32Nibble[(nCrc ^ ((U32)nByte >> 4U)) & 0x0FUL] ^ (nCrc >> 4U);
        }
    }

    return ~nCrc;
}

/****************************************** end of file *******************************************/
