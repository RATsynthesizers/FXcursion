/**
 * @file      chan_map.c
 *
 * @details   Channel map implementation. See chan_map.h for the model.
 *
 * @version   1.0.0
 *
 * @authors   Claude (design draft)
 *
 * \date      01.09.2026 - First release
 *
 * @copyright RAT Synthesizers
 */



/***************************************************************************************************
* Module includes
***************************************************************************************************/

#include "chan_map.h"



/***************************************************************************************************
* Definitions of local (private) constants
***************************************************************************************************/

/** Sign bit of a 24-bit sample sitting in a 32-bit word. */
#define CM_SIGN_BIT_24                  (0x00800000UL)

/** The bits the SAI actually shifts out. */
#define CM_MASK_24                      (0x00FFFFFFUL)

/** What the top byte becomes when the sample is negative. */
#define CM_SIGN_FILL_24                 (0xFF000000UL)



/***************************************************************************************************
* Definitions of local (private) functions
***************************************************************************************************/

/**
 * @brief One hardware word to one engine sample.
 *
 * Same idiom as Unpack24 in looper.c, deliberately: shifting a signed value
 * right to sign extend is implementation defined, and this is not.
 */
static S32 FromHw(const S32 nWord)
{
    U32 nRaw = (U32)nWord & CM_MASK_24;

    if ((nRaw & CM_SIGN_BIT_24) != 0UL)
    {
        nRaw |= CM_SIGN_FILL_24;
    }

    return (S32)nRaw;
}

//--------------------------------------------------------------------------------------------------

/**
 * @brief One engine sample to one hardware word.
 */
static S32 ToHw(const S32 nSample)
{
    return (S32)((U32)nSample & CM_MASK_24);
}



/***************************************************************************************************
* Definitions of global (public) functions
***************************************************************************************************/

void ChanMap_Gather(const S32* const pSai1,
                    const S32* const pSai2,
                    S32* const pBlock,
                    const U16 nFrames)
{
    U16 i;
    U8  s;

    for (i = 0U; i < nFrames; i++)
    {
        const U32 nSai = (U32)i * AIO_SLOTS_PER_SAI;
        const U32 nBlk = (U32)i * AUDIO_CH_QTY;

        for (s = 0U; s < AIO_SLOTS_PER_SAI; s++)
        {
            pBlock[nBlk + AIO_SAI1_PLANE_BASE + s] = FromHw(pSai1[nSai + s]);
            pBlock[nBlk + AIO_SAI2_PLANE_BASE + s] = FromHw(pSai2[nSai + s]);
        }
    }
}

//--------------------------------------------------------------------------------------------------

void ChanMap_Scatter(const S32* const pBlock,
                     S32* const pSai1,
                     S32* const pSai2,
                     const U16 nFrames)
{
    U16 i;
    U8  s;

    for (i = 0U; i < nFrames; i++)
    {
        const U32 nSai = (U32)i * AIO_SLOTS_PER_SAI;
        const U32 nBlk = (U32)i * AUDIO_CH_QTY;

        for (s = 0U; s < AIO_SLOTS_PER_SAI; s++)
        {
            pSai1[nSai + s] = ToHw(pBlock[nBlk + AIO_SAI1_PLANE_BASE + s]);
            pSai2[nSai + s] = ToHw(pBlock[nBlk + AIO_SAI2_PLANE_BASE + s]);
        }
    }
}

//--------------------------------------------------------------------------------------------------

void ChanMap_Silence(S32* const pBuf, const U32 nWords)
{
    U32 i;

    for (i = 0UL; i < nWords; i++)
    {
        pBuf[i] = 0;
    }
}

/****************************************** end of file *******************************************/
