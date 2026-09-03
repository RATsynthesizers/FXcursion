/*
 * Host harness for SystemNav.hpp.
 *
 * This compiles the REAL header - not a transcription of it - which is the
 * whole reason the navigation was pulled out into framework-free integers.
 * The encoder harness taught me what a replicated model is worth: it passed
 * while the firmware was wrong, because the model differed from the hardware.
 * There is no model to get wrong here.
 *
 * The two bugs this refactor replaces were both "one copy out of six
 * disagreed with the other five", so the checks below are mostly exhaustive
 * sweeps and invariants rather than hand-picked cases.
 */
#include <stdio.h>
#include <string.h>

#include "gui/system_screen/SystemNav.hpp"

static int nChecks = 0;
static int nFails  = 0;

#define CHECK(cond, fmt, ...)                                                \
    do {                                                                     \
        nChecks++;                                                           \
        if (!(cond)) {                                                       \
            nFails++;                                                        \
            printf("FAIL line %d: " fmt "\n", __LINE__, __VA_ARGS__);         \
        }                                                                    \
    } while (0)

/* Every row count a real topology can produce, against the stereo flags that
   produce it. Group 1 stereo -> 1 row, mono -> 2; likewise group 2. */
typedef struct { int bSt1, bSt2; U8 nRowQty; const char* pName; } TOPO;

static const TOPO aTopo[4] = {
    { 0, 0, 4U, "4 mono"          },
    { 1, 0, 3U, "stereo1 + 2 mono"},
    { 0, 1, 3U, "2 mono + stereo2"},
    { 1, 1, 2U, "2 stereo"        },
};

static int PosEq(NAV_POS a, NAV_POS b)
{
    return (a.nRow == b.nRow) && (a.nSlot == b.nSlot);
}

static const char* RowName(S8 nRow)
{
    switch (nRow) {
    case NAV_ROW_INPUT:  return "IN";
    case NAV_ROW_OUTPUT: return "OUT";
    case NAV_ROW_STOMP:  return "STOMP";
    default:             return "row";
    }
}

/* Is this a position the cursor is ever allowed to occupy? */
static int PosLegal(const NAV_CTX* p, NAV_POS t)
{
    if (t.nRow == NAV_ROW_INPUT || t.nRow == NAV_ROW_OUTPUT) return 1;
    if (t.nRow == NAV_ROW_STOMP) return t.nSlot < NAV_FOOT_QTY;
    if (t.nRow < 0) return 0;
    return (t.nRow < (S8)p->nRowQty) && (t.nSlot < NAV_SLOT_QTY);
}

