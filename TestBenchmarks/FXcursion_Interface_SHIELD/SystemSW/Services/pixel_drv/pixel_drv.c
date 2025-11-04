/**
 * @file        pixel_drv.c
 *
 * @details     Address LED BAL module implementation
 *
 * @note        Current Led driver works through the SPI with DMA
 *
 * \version     1.0.0 - 22.01.2024 - AVV - First release
 *
 * @copyright   LLC Fly Fire
 *
 */


/***************************************************************************************************
 * Module includes
***************************************************************************************************/

// Get native header
#include "pixel_drv.h"

// Get ws2812 driver
#include "led_ws2812.h"

// Get apa102 driver
//#include "led_apa102.h"

// Get OS interface
#include "cmsis_os.h"

// Get pubsub interface
#include "pubsub.h"

/***************************************************************************************************
 * Global module constants
 **************************************************************************************************/

#define PIXEL_CMD_TOPIC_NAME                    "pixel"

// This array allows neo pixel makes linear transition between colors
const U8 PIXEL_aLedGamma[] =
{
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
    1,  2,  2,  2,  2,  2,  2,  2,  2,  3,  3,  3,  3,  3,  4,  4,
    4,  4,  4,  5,  5,  5,  5,  6,  6,  6,  6,  7,  7,  7,  7,  8,
    8,  8,  9,  9,  9, 10, 10, 10, 11, 11, 12, 12, 12, 13, 13, 14,
   14, 15, 15, 15, 16, 16, 17, 17, 18, 18, 19, 19, 20, 20, 21, 22,
   22, 23, 23, 24, 25, 25, 26, 26, 27, 28, 28, 29, 30, 30, 31, 32,
   33, 33, 34, 35, 36, 36, 37, 38, 39, 40, 40, 41, 42, 43, 44, 45,
   46, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60,
   61, 62, 63, 64, 65, 67, 68, 69, 70, 71, 72, 73, 75, 76, 77, 78,
   80, 81, 82, 83, 85, 86, 87, 89, 90, 91, 93, 94, 95, 97, 98, 99,
  101,102,104,105,107,108,110,111,113,114,116,117,119,121,122,124,
  125,127,129,130,132,134,135,137,139,141,142,144,146,148,150,151,
  153,155,157,159,161,163,165,166,168,170,172,174,176,178,180,182,
  184,186,189,191,193,195,197,199,201,204,206,208,210,212,215,217,
  219,221,224,226,228,231,233,235,238,240,243,245,248,250,253,255
};


/***************************************************************************************************
 * Local module constants
***************************************************************************************************/

// None

/***************************************************************************************************
 * Local module datatypes
***************************************************************************************************/

typedef struct stPixelAbstraction
{
    BOOLEAN             bInitialized;
    osSemaphoreId       xEffectActiveSemaphore;
    osStaticTimerDef_t  stTimerDef;
    osTimerId           hTimer;
    SUB_HANDLE          xCmdSubHandle;
    PIXEL_CMD           currentCmd;                   // Current active command
    U32                 nFrameCounter;
    STD_RESULT          (*Init)(SpiTransport* const pSpiHandle);
    STD_RESULT          (*SetLedColor)(const U16 nLedNumber, PIXEL_COLOR* const pLedColor);
    STD_RESULT          (*SetAllLedsColor)(PIXEL_COLOR* const pLedColor);
    STD_RESULT          (*MoveAllLeds)(const U8 nDirection);
    STD_RESULT          (*SendAllLedsData)(void);
} PixelAbstraction;

/***************************************************************************************************
 * Local (static) variable definitions
***************************************************************************************************/

static PixelAbstraction PIXEL;

static SpiTransport pixelSPI;

/***************************************************************************************************
 * Local (static) function declarations
***************************************************************************************************/

static STD_RESULT PIXEL_ThreadInit(void);

static void PIXEL_ThreadWrapper(void const *arg);

static void PixelTimerEventHandler(void const* pArg);

static STD_RESULT PIXEL_CmdProcessFunc(void* const pData, const U16 nDataLength);

