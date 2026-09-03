/*
 * Host harness for the encoder step logic.
 *
 * IMPORTANT CORRECTION over the first version of this file. It modelled a
 * detent as a full quadrature cycle - (0,0)->(1,0)->(1,1)->(0,1) - and so
 * "passed" while the firmware was delivering one step per two clicks on real
 * hardware. The fitted encoder moves channel A ONCE per detent, i.e. half a
 * cycle, and that is the case that matters.
 *
 * The invariant actually worth asserting does not depend on which encoder is
 * fitted: ONE STEP PER EDGE OF A, sign correct. Both hardware models are
 * driven below and both are checked against that, so this file no longer
 * bakes in an assumption about detent geometry.
 */
#include <stdio.h>
#include <string.h>

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

/* ---------------- the OLD implementation, for comparison ---------------- */

enum { ENC_LEFTTURN = -1, ENC_STILL = 0, ENC_RIGHTTURN = 1 };

typedef struct { int state, bWasChanged, pinValueA, pinValueB; } OldEnc;

/* Returns what old UISurvey would have PUBLISHED, or 2 for "nothing". */
static int OldUpdateAndPublish(OldEnc* e, int a, int b)
{
    int prevState = e->state;
    e->state = ENC_STILL;
    if (a != e->pinValueA) e->state = (a != b) ? ENC_RIGHTTURN : ENC_LEFTTURN;
    if (e->state != prevState) e->bWasChanged = 1;
    e->pinValueA = a; e->pinValueB = b;
    if (e->bWasChanged) { e->bWasChanged = 0; return e->state; }
    return 2;
}

/* ---------------- the NEW implementation ---------------- */

#define MAX_PENDING_STEPS 32

typedef struct { signed char nPendingSteps; int bPrimed, pinValueA, pinValueB; } NewEnc;

static void NewUpdate(NewEnc* e, int a, int b)
{
    if (!e->bPrimed) { e->pinValueA = a; e->pinValueB = b; e->bPrimed = 1; return; }

    if (a != e->pinValueA) {                          /* EITHER edge of A */
        int nStep = (a != b) ? 1 : -1;
        if (nStep > 0 && e->nPendingSteps <  MAX_PENDING_STEPS) e->nPendingSteps++;
        else if (nStep < 0 && e->nPendingSteps > -MAX_PENDING_STEPS) e->nPendingSteps--;
    }
    e->pinValueA = a; e->pinValueB = b;
}

static signed char NewPendingStep(const NewEnc* e)
{
    if (e->nPendingSteps > 0) return  1;
    if (e->nPendingSteps < 0) return -1;
    return 0;
}

static void NewStepDelivered(NewEnc* e)
{
    if (e->nPendingSteps > 0) e->nPendingSteps--;
    else if (e->nPendingSteps < 0) e->nPendingSteps++;
}

/* Drain like PublishEncoderSteps; returns count, and the sign of every step
   (0 if the steps were not all the same sign). */
static int NewDrain(NewEnc* e, int* pSign)
{
    int n = 0, sign = 0, mixed = 0;
    signed char s;
    while ((s = NewPendingStep(e)) != 0) {
        if (n == 0) sign = s; else if (s != sign) mixed = 1;
        NewStepDelivered(e);
        n++;
    }
    *pSign = mixed ? 0 : sign;
    return n;
}

/* ---------------- hardware models ---------------- */

/* Full quadrature states, clockwise. A leads B. */
static const int CW[4][2]  = { {0,0}, {1,0}, {1,1}, {0,1} };

/*
 * Advance the (A,B) state by one detent and report how many A edges that
 * produced. nStatesPerDetent is 2 for the fitted encoder (one A edge per
 * detent) and 4 for a whole-cycle-per-detent part.
 */
static int StepDetent(NewEnc* pNew, OldEnc* pOld, int* pPhase, int nDir,
                      int nStatesPerDetent)
{
    int nAEdges = 0;
    for (int k = 0; k < nStatesPerDetent; k++) {
        int prevA = CW[*pPhase & 3][0];
        *pPhase = (*pPhase + (nDir > 0 ? 1 : 3)) & 3;
        int a = CW[*pPhase][0], b = CW[*pPhase][1];
        if (a != prevA) nAEdges++;
        if (pNew) NewUpdate(pNew, a, b);
        if (pOld) (void)OldUpdateAndPublish(pOld, a, b);
    }
    return nAEdges;
}