int main(void)
{
    /* ---- the two mappings agree with the layout, and with each other ---- */
    CHECK(Nav_FootForSlot(0) == 2, "slot 0 -> foot %u, expected 2", Nav_FootForSlot(0));
    CHECK(Nav_FootForSlot(1) == 1, "slot 1 -> foot %u, expected 1", Nav_FootForSlot(1));
    CHECK(Nav_FootForSlot(2) == 1, "slot 2 -> foot %u, expected 1", Nav_FootForSlot(2));
    CHECK(Nav_FootForSlot(3) == 0, "slot 3 -> foot %u, expected 0", Nav_FootForSlot(3));

    /* Column nearest IN pairs with the switch nearest IN, at both ends. This
       is the invariant the inverted stereo-chain-2 copy violated. */
    CHECK(Nav_SlotForFoot(Nav_FootForSlot(0)) == 0,
          "slot 0 does not round-trip: %u", Nav_SlotForFoot(Nav_FootForSlot(0)));
    CHECK(Nav_SlotForFoot(Nav_FootForSlot(3)) == 3,
          "slot 3 does not round-trip: %u", Nav_SlotForFoot(Nav_FootForSlot(3)));
    /* Both mappings must be monotone in opposite directions. */
    for (U8 s = 0; s + 1 < NAV_SLOT_QTY; s++)
        CHECK(Nav_FootForSlot(s) >= Nav_FootForSlot((U8)(s + 1U)),
              "foot mapping not monotone at slot %u", s);

    for (int t = 0; t < 4; t++) {
        const TOPO* pT = &aTopo[t];

        /* Sweep every mixer placement, including none. */
        for (int mix = -1; mix < (int)NAV_SLOT_QTY; mix++) {
            NAV_CTX ctx;
            ctx.nRowQty     = pT->nRowQty;
            ctx.bMixerAdded = (mix >= 0) ? TRUE : FALSE;
            ctx.nMixerCol   = (S8)mix;

            /* Build the full set of legal positions for this context. */
            NAV_POS aAll[NAV_ROW_MAX * NAV_SLOT_QTY + NAV_FOOT_QTY + 2];
            int nAll = 0;
            for (S8 r = 0; r < (S8)ctx.nRowQty; r++)
                for (U8 s = 0; s < NAV_SLOT_QTY; s++)
                    { aAll[nAll].nRow = r; aAll[nAll].nSlot = s; nAll++; }
            for (U8 f = 0; f < NAV_FOOT_QTY; f++)
                { aAll[nAll].nRow = NAV_ROW_STOMP; aAll[nAll].nSlot = f; nAll++; }
            aAll[nAll].nRow = NAV_ROW_INPUT;  aAll[nAll].nSlot = 0; nAll++;
            aAll[nAll].nRow = NAV_ROW_OUTPUT; aAll[nAll].nSlot = 0; nAll++;

            for (int i = 0; i < nAll; i++) {
                NAV_POS cur = aAll[i];

                /* --- INVARIANT 1: no move ever lands somewhere illegal.
                       This is what makes "select a container that is not on
                       screen" unrepresentable. --- */
                for (S8 d = -1; d <= 1; d += 2) {
                    for (S8 rem = 0; rem < (S8)ctx.nRowQty; rem++) {
                        NAV_POS h = Nav_Horizontal(&ctx, cur, d, rem);
                        CHECK(PosLegal(&ctx, h),
                              "%s mix%d: H(%s %d/%u, %+d) -> illegal %d/%u",
                              pT->pName, mix, RowName(cur.nRow), cur.nRow,
                              cur.nSlot, d, h.nRow, h.nSlot);
                    }
                    NAV_POS v = Nav_Vertical(&ctx, cur, d);
                    CHECK(PosLegal(&ctx, v),
                          "%s mix%d: V(%s %d/%u, %+d) -> illegal %d/%u",
                          pT->pName, mix, RowName(cur.nRow), cur.nRow,
                          cur.nSlot, d, v.nRow, v.nSlot);
                }

                /* --- INVARIANT 2: a move changes at most one axis, and never
                       teleports between two chain rows more than one apart. --- */
                for (S8 d = -1; d <= 1; d += 2) {
                    NAV_POS v = Nav_Vertical(&ctx, cur, d);
                    if (cur.nRow >= 0 && v.nRow >= 0) {
                        int step = v.nRow - cur.nRow;
                        CHECK(step == 0 || step == d,
                              "%s mix%d: V(row %d, %+d) jumped to row %d",
                              pT->pName, mix, cur.nRow, d, v.nRow);
                        CHECK(v.nSlot == cur.nSlot,
                              "%s mix%d: V(row %d slot %u) changed slot to %u",
                              pT->pName, mix, cur.nRow, cur.nSlot, v.nSlot);
                    }
                }

                /* --- INVARIANT 3: horizontal on a chain row keeps the row,
                       except at the two ends where it steps off-grid. --- */
                if (cur.nRow >= 0) {
                    NAV_POS r = Nav_Horizontal(&ctx, cur, +1, 0);
                    NAV_POS l = Nav_Horizontal(&ctx, cur, -1, 0);
                    if (cur.nSlot == 0)
                        CHECK(r.nRow == NAV_ROW_INPUT, "slot 0 +1 -> row %d", r.nRow);
                    else
                        CHECK(r.nRow == cur.nRow && r.nSlot == cur.nSlot - 1,
                              "%s: slot %u +1 -> %d/%u", pT->pName, cur.nSlot, r.nRow, r.nSlot);
                    if (cur.nSlot == NAV_SLOT_QTY - 1)
                        CHECK(l.nRow == NAV_ROW_OUTPUT, "last slot -1 -> row %d", l.nRow);
                    else
                        CHECK(l.nRow == cur.nRow && l.nSlot == cur.nSlot + 1,
                              "%s: slot %u -1 -> %d/%u", pT->pName, cur.nSlot, l.nRow, l.nSlot);
                }

                /* --- INVARIANT 4: standing on the mixer, up does nothing and
                       down leaves for the stomp board. --- */
                if (Nav_IsOnMixer(&ctx, cur)) {
                    NAV_POS up = Nav_Vertical(&ctx, cur, -1);
                    NAV_POS dn = Nav_Vertical(&ctx, cur, +1);
                    CHECK(PosEq(up, cur), "%s mix%d: up off the mixer moved to %d/%u",
                          pT->pName, mix, up.nRow, up.nSlot);
                    CHECK(dn.nRow == NAV_ROW_STOMP,
                          "%s mix%d: down off the mixer went to row %d",
                          pT->pName, mix, dn.nRow);
                    CHECK(dn.nSlot == Nav_FootForSlot((U8)mix),
                          "%s mix%d: down off the mixer hit switch %u, expected %u",
                          pT->pName, mix, dn.nSlot, Nav_FootForSlot((U8)mix));
                }
            }

            /* --- INVARIANT 5: horizontal is reversible everywhere it moves.
                   Catches a direction sign flipped in one place. --- */
            for (int i = 0; i < nAll; i++) {
                NAV_POS cur = aAll[i];
                if (cur.nRow < 0) continue;                 /* off-grid re-entry
                                                               is by design not
                                                               an inverse */
                NAV_POS r = Nav_Horizontal(&ctx, cur, +1, cur.nRow);
                NAV_POS back = Nav_Horizontal(&ctx, r, -1, cur.nRow);
                CHECK(PosEq(back, cur),
                      "%s mix%d: %d/%u ->+1-> %d/%u ->-1-> %d/%u",
                      pT->pName, mix, cur.nRow, cur.nSlot,
                      r.nRow, r.nSlot, back.nRow, back.nSlot);
            }

            /* --- INVARIANT 6: leaving the stomp board upwards always lands on
                   the BOTTOM row, whatever the topology. This is the
                   bIsStereo1/bIsStereo2 bug, stated as a property. --- */
            for (U8 f = 0; f < NAV_FOOT_QTY; f++) {
                NAV_POS st; st.nRow = NAV_ROW_STOMP; st.nSlot = f;
                NAV_POS up = Nav_Vertical(&ctx, st, -1);
                CHECK(up.nRow == (S8)(ctx.nRowQty - 1U),
                      "%s mix%d: up from switch %u -> row %d, bottom row is %u",
                      pT->pName, mix, f, up.nRow, ctx.nRowQty - 1U);
                CHECK(up.nSlot == Nav_SlotForFoot(f),
                      "%s mix%d: up from switch %u -> slot %u", pT->pName, mix, f, up.nSlot);
            }

            /* --- INVARIANT 7: walking down from every chain row reaches the
                   stomp board, and never loops. --- */
            for (int i = 0; i < nAll; i++) {
                NAV_POS cur = aAll[i];
                if (cur.nRow < 0) continue;
                int nHops = 0;
                while (cur.nRow != NAV_ROW_STOMP && nHops < 16) {
                    NAV_POS nxt = Nav_Vertical(&ctx, cur, +1);
                    if (PosEq(nxt, cur)) break;
                    cur = nxt; nHops++;
                }
                CHECK(cur.nRow == NAV_ROW_STOMP,
                      "%s mix%d: walking down from %d/%u stalled at %d/%u after %d hops",
                      pT->pName, mix, aAll[i].nRow, aAll[i].nSlot,
                      cur.nRow, cur.nSlot, nHops);
            }
        }
        printf("  %-18s %u rows: swept\n", pT->pName, pT->nRowQty);
    }

    printf("\n%d checks, %d failures\n", nChecks, nFails);
    return nFails ? 1 : 0;
}