/* Functions for continuous effects */
static void PIXEL_BlinkFrame();
static void PIXEL_PulseFrame();
static void PIXEL_RunningFrame();
static void PIXEL_TransitionFrame();
static void PIXEL_BlinkOneFrame();
static void PIXEL_PulseOneFrame();
static void PIXEL_TransitOneFrame();

/* Helper function for color interpolation */
static void PIXEL_InterpolateColor(PIXEL_COLOR* const start,
                                   PIXEL_COLOR* const end,
                                   const FLOAT32 ratio,
                                   PIXEL_COLOR* const result);

/***************************************************************************************************
 * Global function implementation
***************************************************************************************************/

/**
 * @fn      STD_RESULT pixel_initialization(void)
 * @brief   Internal Pixel driver initialization
 *
 * @param   void
 *
 * @return  STD_RESULT
 */
STD_RESULT PIXEL_Init(void)
{
    STD_RESULT eResult = RESULT_NOT_OK;

    switch(eLedHardware)
    {
    case LED_WS2812:
        PIXEL.Init              = LED_WS2812_Init;
        PIXEL.SetLedColor       = LED_WS2812_SetLedColor;
        PIXEL.SetAllLedsColor   = LED_WS2812_SetAllLedsColor;
        PIXEL.SendAllLedsData   = LED_WS2812_SendAllLedsData;
        PIXEL.MoveAllLeds       = LED_WS2812_MoveAllLeds;
        break;
    case LED_APA102:
        PIXEL.Init              = LED_APA102_Init;
        PIXEL.SetLedColor       = LED_APA102_SetLedColor;
        PIXEL.SetAllLedsColor   = LED_APA102_SetAllLedsColor;
        PIXEL.SendAllLedsData   = LED_APA102_SendAllLedsData;
        PIXEL.MoveAllLeds       = LED_APA102_MoveAllLeds;
        break;
    default:
        PRINTF_ATTR(DEBUG_COLOR_RED,
                    DEBUG_FONT_NORMAL,
                    WITHOUT_TIMESTAMP,
                    "\nPixel: Unavailable hardware selected!\n");
        return eResult;
    }

#if PIXEL_SPI_CHANNEL == 1
    pixelSPI.pSPIHandler = &hspi1;
    MX_SPI1_Init();
#elif PIXEL_SPI_CHANNEL == 2
    pixelSPI.pSPIHandler = &hspi2;
    MX_SPI2_Init();
#elif PIXEL_SPI_CHANNEL == 3
    pixelSPI.pSPIHandler = &hspi3;
    MX_SPI3_Init();
#endif

    if(RESULT_NOT_OK == PIXEL.Init(&pixelSPI))
    {
        PRINTF_ATTR(DEBUG_COLOR_RED,
                    DEBUG_FONT_NORMAL,
                    WITHOUT_TIMESTAMP,
                    "\nPixel: selected SPI%d uninitialized!\n", PIXEL_SPI_CHANNEL);
        return eResult;
    }

    PUBSUB_Init();
    if (RESULT_NOT_OK == PUBSUB_CreateTopic(PIXEL_CMD_TOPIC_NAME,
                                            sizeof(PIXEL_CMD)))
    {
        PRINTF_ATTR_SYNC(DEBUG_COLOR_RED,
                         DEBUG_FONT_NORMAL,
                         WITHOUT_TIMESTAMP,
                         "\nPixel: pubsub topic create failed!\n");
        return eResult;
    }

    /* Initialize PIXEL Thread */
    if (RESULT_NOT_OK == PIXEL_ThreadInit())
    {
        PRINTF_ATTR(DEBUG_COLOR_RED,
                    DEBUG_FONT_NORMAL,
                    WITHOUT_TIMESTAMP,
                    "\nPixel: Could not create rtos thread!\n");
        return eResult;
    }

    memset(&PIXEL.currentCmd, 0, sizeof(PIXEL.currentCmd));
    PIXEL.bInitialized              = TRUE;
    eResult                         = RESULT_OK;

    return eResult;
}

// Set idle mode
STD_RESULT PIXEL_SetAllLedsColor(PIXEL_COLOR* const pLedColor)
{
    PIXEL_CMD cmd;
    memset(&cmd, 0, sizeof(PIXEL_CMD));

    cmd.ePixelMode = PIXEL_SET_ALL;

    memcpy(&cmd.ledColor1, pLedColor, sizeof(PIXEL_COLOR));

    PUBSUB_Publish(PIXEL_CMD_TOPIC_NAME,
                   &cmd,
                   sizeof(PIXEL_CMD));

    return RESULT_OK;
}

