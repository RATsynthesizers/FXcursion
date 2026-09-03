/**
 * @file        pubsub.c
 *
 * @details     Implementation of the publisher-subscriber service functionality.
 *
 * \version     1.0.0 - 31.01.2024 - AVV - First release
 */



/***************************************************************************************************
* Module includes
***************************************************************************************************/

// Native header
#include "pubsub.h"

// Get configuration parameters
#include "pubsub_cfg.h"

/* Definitions for memory and string functions */
#include <string.h>

// Get Os interface
#include "cmsis_os.h"



//**************************************************************************************************
// Verification of the imported configuration parameters
//**************************************************************************************************

#if ((TOPIC_NAME_MAX_SIZE > 50U) || (TOPIC_NAME_MAX_SIZE == 0U))
#error "Wrong program configuration: TOPIC_NAME_MAX_SIZE must be in the range [1 ; 50]"
#endif // #if ((TOPIC_NAME_MAX_SIZE > 50U) || (TOPIC_NAME_MAX_SIZE == 0U))

#if ((TOPIC_SUBSCRIBERS_MAX_QTY > 5U) || (TOPIC_SUBSCRIBERS_MAX_QTY == 0U))
#error "Wrong program configuration: TOPIC_SUBSCRIBERS_MAX_QTY must be in the range [1 ; 5]"
#endif // ((TOPIC_SUBSCRIBERS_MAX_QTY > 0U) || (TOPIC_SUBSCRIBERS_MAX_QTY == 0U))

#if ((TOPIC_QUEUE_DEPTH > 64U) || (TOPIC_QUEUE_DEPTH == 0U))
#error "Wrong program configuration: TOPIC_QUEUE_DEPTH must be in the range [1 ; 64]"
#endif // ((TOPIC_QUEUE_DEPTH > 64U) || (TOPIC_QUEUE_DEPTH == 0U))



/***************************************************************************************************
* Definitions of global (public) variables
***************************************************************************************************/

// None.



/***************************************************************************************************
* Definitions of module constants
***************************************************************************************************/





/***************************************************************************************************
* Declarations of local (private) data types
***************************************************************************************************/

// Topic subscriber type
typedef struct stSUBSCRIBER
{
    /** Flag to show that subscriber is already used */
    BOOLEAN bIsUsed;

    /* OS queue Id for receive messages */
    osMailQId  xMsgQueueId;

    /* OS queue control block */
    struct os_mailQ_cb* os_mailQ_cb;


    osMailQDef_t os_mailQ_def;

    /* Notify function */
    PUBSUB_NOTIFY_FUNC pNotifyFunc;
} SUBSCRIBER;

// Topics table type
typedef struct stTOPIC
{
    /** Flag to show that topic is already used */
    BOOLEAN bIsUsed;

    /** Name */
    U8 aName[TOPIC_NAME_MAX_SIZE];

    /** Name length */
    U8 nNameLength;

    /** Data length */
    U8 nDataLength;

    /* Publish counter */
    U32 nPublishCounter;

    /** Subscribers array */
    SUBSCRIBER aSubscribers[TOPIC_SUBSCRIBERS_MAX_QTY];

} TOPIC;

typedef struct stPUBSUB
{
    BOOLEAN bInitialized;
    TOPIC   aTopics[TOPICS_MAX_QTY];

} PUBSUB;



/***************************************************************************************************
* Definitions of static global (private) variables
***************************************************************************************************/

// Module initializing flag
static PUBSUB PubSub;



/***************************************************************************************************
* Declarations of local (private) functions
***************************************************************************************************/

/// None.



/***************************************************************************************************
* Definitions of global (public) functions
***************************************************************************************************/

/**
 * @fn    STD_RESULT UGREEN_Init(void)
 *
 * @brief Initalize service.
 * 
 * @param[in] None.
 *
 * @return    Result of the function execution. See STD_RESULT type.
 */
STD_RESULT PUBSUB_Init(void)
{
    STD_RESULT eResult = RESULT_NOT_OK;

    if (FALSE == PubSub.bInitialized)
    {
        PubSub.bInitialized = TRUE;
        eResult = RESULT_OK;
    }

    return eResult;
}



/**
 * @fn    STD_RESULT PUBSUB_CreateTopic(char* const pTopicName,
                                        const U16 nTopicDataLength);
 *
 * @brief Creates the topic.
 *
 * @param[in] pTopicName - pointer to topic name.
 * @param[in] nTopicDataLength - topic data length in bytes.
 *
 * @return    Result of the function execution. See STD_RESULT type.
 */