static void RunModel(const char* pLabel, int nStatesPerDetent)
{
    printf("  %s (%d states/detent):\n", pLabel, nStatesPerDetent);

    for (int nDir = 1; nDir >= -1; nDir -= 2) {
        NewEnc e; memset(&e, 0, sizeof e);
        int nPhase = 0;
        NewUpdate(&e, CW[0][0], CW[0][1]);          /* prime */

        int nTotalSteps = 0, nTotalEdges = 0;

        for (int detent = 1; detent <= 6; detent++) {
            int nEdges = StepDetent(&e, NULL, &nPhase, nDir, nStatesPerDetent);
            int sign; int n = NewDrain(&e, &sign);

            nTotalEdges += nEdges;
            nTotalSteps += n;

            /* THE invariant: one step per edge of A. */
            CHECK(n == nEdges, "%s dir %+d detent %d: %d steps for %d A edges",
                  pLabel, nDir, detent, n, nEdges);
            if (n > 0)
                CHECK(sign == nDir, "%s dir %+d detent %d: sign %d",
                      pLabel, nDir, detent, sign);
        }
        printf("    dir %+d: %d detents -> %d A edges -> %d steps\n",
               nDir, 6, nTotalEdges, nTotalSteps);
    }
}

int main(void)
{
    /* --- what the fitted hardware does, and the alternative part --- */
    RunModel("one A edge per detent  [FITTED]", 2);
    RunModel("full cycle per detent", 4);

    /* --- reversing direction mid-turn must not produce a wrong-way step --- */
    {
        NewEnc e; memset(&e, 0, sizeof e);
        int nPhase = 0;
        NewUpdate(&e, CW[0][0], CW[0][1]);

        for (int i = 0; i < 3; i++) {                       /* forward */
            StepDetent(&e, NULL, &nPhase, +1, 2);
            int sign; int n = NewDrain(&e, &sign);
            CHECK(n == 1 && sign == +1, "forward run: %d steps sign %d", n, sign);
        }
        for (int i = 0; i < 3; i++) {                       /* then back */
            StepDetent(&e, NULL, &nPhase, -1, 2);
            int sign; int n = NewDrain(&e, &sign);
            CHECK(n == 1 && sign == -1, "reverse run: %d steps sign %d", n, sign);
        }
    }

    /* --- a burst with no drain never delivers a magnitude above 1 --- */
    {
        NewEnc e; memset(&e, 0, sizeof e);
        int nPhase = 0;
        NewUpdate(&e, CW[0][0], CW[0][1]);

        for (int detent = 0; detent < 7; detent++)
            StepDetent(&e, NULL, &nPhase, +1, 2);

        int nDelivered = 0; signed char s;
        while ((s = NewPendingStep(&e)) != 0) {
            CHECK(s == 1 || s == -1, "delivered magnitude %d", (int)s);
            NewStepDelivered(&e);
            nDelivered++;
        }
        CHECK(nDelivered == 7, "7 detents delivered %d steps", nDelivered);
    }

    /* --- saturation keeps its sign --- */
    {
        NewEnc e; memset(&e, 0, sizeof e);
        int nPhase = 0;
        NewUpdate(&e, CW[0][0], CW[0][1]);
        for (int detent = 0; detent < 200; detent++)
            StepDetent(&e, NULL, &nPhase, +1, 2);
        CHECK(e.nPendingSteps == MAX_PENDING_STEPS, "saturated at %d",
              (int)e.nPendingSteps);
        CHECK(NewPendingStep(&e) == 1, "sign lost at saturation: %d",
              (int)NewPendingStep(&e));
    }

    /* --- priming swallows a boot state where A rests high --- */
    {
        NewEnc e; memset(&e, 0, sizeof e);
        NewUpdate(&e, 1, 0);
        CHECK(e.nPendingSteps == 0, "phantom step at boot: %d",
              (int)e.nPendingSteps);
    }

    /* --- the OLD implementation, on the FITTED hardware model --- */
    {
        OldEnc e; memset(&e, 0, sizeof e);
        int nPhase = 0;
        int nSteps = 0, nPhantomZero = 0;

        StepDetent(NULL, &e, &nPhase, +1, 2);       /* one detent */
        for (int i = 0; i < 3; i++)                 /* then idle polls */
            (void)OldUpdateAndPublish(&e, CW[nPhase][0], CW[nPhase][1]);

        /* count what it published */
        memset(&e, 0, sizeof e); nPhase = 0;
        for (int k = 0; k < 2; k++) {
            int prevA = CW[nPhase & 3][0];
            nPhase = (nPhase + 1) & 3;
            int v = OldUpdateAndPublish(&e, CW[nPhase][0], CW[nPhase][1]);
            (void)prevA;
            if (v != 2) { if (v == 0) nPhantomZero++; else nSteps++; }
        }
        for (int i = 0; i < 3; i++) {
            int v = OldUpdateAndPublish(&e, CW[nPhase][0], CW[nPhase][1]);
            if (v != 2) { if (v == 0) nPhantomZero++; else nSteps++; }
        }

        printf("  old impl, ONE fitted detent: %d direction + %d phantom zero\n",
               nSteps, nPhantomZero);
        CHECK(nSteps == 1, "old impl gave %d direction events per fitted detent",
              nSteps);
        CHECK(nPhantomZero >= 1, "old impl published %d phantom zeros",
              nPhantomZero);
    }

    printf("\n%d checks, %d failures\n", nChecks, nFails);
    return nFails ? 1 : 0;
}