// Set led color
STD_RESULT PIXEL_SetLedColor(PIXEL_COLOR* const pLedColor, const U16 nLedNum)
{
    PIXEL_CMD cmd;
    memset(&cmd, 0, sizeof(PIXEL_CMD));

    cmd.ePixelMode = PIXEL_SET_LED;
    cmd.nCmdParam1 = nLedNum;

    memcpy(&cmd.ledColor1, pLedColor, sizeof(PIXEL_COLOR));

    PUBSUB_Publish(PIXEL_CMD_TOPIC_NAME,
                   &cmd,
                   sizeof(PIXEL_CMD));

    return RESULT_OK;
}

// Set blink mode
STD_RESULT PIXEL_SetBlinkMode(PIXEL_COLOR* const pLedColor,
                                const U16 nOnTime,
                                const U16 nOffTime)
{
    PIXEL_CMD cmd;
    memset(&cmd, 0, sizeof(PIXEL_CMD));

    cmd.ePixelMode = PIXEL_BLINK;
    cmd.nCmdParam1 = nOnTime;
    cmd.nCmdParam2 = nOffTime;

    memcpy(&cmd.ledColor1, pLedColor, sizeof(PIXEL_COLOR));

    cmd.ledColor2.red = 0;
    cmd.ledColor2.green = 0;
    cmd.ledColor2.blue = 0;
    cmd.ledColor2.bright = 0;

    PUBSUB_Publish(PIXEL_CMD_TOPIC_NAME,
                   &cmd,
                   sizeof(PIXEL_CMD));

    return RESULT_OK;
}

// Set pulse mode
STD_RESULT PIXEL_SetPulseMode(PIXEL_COLOR* const pLedColor,
                                const U16 nRiseTime,
                                const U16 nFallTime,
                                const U16 nFrameDelay)
{

    PIXEL_CMD cmd;
    memset(&cmd, 0, sizeof(PIXEL_CMD));

    cmd.ePixelMode = PIXEL_PULSE;
    cmd.nCmdParam1 = nRiseTime; // time to rise
    cmd.nCmdParam2 = nFallTime; // time to fall
    cmd.nCmdParam3 = nFrameDelay; // Frame delay

    memcpy(&cmd.ledColor1, pLedColor, sizeof(PIXEL_COLOR));

    cmd.ledColor2.red = 0;
    cmd.ledColor2.green = 0;
    cmd.ledColor2.blue = 0;
    cmd.ledColor2.bright = 0;

    PUBSUB_Publish(PIXEL_CMD_TOPIC_NAME,
                   &cmd,
                   sizeof(PIXEL_CMD));

    return RESULT_OK;
}

// Set running module
STD_RESULT PIXEL_SetRunningMode(PIXEL_COLOR* const pLineColor,
                                const U16 nLinesNumber,
                                const U16 nLinesLength,
                                const U16 nLinesDistance,
                                const U16 bDirection,
                                const U16 nFrameDelay)
{
    PIXEL_CMD cmd;
    memset(&cmd, 0, sizeof(PIXEL_CMD));

    cmd.ePixelMode = PIXEL_RUNNING;
    cmd.nCmdParam1 = nLinesNumber;
    cmd.nCmdParam2 = nLinesLength;
    cmd.nCmdParam3 = nLinesDistance; // Distance between lines
    cmd.nCmdParam4 = bDirection; // Movement direction right
    cmd.nCmdParam5 = nFrameDelay; // Frame delay

    memcpy(&cmd.ledColor1, pLineColor, sizeof(PIXEL_COLOR));

    cmd.ledColor2.red = 0;
    cmd.ledColor2.green = 0;
    cmd.ledColor2.blue = 0;
    cmd.ledColor2.bright = 0;


    PUBSUB_Publish(PIXEL_CMD_TOPIC_NAME,
                   &cmd,
                   sizeof(PIXEL_CMD));

    return RESULT_OK;
}