STD_RESULT PUBSUB_CreateTopic(char* const pTopicName,
                              const U16 nTopicDataLength)
{
    STD_RESULT eResult = RESULT_NOT_OK;
    BOOLEAN bIsFreePlaceFound = FALSE;

    if (TRUE == PubSub.bInitialized)
    {
        if (NULL_PTR != pTopicName)
        {
            for (U8 nTopicNum = 0U; nTopicNum < TOPICS_MAX_QTY; nTopicNum++)
            {
                if (FALSE == PubSub.aTopics[nTopicNum].bIsUsed)
                {
                    U8 nTopicNameLength = strlen(pTopicName);
                    if (nTopicNameLength <= TOPIC_NAME_MAX_SIZE)
                    {
                        PubSub.aTopics[nTopicNum].bIsUsed = TRUE;

                        for (U8 nIterator = 0U; nIterator < nTopicNameLength; nIterator++)
                        {
                            PubSub.aTopics[nTopicNum].aName[nIterator] = pTopicName[nIterator];
                        }

                        PubSub.aTopics[nTopicNum].nNameLength = nTopicNameLength;
                        PubSub.aTopics[nTopicNum].nDataLength = nTopicDataLength;

                        eResult = RESULT_OK;
                        bIsFreePlaceFound = TRUE;
                    }
                    break;
                }
            }

            ASSERT(bIsFreePlaceFound);
        }
    }

    return eResult;
}



/**
 * @fn    STD_RESULT PUBSUB_Subscribe(char* const pTopicName,
                                      const PUBSUB_NOTIFY_FUNC pNotifyFunc);
 *
 * @brief Subscribes to the topic.
 * @note
 *
 * @param[in] pTopicName - pointer to topic name.
 * @param[in] pNotifyFunc - pointer to the subscriber notify function.
 *
 * @return    Result of the function execution. See STD_RESULT type.
 */
SUB_HANDLE PUBSUB_Subscribe(char* const pTopicName,
                            const PUBSUB_NOTIFY_FUNC pNotifyFunc)
{
    SUB_HANDLE handle = NULL_PTR;

    if (TRUE == PubSub.bInitialized)
    {
        if (NULL_PTR != pTopicName)
        {
            /* Find the topic */
            for (U8 nTopicNum = 0U; nTopicNum < TOPICS_MAX_QTY; nTopicNum++)
            {
                if (TRUE == PubSub.aTopics[nTopicNum].bIsUsed)
                {
                    if (0 == strncmp((char*)pTopicName,
                                     (char*)PubSub.aTopics[nTopicNum].aName,
                                     (size_t)PubSub.aTopics[nTopicNum].nNameLength))
                    {
                        /* Try to subscribe to this topic */
                        for (U8 nSubscriberNum = 0U;
                                nSubscriberNum < TOPIC_SUBSCRIBERS_MAX_QTY;
                                nSubscriberNum++)
                        {
                            if (FALSE == PubSub.aTopics[nTopicNum].aSubscribers[nSubscriberNum].bIsUsed)
                            {
                                PubSub.aTopics[nTopicNum].aSubscribers[nSubscriberNum].bIsUsed = TRUE;
                                PubSub.aTopics[nTopicNum].aSubscribers[nSubscriberNum].pNotifyFunc =
                                        pNotifyFunc;
                                PubSub.aTopics[nTopicNum].aSubscribers[nSubscriberNum].os_mailQ_def.queue_sz = TOPIC_QUEUE_DEPTH;
                                PubSub.aTopics[nTopicNum].aSubscribers[nSubscriberNum].os_mailQ_def.item_sz =
                                        PubSub.aTopics[nTopicNum].nDataLength;
                                PubSub.aTopics[nTopicNum].aSubscribers[nSubscriberNum].os_mailQ_def.cb =
                                        &PubSub.aTopics[nTopicNum].aSubscribers[nSubscriberNum].os_mailQ_cb;

                                PubSub.aTopics[nTopicNum].aSubscribers[nSubscriberNum].xMsgQueueId =
                                        osMailCreate(&PubSub.aTopics[nTopicNum].aSubscribers[nSubscriberNum].os_mailQ_def, NULL);
                                if (NULL != PubSub.aTopics[nTopicNum].aSubscribers[nSubscriberNum].xMsgQueueId)
                                {
                                    handle = PubSub.aTopics[nTopicNum].aSubscribers[nSubscriberNum].xMsgQueueId;
                                }
                                break;
                            }
                        }
                        break;
                    }
                }
            }
        }
    }
    return handle;
}



