#ifndef SYSTEMNAV_HPP
#define SYSTEMNAV_HPP

/**
 * @file    SystemNav.hpp
 *
 * @details Where the cursor goes on the System screen, and nothing else.
 *
 *          NO TOUCHGFX, NO WIDGETS, NO GUI ENUMS. Positions are plain
 *          integers, so this logic can be - and is - exercised on the host
 *          across every topology and every cursor position. SystemView owns
 *          the translation between these integers and its containers.
 *
 *          ---------------------------------------------------------------
 *          WHY THIS FILE EXISTS
 *          ---------------------------------------------------------------
 *
 *          SystemView::encMenuUpdate was 1090 lines: the same walk written
 *          out once per chain, six times over. Normalising the chain index
 *          collapsed its 881 non-blank lines to 72 unique ones. Two bugs
 *          lived in that duplication, both in the copies least likely to be
 *          exercised by hand:
 *
 *            - btnUpUpdate decided between the mono and stereo form of the
 *              BOTTOM row by testing bIsStereo1, the flag for the TOP pair.
 *              With input 1 mono and input 2 stereo, leaving the stomp board
 *              upwards selected a container that is not on screen.
 *
 *            - btnDownUpdate mapped stereo chain 2's slots onto the foot
 *              switches backwards - slot 1 to switch 1 - where every other
 *              site in the file maps slot 1 to switch 3.
 *
 *          Neither is expressible here. There is one row table, so "the
 *          bottom row" is an index rather than a hand-picked container, and
 *          one slot/foot-switch mapping, so it cannot disagree with itself.
 *
 *          ---------------------------------------------------------------
 *          THE COORDINATE SPACE
 *          ---------------------------------------------------------------
 *
 *          nRow is a chain row index 0..nRowQty-1, top to bottom, or one of
 *          the three off-grid rows below. How many chain rows exist and which
 *          container each one is depends on the two stereo flags: a stereo
 *          input pair is ONE row where a mono pair is two. That is the whole
 *          reason the old code needed six copies.
 *
 *          nSlot is a grid column 0..NAV_SLOT_QTY-1 with 0 NEAREST THE INPUT,
 *          matching both the GUI's ChainModuleNumber and the protocol's grid
 *          slot index. On NAV_ROW_STOMP it is a foot-switch index instead;
 *          on NAV_ROW_INPUT and NAV_ROW_OUTPUT it is unused.
 */

#include "general.h"

/** Grid columns per chain. Matches GRID_SLOT_QTY on the wire. */
#define NAV_SLOT_QTY            (4U)

/** Foot switches along the bottom. */
#define NAV_FOOT_QTY            (3U)

/** Most chain rows that can be on screen at once (four mono inputs). */
#define NAV_ROW_MAX             (4U)

/** Off-grid rows. Negative so they cannot collide with a row index. */
#define NAV_ROW_INPUT           ((S8)-1)
#define NAV_ROW_OUTPUT          ((S8)-2)
#define NAV_ROW_STOMP           ((S8)-3)

/**
 * @brief What the cursor can move over. Everything the walk needs to know.
 */
typedef struct stNAV_CTX
{
    U8      nRowQty;        /**< active chain rows, 2..NAV_ROW_MAX          */
    BOOLEAN bMixerAdded;    /**< TRUE when a mixer is on the grid           */
    S8      nMixerCol;      /**< its column 0..NAV_SLOT_QTY-1, else -1      */

} NAV_CTX;

/**
 * @brief A cursor position.
 */
typedef struct stNAV_POS
{
    S8 nRow;                /**< chain row, or NAV_ROW_*                    */
    U8 nSlot;               /**< grid column, or foot switch on the stomp row */

} NAV_POS;


/**
 * @brief Foot switch under a grid column.
 *
 * The chain is drawn right to left with column 0 next to the input, and the
 * foot switches left to right, so the two orders are opposite: column 0 sits
 * above switch 3. The middle two columns share the middle switch.
 *
 * This is the mapping every site in SystemView.cpp used except one, which is
 * the bug described at the top of this file. There is now only one of it.
 */
static inline U8 Nav_FootForSlot(const U8 nSlot)
{
    switch (nSlot)
    {
    case 0U:  return 2U;                    /* column nearest IN  -> switch 3 */
    case 3U:  return 0U;                    /* column nearest OUT -> switch 1 */
    default:  return 1U;                    /* both middle columns            */
    }
}

/**
 * @brief Grid column above a foot switch.
 *
 * Not a perfect inverse, and deliberately the same choice the old code made:
 * two columns map onto the middle switch, and coming back up from it picks
 * column 2. Leaving column 1 upwards and returning therefore lands one column
 * over - a rough edge in the original layout, preserved so that this refactor
 * changes no behaviour a user could notice.
 */
static inline U8 Nav_SlotForFoot(const U8 nFoot)
{
    switch (nFoot)
    {
    case 0U:  return 3U;                    /* switch 1 -> column nearest OUT */
    case 2U:  return 0U;                    /* switch 3 -> column nearest IN  */
    default:  return 2U;                    /* switch 2                       */
    }
}

/** TRUE when the cursor is standing on a chain row rather than off-grid. */
static inline BOOLEAN Nav_IsChainRow(const NAV_POS tPos)
{
    return (tPos.nRow >= 0) ? TRUE : FALSE;
}

/**
 * @brief TRUE when the cursor is on the mixer.
 *
 * The mixer is ONE widget spanning a whole column across every chain row, so
 * "on the mixer" is a column test, not a cell test - and it is why vertical
 * movement stops there: within the mixer there is no row to move to.
 */