// Set transit module
STD_RESULT PIXEL_SetTransitMode(PIXEL_COLOR* const pBaseColor,
                                PIXEL_COLOR* const pNewColor,
                                const U16 nTransitionTime,
                                const U16 nFrameDelay)
{
    PIXEL_CMD cmd;
    memset(&cmd, 0, sizeof(PIXEL_CMD));

    cmd.ePixelMode = PIXEL_TRANSITION;
    memcpy(&cmd.ledColor1, pBaseColor, sizeof(PIXEL_COLOR));
    memcpy(&cmd.ledColor2, pNewColor, sizeof(PIXEL_COLOR));
    cmd.nCmdParam1 = nTransitionTime;
    cmd.nCmdParam2 = nFrameDelay;

    PUBSUB_Publish(PIXEL_CMD_TOPIC_NAME,
                   &cmd,
                   sizeof(PIXEL_CMD));

    return RESULT_OK;
}


// Set blink one mode
STD_RESULT PIXEL_SetBlinkOneMode(PIXEL_COLOR* const pLedColor,
                                    const U16 nOnTime,
                                    const U16 nOffTime,
                                    const U16 nLedNum)
{
    PIXEL_CMD cmd;
    memset(&cmd, 0, sizeof(PIXEL_CMD));

    cmd.ePixelMode = PIXEL_BLINK_ONE;
    cmd.nCmdParam1 = nLedNum;
    cmd.nCmdParam2 = nOnTime;
    cmd.nCmdParam3 = nOffTime;

    memcpy(&cmd.ledColor1, pLedColor, sizeof(PIXEL_COLOR));

    cmd.ledColor2.red = 0;
    cmd.ledColor2.green = 0;
    cmd.ledColor2.blue = 0;
    cmd.ledColor2.bright = 0;


    PUBSUB_Publish(PIXEL_CMD_TOPIC_NAME,
                   &cmd,
                   sizeof(PIXEL_CMD));

    return RESULT_OK;
}

// Set pulse one module
STD_RESULT PIXEL_SetPulseOneMode(PIXEL_COLOR* const pLedColor,
                                    const U16 nRiseTime,
                                    const U16 nFallTime,
                                    const U16 nFrameDelay,
                                    const U16 nLedNum)
{
    PIXEL_CMD cmd;
    memset(&cmd, 0, sizeof(PIXEL_CMD));

    cmd.ePixelMode = PIXEL_PULSE_ONE;
    cmd.nCmdParam1 = nLedNum;
    cmd.nCmdParam2 = nRiseTime;
    cmd.nCmdParam3 = nFallTime;
    cmd.nCmdParam4 = nFrameDelay;

    memcpy(&cmd.ledColor1, pLedColor, sizeof(PIXEL_COLOR));

    cmd.ledColor2.red = 0;
    cmd.ledColor2.green = 0;
    cmd.ledColor2.blue = 0;
    cmd.ledColor2.bright = 0;

    PUBSUB_Publish(PIXEL_CMD_TOPIC_NAME,
                   &cmd,
                   sizeof(PIXEL_CMD));

    return RESULT_OK;
}

// Set transit one module
STD_RESULT PIXEL_SetTransitOneMode(PIXEL_COLOR* const pBaseColor,
                                    PIXEL_COLOR* const pNewColor,
                                    const U16 nTransitionTime,
                                    const U16 nFrameDelay,
                                    const U16 nLedNum)
{
    PIXEL_CMD cmd;
    memset(&cmd, 0, sizeof(PIXEL_CMD));

    cmd.ePixelMode = PIXEL_TRANSIT_ONE;
    memcpy(&cmd.ledColor1, pBaseColor, sizeof(PIXEL_COLOR));
    memcpy(&cmd.ledColor2, pNewColor, sizeof(PIXEL_COLOR));
    cmd.nCmdParam1 = nLedNum;
    cmd.nCmdParam2 = nTransitionTime;
    cmd.nCmdParam3 = nFrameDelay;

    PUBSUB_Publish(PIXEL_CMD_TOPIC_NAME,
                   &cmd,
                   sizeof(PIXEL_CMD));

    return RESULT_OK;
}