/**
 * @fn    STD_RESULT PUBSUB_Subscribe(char* const pTopicName,
                                      const PUBSUB_NOTIFY_FUNC pNotifyFunc);
 *
 * @brief Subscribes to the topic.
 * @note
 *
 * @param[in] pTopicName - pointer to topic name.
 * @param[in] pData - pointer to the message data.
 * @param[in] nDataLength - message data length.
 *
 * @return    Result of the function execution. See STD_RESULT type.
 */
STD_RESULT PUBSUB_Publish(char* const pTopicName,
                          void* const pData,
                          const U16 nDataLength)
{
    STD_RESULT eResult = RESULT_NOT_OK;

    if ((NULL_PTR != pTopicName) && (NULL_PTR != pData))
    {
        /* Find the topic */
        for (U8 nTopicNum = 0U; nTopicNum < TOPICS_MAX_QTY; nTopicNum++)
        {
            if (TRUE == PubSub.aTopics[nTopicNum].bIsUsed)
            {
                if (0 == strncmp((char*)pTopicName,
                                 (char*)PubSub.aTopics[nTopicNum].aName,
                                 (size_t)PubSub.aTopics[nTopicNum].nNameLength))
                {
                    /* Send message to all subscribers to this topic */
                    for (U8 nSubscriberNum = 0U;
                            nSubscriberNum < TOPIC_SUBSCRIBERS_MAX_QTY;
                            nSubscriberNum++)
                    {
                        if (TRUE == PubSub.aTopics[nTopicNum].aSubscribers[nSubscriberNum].bIsUsed)
                        {
                            /* Send message to the corresponding queue */
                            osMailQId xMsgQueueId =
                                    PubSub.aTopics[nTopicNum].aSubscribers[nSubscriberNum].xMsgQueueId;
                            if (NULL != xMsgQueueId)
                            {
                                /* Put message to queue */
                                void* pMsgQueueItem = osMailAlloc(xMsgQueueId,
                                                                  0);
                                if (NULL_PTR != pMsgQueueItem)
                                {
                                    memcpy(pMsgQueueItem,
                                           pData,
                                           nDataLength);
                                    osMailPut(xMsgQueueId,
                                              pMsgQueueItem);
                                    PubSub.aTopics[nTopicNum].nPublishCounter ++;
                                    eResult = RESULT_OK;
                                }
                            }

                            /* Call notification function */
                            if (NULL_PTR != PubSub.aTopics[nTopicNum].aSubscribers[nSubscriberNum].pNotifyFunc)
                            {
                                PubSub.aTopics[nTopicNum].aSubscribers[nSubscriberNum].pNotifyFunc(pData,
                                                                                                   nDataLength);
                            }
                        }
                    }
                }
            }
        }
    }
    return eResult;
}



/**
 * @fn    STD_RESULT PUBSUB_Update(const SUB_HANDLE pSubscriberHandle,
                                   void* const pDataBuf,
                                   const U16 nDataBufferLength,
                                   const U32 nTimeout);
 *
 * @brief Receive the data from topic.
 * @note
 *
 * @param[in] pSubscriberHandle - subscriber handle.
 * @param[in] pDataBuf - pointer to the data buffer.
 * @param[in] nDataBufferLength - size of the data buffer.
 * @param[in] nTimeout - receive timeout.
 *
 * @return    Result of the function execution. See STD_RESULT type.
 */
STD_RESULT PUBSUB_Update(const SUB_HANDLE pSubscriberHandle,
                         void* const pDataBuf,
                         const U16 nDataBufferLength,
                         const U32 nTimeout)
{
    STD_RESULT eResult = RESULT_NOT_OK;

    if ((NULL_PTR != pSubscriberHandle) && (NULL_PTR != pDataBuf))
    {
        /* Get message from mail queue */
        osMailQId queue = (osMailQId)pSubscriberHandle;
        osEvent  stEventOS;
        stEventOS = osMailGet(queue, nTimeout);

        if (osEventMail == stEventOS.status)
        {
            void* pMsgData = stEventOS.value.p;

            //U16 nMsgSize = stEventOS.def.mail_id->queue_def->item_sz;//queue->queue_def->item_sz;

            if (NULL_PTR != pMsgData)
            {
                memcpy(pDataBuf, pMsgData, nDataBufferLength);
            }

            osMailFree((osMailQId)pSubscriberHandle, pMsgData);
            eResult = RESULT_OK;
        }
    }
    return eResult;
}



/***************************************************************************************************
* Definitions of local (private) functions
***************************************************************************************************/

/// None.