static inline BOOLEAN Nav_IsOnMixer(const NAV_CTX* const pCtx,
                                    const NAV_POS tPos)
{
    if ((FALSE == Nav_IsChainRow(tPos)) || (FALSE == pCtx->bMixerAdded))
    {
        return FALSE;
    }

    return (pCtx->nMixerCol == (S8)tPos.nSlot) ? TRUE : FALSE;
}

/**
 * @brief Move along a row. nDir is +1 towards the INPUT, -1 towards the OUTPUT.
 *
 * @param nRememberedRow the chain row to re-enter when leaving INPUT or
 *                       OUTPUT - the one the cursor left from, already
 *                       resolved to a row that exists under the current
 *                       topology.
 */
static inline NAV_POS Nav_Horizontal(const NAV_CTX* const pCtx,
                                     const NAV_POS tCur,
                                     const S8 nDir,
                                     const S8 nRememberedRow)
{
    NAV_POS tNew = tCur;

    if (0 == nDir)
    {
        return tNew;
    }

    if (NAV_ROW_INPUT == tCur.nRow)
    {
        /* Only inwards. The column nearest the input is 0. */
        if (nDir < 0)
        {
            tNew.nRow  = nRememberedRow;
            tNew.nSlot = 0U;
        }
        return tNew;
    }

    if (NAV_ROW_OUTPUT == tCur.nRow)
    {
        if (nDir > 0)
        {
            tNew.nRow  = nRememberedRow;
            tNew.nSlot = (U8)(NAV_SLOT_QTY - 1U);
        }
        return tNew;
    }

    if (NAV_ROW_STOMP == tCur.nRow)
    {
        /* Switch 3 is nearest the input, so +1 counts up here and down on a
           chain row. Clamped at both ends, as before. */
        if ((nDir > 0) && (tCur.nSlot < (NAV_FOOT_QTY - 1U)))
        {
            tNew.nSlot = (U8)(tCur.nSlot + 1U);
        }
        else if ((nDir < 0) && (tCur.nSlot > 0U))
        {
            tNew.nSlot = (U8)(tCur.nSlot - 1U);
        }
        return tNew;
    }

    /* On a chain row. Running off either end steps onto INPUT or OUTPUT. */
    if (nDir > 0)
    {
        if (0U == tCur.nSlot)
        {
            tNew.nRow = NAV_ROW_INPUT;
        }
        else
        {
            tNew.nSlot = (U8)(tCur.nSlot - 1U);
        }
    }
    else
    {
        if ((NAV_SLOT_QTY - 1U) == tCur.nSlot)
        {
            tNew.nRow = NAV_ROW_OUTPUT;
        }
        else
        {
            tNew.nSlot = (U8)(tCur.nSlot + 1U);
        }
    }

    return tNew;
}

/**
 * @brief Move between rows. nDir is -1 for up, +1 for down.
 *
 * ASYMMETRY WORTH KNOWING ABOUT, and preserved from the original: standing on
 * the mixer, UP does nothing while DOWN leaves for the stomp board. That is
 * consistent once you picture the mixer as a single tall cell touching the top
 * of the grid - there is no row above it to reach, but there is something
 * below.
 */
static inline NAV_POS Nav_Vertical(const NAV_CTX* const pCtx,
                                   const NAV_POS tCur,
                                   const S8 nDir)
{
    NAV_POS tNew = tCur;

    if (0 == nDir)
    {
        return tNew;
    }

    if (nDir < 0)
    {
        /* ---- up ---- */

        if (NAV_ROW_STOMP == tCur.nRow)
        {
            /* Into the BOTTOM chain row. An index, so it cannot be the wrong
               container for the topology - which is exactly the bug this
               replaces. */
            tNew.nRow  = (S8)(pCtx->nRowQty - 1U);
            tNew.nSlot = Nav_SlotForFoot(tCur.nSlot);
            return tNew;
        }

        if (FALSE == Nav_IsChainRow(tCur))
        {
            /* INPUT and OUTPUT are full-height; there is nowhere above. */
            return tNew;
        }

        if (FALSE != Nav_IsOnMixer(pCtx, tCur))
        {
            return tNew;
        }

        if (tCur.nRow > 0)
        {
            tNew.nRow = (S8)(tCur.nRow - 1);
        }

        return tNew;
    }

    /* ---- down ---- */

    if (NAV_ROW_INPUT == tCur.nRow)
    {
        tNew.nRow  = NAV_ROW_STOMP;
        tNew.nSlot = 2U;                        /* switch 3, under the input */
        return tNew;
    }

    if (NAV_ROW_OUTPUT == tCur.nRow)
    {
        tNew.nRow  = NAV_ROW_STOMP;
        tNew.nSlot = 0U;                        /* switch 1, under the output */
        return tNew;
    }

    if (NAV_ROW_STOMP == tCur.nRow)
    {
        return tNew;                            /* already at the bottom */
    }

    if (FALSE != Nav_IsOnMixer(pCtx, tCur))
    {
        /* Straight out of the bottom of the mixer column. */
        tNew.nRow  = NAV_ROW_STOMP;
        tNew.nSlot = Nav_FootForSlot((U8)pCtx->nMixerCol);
        return tNew;
    }

    if (tCur.nRow >= (S8)(pCtx->nRowQty - 1U))
    {
        tNew.nRow  = NAV_ROW_STOMP;
        tNew.nSlot = Nav_FootForSlot(tCur.nSlot);
        return tNew;
    }

    tNew.nRow = (S8)(tCur.nRow + 1);

    return tNew;
}

#endif // SYSTEMNAV_HPP