static STD_RESULT PIXEL_ThreadInit(void)
{
    char strThreadName[configMAX_TASK_NAME_LEN];

    /* Create Rx Thread */
    strThreadName[0U] = 'P';
    strThreadName[1U] = 'i';
    strThreadName[2U] = 'x';
    strThreadName[3U] = 'e';
    strThreadName[4U] = 'l';
    strThreadName[5U] = 0U;

    /* Create synchronization primitives */

    osTimerStaticDef(pixel_timer,
                     PixelTimerEventHandler,
                     &PIXEL.stTimerDef);
    PIXEL.hTimer = osTimerCreate(osTimer(pixel_timer),
                                 osTimerOnce,
                                 NULL);

    osSemaphoreDef(PixelCmdSemaphore);
    PIXEL.xEffectActiveSemaphore = osSemaphoreCreate(osSemaphore(PixelCmdSemaphore), 1);
    osSemaphoreWait(PIXEL.xEffectActiveSemaphore, 0);

    if (!PIXEL.hTimer || !PIXEL.xEffectActiveSemaphore)
    {
        return RESULT_NOT_OK;
    }

    /* Create the thread */
    osThreadDef_t ThreadDef =
    {
        .name = strThreadName,
        .pthread = PIXEL_ThreadWrapper,
        .tpriority = osPriorityNormal,
        .instances = 0,
        .stacksize = 300U
    };

    if (NULL == osThreadCreate(&ThreadDef, NULL_PTR))
    {
        return RESULT_NOT_OK;
    }

    return RESULT_OK;
}

static void PIXEL_ThreadWrapper(void const *arg)
{

    PIXEL.xCmdSubHandle = PUBSUB_Subscribe(PIXEL_CMD_TOPIC_NAME, PIXEL_CmdProcessFunc);

    PIXEL_COLOR off_leds_color     = {0, 0, 0, 0};

#ifdef PIXEL_LEDS_INITIAL_BRIGHTNESS
    U8 nBrightness = PIXEL_LEDS_INITIAL_BRIGHTNESS;
#else
    U8 nBrightness = 0;
#endif

    PIXEL_COLOR initial_leds_color = {PIXEL_LEDS_INITIAL_RED_COLOR,
                                      PIXEL_LEDS_INITIAL_GREEN_COLOR,
                                      PIXEL_LEDS_INITIAL_BLUE_COLOR,
                                      nBrightness};
    PIXEL.SetAllLedsColor(&off_leds_color);
    PIXEL.SendAllLedsData();
    osDelay(50);
    PIXEL.SetAllLedsColor(&initial_leds_color);
    PIXEL.SendAllLedsData();

    for (;;)
    {
        /* Wait for command signal */
        osSemaphoreWait(PIXEL.xEffectActiveSemaphore, osWaitForever);

        PIXEL_CMD newPixelCmd;

        if(RESULT_OK == PUBSUB_Update(PIXEL.xCmdSubHandle,
                                       &newPixelCmd,
                                       sizeof(newPixelCmd),
                                       0))
        {
            switch (newPixelCmd.ePixelMode)
            {
            case PIXEL_SET_ALL:
                /* Execute immediate command */
                PIXEL.SetAllLedsColor(&newPixelCmd.ledColor1);
                /* Stop executing any continuous mode*/
                memcpy(&PIXEL.currentCmd, &newPixelCmd, sizeof(PIXEL_CMD));
                break;

            case PIXEL_SET_LED:

                /* Execute immediate command */
                /* pCmd->nCmdParam1 contains nLedNum value - which led color to change */
                PIXEL.SetLedColor(newPixelCmd.nCmdParam1 /*nLedNum*/, &newPixelCmd.ledColor1);

                // Individual continuous modes
                if((PIXEL_TRANSIT_ONE == PIXEL.currentCmd.ePixelMode
                        || PIXEL_BLINK_ONE == PIXEL.currentCmd.ePixelMode
                        || PIXEL_PULSE_ONE == PIXEL.currentCmd.ePixelMode)
                        && PIXEL.currentCmd.nCmdParam1 == newPixelCmd.nCmdParam1 /*nLedNum == nLedNum*/)
                {
                    /*
                     * Stop executing current continuous mode
                     * if LED numbers of new and old commands are equal
                     * */
                    memcpy(&PIXEL.currentCmd, &newPixelCmd, sizeof(PIXEL_CMD));
                }
                break;

            // One of continuous modes
            case PIXEL_RUNNING:
            case PIXEL_BLINK:
            case PIXEL_PULSE:
            case PIXEL_TRANSITION:
            case PIXEL_BLINK_ONE:
            case PIXEL_PULSE_ONE:
            case PIXEL_TRANSIT_ONE:

                memcpy(&PIXEL.currentCmd, &newPixelCmd, sizeof(PIXEL_CMD));
                PIXEL.nFrameCounter = 0;
                break;

            default:
                PRINTF_ATTR(DEBUG_COLOR_RED,
                            DEBUG_FONT_NORMAL,
                            WITHOUT_TIMESTAMP,
                            "\nPixel: Unknown command %d received!\n", newPixelCmd.ePixelMode);
                break;
            }
        }

        /* Execute one frame of current effect */
        switch (PIXEL.currentCmd.ePixelMode)
        {
            case PIXEL_BLINK:
                PIXEL_BlinkFrame();
                break;
            case PIXEL_PULSE:
                PIXEL_PulseFrame();
                break;
            case PIXEL_RUNNING:
                PIXEL_RunningFrame();
                break;
            case PIXEL_TRANSITION:
                PIXEL_TransitionFrame();
                break;
            case PIXEL_BLINK_ONE:
                PIXEL_BlinkOneFrame();
                break;
            case PIXEL_PULSE_ONE:
                PIXEL_PulseOneFrame();
                break;
            case PIXEL_TRANSIT_ONE:
                PIXEL_TransitOneFrame();
                break;
            default:
                /* Static led command executed */
                break;
        }

        /* Update LEDs */
        PIXEL.SendAllLedsData();
    }
}

