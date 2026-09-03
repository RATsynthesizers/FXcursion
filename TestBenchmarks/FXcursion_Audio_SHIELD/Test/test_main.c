/**
 * @file      test_main.c
 *
 * @details   Host test runner and shared fixtures.
 *
 * @copyright RAT Synthesizers
 */

#include "test_util.h"

#include "fx_protocol.h"
#include "audio_sys.h"

int         g_nChecks      = 0;
int         g_nFailures    = 0;
const char* g_pCurrentTest = "";


void Test_MakeDefaultCfg(PROTO_CFG* const pCfg, const U8 eTopology)
{
    U8 i;
    U8 j;

    (void)memset(pCfg, 0, sizeof(*pCfg));

    pCfg->nVersion     = (U8)PROTO_VERSION;
    pCfg->eTopology    = eTopology;
    pCfg->nMixerCol    = GRID_MIXER_COL_NONE;
    pCfg->bAutoGain    = (U8)FALSE;
    pCfg->nBpmX10      = 1200U;
    pCfg->nBeatsPerBar = 4U;
    pCfg->nBeatUnit    = 4U;

    for (i = 0U; i < CHAIN_MAX_QTY; i++)
    {
        for (j = 0U; j < GRID_SLOT_QTY; j++)
        {
            pCfg->aSlot[i][j] = (U8)BLOCK_NONE;
        }
        for (j = 0U; j < FXBLOCK_SLOT_QTY; j++)
        {
            pCfg->aFxSlot[i][j] = (U8)FX_TYPE_NONE;
        }
        for (j = 0U; j < CHAIN_MAX_QTY; j++)
        {
            /* Unity on the diagonal, silence elsewhere. 32768 == unity. */
            pCfg->aMixGain[i][j] = (i == j) ? 32768U : 0U;
            pCfg->aMixPan[i][j]  = 0;
        }
    }
}


int main(void)
{
    printf("\nFXcursion audio controller - host tests\n\n");

    (void)AudioSys_Init();

    printf("memory plan\n");
    Test_Memory();

    printf("shared contract\n");
    Test_Protocol();

    printf("engine\n");
    Test_Identity();
    Test_Grid();
    Test_Mixer();

    printf("effects\n");
    Test_Delay();
    Test_DelayStereo();
    Test_Compressor();
    Test_Reverb();
    Test_ReverbProfile();
    Test_Modulation();
    Test_ModProfile();

    printf("loop store\n");
    Test_LoopMem();
    Test_LoopXfer();

    printf("audio io\n");
    Test_ChanMap();
    Test_HpBus();
    Test_RecStream();
    Test_Interleave();

    printf("control link\n");
    Test_CtrlLink();

    printf("looper and tempo\n");
    Test_Tempo();
    Test_Looper();

    printf("topology\n");
    Test_Topology();

    printf("\n%d checks, %d failures\n\n", g_nChecks, g_nFailures);

    return (g_nFailures == 0) ? 0 : 1;
}
