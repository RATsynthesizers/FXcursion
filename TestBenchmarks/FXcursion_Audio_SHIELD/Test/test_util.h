/**
 * @file      test_util.h
 *
 * @details   Assert-based test runner. No framework on purpose: it has to build
 *            with the same compiler settings as the firmware, and a MISRA-checked
 *            project does not want a test framework dragged into its include path.
 *
 * @copyright RAT Synthesizers
 */

#ifndef TEST_UTIL_H
#define TEST_UTIL_H

#include "general.h"
#include "audio_cfg.h"
#include "fx_protocol.h"

#include <stdio.h>
#include <math.h>

extern int g_nChecks;
extern int g_nFailures;
extern const char* g_pCurrentTest;

#define TEST_BEGIN(name)                                                        \
    do {                                                                        \
        g_pCurrentTest = (name);                                                \
        printf("  %-46s", (name));                                              \
        fflush(stdout);                                                         \
    } while (0)

#define TEST_END()                                                              \
    do {                                                                        \
        printf("ok\n");                                                         \
    } while (0)

#define CHECK(cond)                                                             \
    do {                                                                        \
        g_nChecks++;                                                            \
        if (!(cond)) {                                                          \
            g_nFailures++;                                                      \
            printf("FAIL\n    %s:%d  %s\n", __FILE__, __LINE__, #cond);         \
        }                                                                       \
    } while (0)

#define CHECK_EQ_U32(got, want)                                                 \
    do {                                                                        \
        g_nChecks++;                                                            \
        if ((unsigned long)(got) != (unsigned long)(want)) {                    \
            g_nFailures++;                                                      \
            printf("FAIL\n    %s:%d  %s: got %lu want %lu\n",                   \
                   __FILE__, __LINE__, #got,                                    \
                   (unsigned long)(got), (unsigned long)(want));                \
        }                                                                       \
    } while (0)

#define CHECK_NEAR(got, want, tol)                                              \
    do {                                                                        \
        g_nChecks++;                                                            \
        if (fabs((double)(got) - (double)(want)) > (double)(tol)) {             \
            g_nFailures++;                                                      \
            printf("FAIL\n    %s:%d  %s: got %.9g want %.9g tol %.9g\n",        \
                   __FILE__, __LINE__, #got,                                    \
                   (double)(got), (double)(want), (double)(tol));               \
        }                                                                       \
    } while (0)

/* Test entry points. */
extern void Test_Protocol(void);
extern void Test_Frame(void);
/** Build a valid, empty configuration for the given topology. */
extern void Test_MakeDefaultCfg(PROTO_CFG* const pCfg, const U8 eTopology);

extern void Test_Memory(void);
extern void Test_Identity(void);
extern void Test_Grid(void);
extern void Test_Mixer(void);
extern void Test_Delay(void);
extern void Test_DelayStereo(void);
extern void Test_Looper(void);
extern void Test_Tempo(void);
extern void Test_Topology(void);
extern void Test_Compressor(void);
extern void Test_Reverb(void);
extern void Test_ReverbProfile(void);
extern void Test_Modulation(void);
extern void Test_ModProfile(void);
extern void Test_LoopMem(void);
extern void Test_LoopXfer(void);
extern void Test_ChanMap(void);
extern void Test_HpBus(void);
extern void Test_RecStream(void);
extern void Test_Interleave(void);
extern void Test_CtrlLink(void);

#endif // #ifndef TEST_UTIL_H