static void PixelTimerEventHandler(void const* pArg)
{
    osSemaphoreRelease(PIXEL.xEffectActiveSemaphore);
}

static STD_RESULT PIXEL_CmdProcessFunc(void* const pData, const U16 nDataLength)
{
    osTimerStart(PIXEL.hTimer, 0);

    return RESULT_OK;
}

/* Running effect frame processing */
static void PIXEL_BlinkFrame()
{
    /* Determine which state we're in and time thresholds */
    U16 nOnTime = PIXEL.currentCmd.nCmdParam1;        // Time for color1 (ms)
    U16 nOffTime = PIXEL.currentCmd.nCmdParam2;        // Time for color2 (ms)

    if(0 == PIXEL.nFrameCounter % 2)
    {
        // Show color1
        PIXEL.SetAllLedsColor(&PIXEL.currentCmd.ledColor1);

        PIXEL.nFrameCounter++;
        // Wait for next frame
        osTimerStart(PIXEL.hTimer, nOnTime);
    }
    else
    {
        // Show color2
        PIXEL.SetAllLedsColor(&PIXEL.currentCmd.ledColor2);

        PIXEL.nFrameCounter++;
        // Wait for next frame
        osTimerStart(PIXEL.hTimer, nOffTime);
    }
}

/* Running effect frame processing */
static void PIXEL_PulseFrame()
{
    /* Get pulse parameters */
    U16 nRiseTime = PIXEL.currentCmd.nCmdParam1;       // Time to rise from color1 to color2 (ms)
    U16 nFallTime = PIXEL.currentCmd.nCmdParam2;       // Time to fall back to color1 (ms)
    U16 nPulseDuration = nRiseTime + nFallTime;  // Total pulse duration

    U16 nFrameDelay = PIXEL.currentCmd.nCmdParam3;

    /* Calculate position in current pulse */
    U32 nTimeInPulse = PIXEL.nFrameCounter * nFrameDelay;
    U16 nPulsePosition = nTimeInPulse % nPulseDuration;

    /* Calculate interpolation ratio */
    FLOAT32 fRatio = 0.0F;
    PIXEL_COLOR currentColor;

    if (nPulsePosition < nRiseTime)
    {
        // Rising phase (color1 -> color2)
        fRatio = (FLOAT32)nPulsePosition / (FLOAT32)nRiseTime;
        PIXEL_InterpolateColor(&PIXEL.currentCmd.ledColor1, &PIXEL.currentCmd.ledColor2, fRatio, &currentColor);
    }
    else
    {
        // Falling phase (color2 -> color1)
        fRatio = (FLOAT32)(nPulsePosition - nRiseTime) / (FLOAT32)nFallTime;
        PIXEL_InterpolateColor(&PIXEL.currentCmd.ledColor2, &PIXEL.currentCmd.ledColor1, fRatio, &currentColor);
    }

    /* Apply color to all LEDs */
    PIXEL.SetAllLedsColor(&currentColor);

    PIXEL.nFrameCounter++;
    // Wait for next frame
    osTimerStart(PIXEL.hTimer, nFrameDelay);
}

/* Running effect frame processing */
static void PIXEL_RunningFrame()
{
    const U8 nLinesNum = PIXEL.currentCmd.nCmdParam1;
    const U8 nLinesLength = PIXEL.currentCmd.nCmdParam2;
    const U8 nLinesDistance = PIXEL.currentCmd.nCmdParam3;
    const U16 nSegmentLength = nLinesLength + nLinesDistance;

    /* Get movement direction from command parameters */
    const U8 nDirection = PIXEL.currentCmd.nCmdParam4;

    /* Get delay between frames in ms */
    const U16 nFrameDelay = PIXEL.currentCmd.nCmdParam5;

    if(0 == PIXEL.nFrameCounter)
    {
        PIXEL.SetAllLedsColor(&PIXEL.currentCmd.ledColor2);

        /* Create foreground lines */
        for (U8 lineIdx = 0; lineIdx < nLinesNum; lineIdx++)
        {
            const U16 startPos = lineIdx * nSegmentLength;

            for (U8 posInLine = 0; posInLine < nLinesLength; posInLine++)
            {
                const U16 ledIdx = startPos + posInLine;

                if (ledIdx < PIXEL_TOTAL_LED_QUANTITY)
                {
                    PIXEL.SetLedColor(ledIdx, &PIXEL.currentCmd.ledColor1);
                }
            }
        }
    }
    else
    {
        /* Move all LEDs in specified direction */
        PIXEL.MoveAllLeds(nDirection);
    }

    PIXEL.nFrameCounter++;
    // Wait for next frame
    osTimerStart(PIXEL.hTimer, nFrameDelay);
}

/* Running effect frame processing */
static void PIXEL_TransitionFrame()
{

    /* Get transition parameters */
    U16 nTransitionTime = PIXEL.currentCmd.nCmdParam1;  // Total transition time (ms)
    U16 nFrameDelay = PIXEL.currentCmd.nCmdParam2;

    /* Calculate current progress */
    U32 nElapsedTime = PIXEL.nFrameCounter * nFrameDelay;
    FLOAT32 fRatio = (FLOAT32)nElapsedTime / (FLOAT32)nTransitionTime;

    /* Clamp ratio to 1.0 */
    if (fRatio > 1.0f)
    {
        fRatio = 1.0f;
    }

    /* Interpolate color */
    PIXEL_COLOR currentColor;
    PIXEL_InterpolateColor(&PIXEL.currentCmd.ledColor1, &PIXEL.currentCmd.ledColor2, fRatio, &currentColor);

    /* Apply color to all LEDs */
    PIXEL.SetAllLedsColor(&currentColor);

    PIXEL.nFrameCounter++;

    /* Clamp ratio to 1.0 */
    if (fRatio < 1.0f)
    {
        // Wait for next frame
        osTimerStart(PIXEL.hTimer, nFrameDelay);
    }
}

/* Transit one pixel frame processing */
static void PIXEL_BlinkOneFrame()
{
    /* Determine which state we're in and time thresholds */
    U16 nLedNum = PIXEL.currentCmd.nCmdParam1;
    U16 nOnTime = PIXEL.currentCmd.nCmdParam2;        // Time for color1 (ms)
    U16 nOffTime = PIXEL.currentCmd.nCmdParam3;        // Time for color2 (ms)

    if(0 == PIXEL.nFrameCounter % 2)
    {
        // Show color1
        PIXEL.SetLedColor(nLedNum, &PIXEL.currentCmd.ledColor1);

        PIXEL.nFrameCounter++;
        // Wait for next frame
        osTimerStart(PIXEL.hTimer, nOnTime);
    }
    else
    {
        // Show color2
        PIXEL.SetLedColor(nLedNum, &PIXEL.currentCmd.ledColor2);

        PIXEL.nFrameCounter++;
        // Wait for next frame
        osTimerStart(PIXEL.hTimer, nOffTime);
    }
}

/* Transit one pixel frame processing */
static void PIXEL_PulseOneFrame()
{
    U16 nLedNum = PIXEL.currentCmd.nCmdParam1;

    /* Get pulse parameters */
    U16 nRiseTime = PIXEL.currentCmd.nCmdParam2;       // Time to rise from color1 to color2 (ms)
    U16 nFallTime = PIXEL.currentCmd.nCmdParam3;       // Time to fall back to color1 (ms)
    U16 nPulseDuration = nRiseTime + nFallTime;  // Total pulse duration

    U16 nFrameDelay = PIXEL.currentCmd.nCmdParam4;


    /* Calculate position in current pulse */
    U32 nTimeInPulse = PIXEL.nFrameCounter * nFrameDelay;
    U16 nPulsePosition = nTimeInPulse % nPulseDuration;

    /* Calculate interpolation ratio */
    FLOAT32 fRatio = 0.0F;
    PIXEL_COLOR currentColor;
    PIXEL_COLOR colorOff = {0,0,0};

    if (nPulsePosition < nRiseTime)
    {
        // Rising phase (color1 -> color2)
        fRatio = (FLOAT32)nPulsePosition / (FLOAT32)nRiseTime;
        PIXEL_InterpolateColor(&PIXEL.currentCmd.ledColor1, &colorOff, fRatio, &currentColor);
    }
    else
    {
        // Falling phase (color2 -> color1)
        fRatio = (FLOAT32)(nPulsePosition - nRiseTime) / (FLOAT32)nFallTime;
        PIXEL_InterpolateColor(&colorOff, &PIXEL.currentCmd.ledColor1, fRatio, &currentColor);
    }

    /* Apply color to all LEDs */
    PIXEL.SetLedColor(nLedNum, &currentColor);

    PIXEL.nFrameCounter++;
    // Wait for next frame
    osTimerStart(PIXEL.hTimer, nFrameDelay);
}

/* Transit one pixel frame processing */
static void PIXEL_TransitOneFrame()
{

    U16 nLedNum = PIXEL.currentCmd.nCmdParam1;
    /* Get transition parameters */
    U16 nTransitionTime = PIXEL.currentCmd.nCmdParam2;  // Total transition time (ms)
    U16 nFrameDelay = PIXEL.currentCmd.nCmdParam3;

    /* Calculate current progress */
    U32 nElapsedTime = PIXEL.nFrameCounter * nFrameDelay;
    FLOAT32 fRatio = (FLOAT32)nElapsedTime / (FLOAT32)nTransitionTime;

    /* Clamp ratio to 1.0 */
    if (fRatio > 1.0f)
    {
        fRatio = 1.0f;
    }

    /* Interpolate color */
    PIXEL_COLOR currentColor;
    PIXEL_InterpolateColor(&PIXEL.currentCmd.ledColor1, &PIXEL.currentCmd.ledColor2, fRatio, &currentColor);

    /* Apply color to all LEDs */
    PIXEL.SetLedColor(nLedNum, &currentColor);

    PIXEL.nFrameCounter++;

    /* Clamp ratio to 1.0 */
    if (fRatio < 1.0f)
    {
        // Wait for next frame
        osTimerStart(PIXEL.hTimer, nFrameDelay);
    }
}

/* Helper function for color interpolation */
static void PIXEL_InterpolateColor(PIXEL_COLOR* const start,
                                   PIXEL_COLOR* const end,
                                   const FLOAT32 ratio,
                                   PIXEL_COLOR* const result)
{
    result->red = (U8)(start->red + (end->red - start->red) * ratio);
    result->green = (U8)(start->green + (end->green - start->green) * ratio);
    result->blue = (U8)(start->blue + (end->blue - start->blue) * ratio);
}

